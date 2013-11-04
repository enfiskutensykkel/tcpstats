#include "flow.h"
#include "range.h"
#include <map>
#include <vector>
#include <tr1/cstdint>
#include <sys/time.h>
#include <assert.h>



/*
 * Helper method to retrieve and split matching ranges.
 */
void flowdata::find_and_split_ranges(range_list& list, range& key, bool include_new_ranges)
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
	// TODO: update last_segment if ts > last_segment
	
	// Check if there actually is any data to update
	if ((end - start) == 0)
	{
		return;
	}

	// Find byte ranges that has matches
	range key(adjust(start), adjust(end));
	range_list ranges;
	find_and_split_ranges(ranges, key, false);


	if (ranges.empty())
	{
		// We have a completely new range
		this->ranges.insert(std::pair<range,rangedata>(key, ts));
	}
	else
	{
		// Update existing ranges' transmission count
		for (range_list::iterator i = ranges.begin(); i != ranges.end(); ++i)
		{
			(*i)->sent.push_back(ts);
		}
	}
}



/*
 * Mark a byte range as acknowledged.
 */
void flowdata::register_ack(uint32_t ackno, const timeval& ts)
{
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



flowdata::flowdata(uint32_t first_seqno, const timeval& first_ts)
	: first_seqno(first_seqno), highest_ackd(first_seqno), rtt(-1)
	, first_ts(first_ts), last_ts(first_ts)
{
}



flowdata& flowdata::operator=(const flowdata& rhs)
{
	first_seqno = rhs.first_seqno;
	highest_ackd = rhs.highest_ackd;
	rtt = rhs.rtt;
	first_ts = rhs.first_ts;
	last_ts = rhs.last_ts;

	// TODO: Only copy the members that needs to be copied, we should avoid deep-copies anyway
	return *this;
}
