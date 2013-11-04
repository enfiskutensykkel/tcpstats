#include "flow.h"
#include "range.h"
#include <map>
#include <vector>
#include <algorithm>
#include <tr1/cstdint>
#include <sys/time.h>
#include <assert.h>

#include <cstdio>

/* Helper macros to check if X comes before Y */
#define sequential(x, y)  ((int32_t) ((x) - (y)) < 0)
#define isequential(x, y) ((int32_t) ((y) - (x)) >= 0)


/* Helper function to handle sequence number wrapping */
inline uint64_t flowdata::relative_seqno(uint32_t abs_current)
{
	uint64_t wrapped = abs_seqno_first + rel_seqno_max;

	if (abs_current < abs_seqno_max)
	{
		if (sequential(abs_current, abs_seqno_max))
		{
			// Retransmission of earlier sequence number
			wrapped -= (abs_seqno_max - abs_current);
		}
		else
		{
			// Sequence number has wrapped
			wrapped += (0 - abs_seqno_max) + abs_current;
		}

	}
	else
	{
		if (isequential(abs_seqno_max, abs_current))
		{
			// New sequence number
			wrapped += (abs_current - abs_seqno_max);
		}
		else
		{
			// Sequence number is older than abs_max, abs_max has wrapped
			wrapped -= (0 - abs_current) - abs_seqno_max;
		}
	}

	// Return relative sequence number
	return abs_current + ((wrapped / UINT32_MAX) * UINT32_MAX) - abs_seqno_first;
}



/*
 * Helper method to retrieve range matches.
 */
inline void flowdata::find_ranges(range_list& list, const range& key)
{
	range_map::iterator lo, hi, it;

	lo = ranges.lower_bound(key);
	hi = ranges.upper_bound(key);

	for (it = lo; it != hi; it++)
	{
		list.push_back(&it->second);
	}
}



/*
 * Helper method to retrieve and split matching ranges.
 */
inline void flowdata::find_and_split_ranges(range_list& list, const range& key, bool include_new_ranges)
{
	range_map::iterator curr, next, last, lo, hi, ins;

	lo = ranges.lower_bound(key);
	hi = ranges.upper_bound(key);

	if (lo == hi)
	{
		// A completely new range with no matching data
		return;
	}

	last = hi--;

	// Check if we have new leading data
	if (key.seqno_lo < lo->first.seqno_lo)
	{
		ins = ranges.insert(lo, range_map::value_type(range(key.seqno_lo, lo->first.seqno_lo), lo->second));

		if (include_new_ranges)
			list.push_back(&ins->second);
	}

	// Check if we have new trailing data
	if (key.seqno_hi > hi->first.seqno_hi)
	{
		last = ranges.insert(hi, range_map::value_type(range(hi->first.seqno_hi, key.seqno_hi), hi->second));

		if (include_new_ranges)
			list.push_back(&last->second);
	}

	// Find partial matches and split existing ranges
	for (curr = next = lo; curr != last; curr = next)
	{
		++next;
		const range& rrange = curr->first;
		rangedata& data = curr->second;

		if (key.seqno_lo <= rrange.seqno_lo && key.seqno_hi >= rrange.seqno_hi)
		{

			// The new range overlaps this chunk completely
			// Existing range:  |-----|
			// New range:       <-----> 
			ins = curr;
		}

		else if (key.seqno_lo > rrange.seqno_lo && key.seqno_hi < rrange.seqno_hi)
		{
			// The new range overlaps only part of this chunks, split into three chunks
			// Existing range: |-----|
			// New range:        |-| 
			ranges.insert(curr, range_map::value_type(range(rrange.seqno_lo, key.seqno_lo), data));
			ins = ranges.insert(curr, range_map::value_type(range(key.seqno_lo, key.seqno_hi), data));
			ranges.insert(curr, range_map::value_type(range(key.seqno_hi, rrange.seqno_hi), data));
			ranges.erase(curr);
		}

		else if (key.seqno_lo > rrange.seqno_lo)
		{
			// The new range overlaps the latter part of this chunk, split into two chunks
			// Existing range: |-----|
			// New range:        |---|
			ranges.insert(curr, range_map::value_type(range(rrange.seqno_lo, key.seqno_lo), data));
			ins = ranges.insert(curr, range_map::value_type(range(key.seqno_lo, rrange.seqno_hi), data));
			ranges.erase(curr);
		}

		else if (key.seqno_hi < rrange.seqno_hi)
		{
			// The new range overlaps the former part of this chunk, split into two chunks
			// Existing range: |-----|
			// New range:      |---|
			ins = ranges.insert(curr, range_map::value_type(range(rrange.seqno_lo, key.seqno_hi), data));
			ranges.insert(curr, range_map::value_type(range(key.seqno_hi, rrange.seqno_hi), data));
			ranges.erase(curr);
		}

		else
		{
			// This really shouldn't happen!
			assert(false);
		}

		list.push_back(&ins->second);
	}

	return;
}



/*
 * Increase sent count on a byte range.
 */
void flowdata::register_sent(uint32_t start, uint32_t end, const timeval& ts)
{
	if (ranges.empty())
	{
		abs_seqno_first = start;
		abs_seqno_max = end;
		rel_seqno_max = 0;

		highest_ackd = rel_seqno_max;
		rtt_min = -1;

		ts_first = ts;
		ts_last = ts;
	}

	// Calculate and update relative sequence numbers
	uint64_t rel_start, rel_end;
	rel_start = relative_seqno(start);
	rel_end = relative_seqno(end);

	if (rel_end > rel_seqno_max)
	{
		rel_seqno_max = rel_end;
		abs_seqno_max = end;
	}

	// Check if there actually is any data to update
	if ((end - start) == 0)
	{
		return;
	}

	// Find byte ranges that has matches
	range key(rel_start, rel_end);
	range_list list;
	find_and_split_ranges(list, key, false);


	if (list.empty())
	{
		// We have a completely new range
		ranges.insert(std::pair<range,rangedata>(key, ts));
	}
	else
	{
		// Update existing ranges' transmission count
		for (range_list::iterator it = list.begin(); it != list.end(); ++it)
		{
			(*it)->sent.push_back(ts);
		}
	}
}



/*
 * Mark a byte range as acknowledged.
 */
void flowdata::register_ack(uint32_t ackno, const timeval& ts)
{
	// We assume that only segments with ACK flag set is registered
	// This assumption holds as long as the trace isn't missing any packets
	//uint64_t rel_ackno = relative
	//assert(!ranges.empty() || (highest_ackd == first_seqno - 1)); 

	if (ackno < highest_ackd)
	{
	}
}



uint32_t flowdata::total_retransmissions() const
{
	uint32_t retr = 0;

	for (range_map::const_iterator it = ranges.begin(); it != ranges.end(); it++)
	{
		retr += it->second.sent.size() - 1;
	}

	return retr;
}



flowdata& flowdata::operator=(const flowdata& rhs)
{
	// FIXME: Only copy the members that needs to be copied, we should avoid deep-copies anyway
	abs_seqno_first = rhs.abs_seqno_first;
	abs_seqno_max = rhs.abs_seqno_max;
	rel_seqno_max = rhs.rel_seqno_max;

	rtt_min = rhs.rtt_min;
	highest_ackd = rhs.highest_ackd;

	ts_first = rhs.ts_first;
	ts_last = rhs.ts_last;

	return *this;
}
