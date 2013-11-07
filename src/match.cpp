#include "flow.h"
#include "range.h"
#include <map>
#include <vector>
#include <algorithm>
#include <tr1/cstdint>
#include <sys/time.h>
#include <assert.h>

/* Helper macros to check if X comes before Y */
#define sequential(x, y)   ((int32_t) ((x) - (y)) < 0)
#define i_sequential(x, y) ((int32_t) ((y) - (x)) >= 0)


/* 
 * Helper function to handle sequence number wrapping 
 */
static inline uint64_t relative(uint32_t seqno, uint32_t first, uint32_t last, uint64_t current)
{
	uint64_t wrapped = first + current;

	if (seqno < last)
	{
		if (sequential(seqno, last))
		{
			// Earlier sequence number
			wrapped -= (last - seqno);
		}
		else
		{
			// Sequence number has wrapped
			wrapped += (0 - last) + seqno;
		}
	}
	else
	{
		if (i_sequential(last, seqno))
		{
			// New sequence number
			wrapped += (seqno - last);
		}
		else
		{
			// Sequence number is older than last, last has wrapped
			wrapped -= (0 - seqno) - last;
		}
	}

	// Return the relative sequence number
	return seqno + ((wrapped / (((uint64_t) UINT32_MAX) + 1)) * (((uint64_t) UINT32_MAX) + 1)) - first;
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
		const range& curr_range = curr->first;
		rangedata& curr_data = curr->second;

		if (key.seqno_lo <= curr_range.seqno_lo && key.seqno_hi >= curr_range.seqno_hi)
		{

			// The new range overlaps this chunk completely
			// Existing range:  |-----|
			// New range:       <-----> 
			ins = curr;
		}

		else if (key.seqno_lo > curr_range.seqno_lo && key.seqno_hi < curr_range.seqno_hi)
		{
			// The new range overlaps only part of this chunks, split into three chunks
			// Existing range: |-----|
			// New range:        |-| 
			ranges.insert(curr, range_map::value_type(range(curr_range.seqno_lo, key.seqno_lo), curr_data));
			ins = ranges.insert(curr, range_map::value_type(range(key.seqno_lo, key.seqno_hi), curr_data));
			ranges.insert(curr, range_map::value_type(range(key.seqno_hi, curr_range.seqno_hi), curr_data));
			ranges.erase(curr);
		}

		else if (key.seqno_lo > curr_range.seqno_lo)
		{
			// The new range overlaps the latter part of this chunk, split into two chunks
			// Existing range: |-----|
			// New range:        |---|
			ranges.insert(curr, range_map::value_type(range(curr_range.seqno_lo, key.seqno_lo), curr_data));
			ins = ranges.insert(curr, range_map::value_type(range(key.seqno_lo, curr_range.seqno_hi), curr_data));
			ranges.erase(curr);
		}

		else if (key.seqno_hi < curr_range.seqno_hi)
		{
			// The new range overlaps the former part of this chunk, split into two chunks
			// Existing range: |-----|
			// New range:      |---|
			ins = ranges.insert(curr, range_map::value_type(range(curr_range.seqno_lo, key.seqno_hi), curr_data));
			ranges.insert(curr, range_map::value_type(range(key.seqno_hi, curr_range.seqno_hi), curr_data));
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
	if (rel_seqno_max == UINT64_MAX)
	{
		abs_seqno_min = abs_seqno_max = end;
		rel_seqno_max = 0;

		ts_first = ts;
		ts_last = ts;
	}

	if (USECS(ts) > USECS(ts_last))
	{
		ts_last = ts;
	}

	// Calculate and update relative sequence numbers
	uint64_t rel_start, rel_end;
	rel_start = relative(start, abs_seqno_min, abs_seqno_max, rel_seqno_max);
	rel_end = relative(end, abs_seqno_min, abs_seqno_max, rel_seqno_max);

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
	if (curr_ack == UINT64_MAX)
	{
		abs_ackno_min = abs_ackno_max = ackno;
		curr_ack = prev_ack = 0;
	}

	uint64_t rel_ackno = relative(ackno, abs_ackno_min, abs_ackno_max, curr_ack);

	if (rel_ackno == 0 || (ackno - abs_seqno_min) == 1)
	{
		return;
	}

	range_list list;

	if (rel_ackno <= prev_ack)
	{
		// This shouldn't happen
		assert(false);
	}
	else if (rel_ackno <= curr_ack)
	{
		// We got a duplicate ACK
		range key(prev_ack, rel_ackno);
		find_and_split_ranges(list, key, true);
	}
	else if (rel_ackno > curr_ack)
	{
		// We got a new ACK
		range key(curr_ack, rel_ackno);
		find_and_split_ranges(list, key, true);

		abs_ackno_max = ackno;
		prev_ack = curr_ack;
		curr_ack = rel_ackno;
	}

	// Update acknowledgement times for all the matching ranges
	for (range_list::iterator it = list.begin(); it != list.end(); ++it)
	{
		if ((USECS(ts) - USECS((*it)->sent.back())) < rtt_min)
		{
			rtt_min = (USECS(ts) - USECS((*it)->sent.back()));
		}

		(*it)->ackd.push_back(ts);
	}
}



flowdata::flowdata()
	: abs_seqno_min(0), abs_seqno_max(0), rel_seqno_max(UINT64_MAX)
	, abs_ackno_min(0), abs_ackno_max(0), curr_ack(UINT64_MAX), prev_ack(UINT64_MAX)
	, rtt_min(UINT64_MAX)
{
	ts_first.tv_sec = ts_first.tv_usec = 0;
	ts_last.tv_sec = ts_last.tv_usec = 0;
}



flowdata& flowdata::operator=(const flowdata& rhs)
{
	// FIXME: Only copy the members that needs to be copied
	// we should avoid deep-copies anyway
	abs_seqno_min = rhs.abs_seqno_min;
	abs_seqno_max = rhs.abs_seqno_max;
	rel_seqno_max = rhs.rel_seqno_max;

	curr_ack = rhs.curr_ack;
	prev_ack = rhs.prev_ack;

	rtt_min = rhs.rtt_min;

	ts_first = rhs.ts_first;
	ts_last = rhs.ts_last;

	return *this;
}
