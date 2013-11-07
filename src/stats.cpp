#include "flow.h"
#include "range.h"
#include <tr1/cstdint>
#include <sys/time.h>


uint32_t flowdata::total_retrans() const
{
	uint32_t retr = 0;

	for (range_map::const_iterator it = ranges.begin(); it != ranges.end(); it++)
	{
		retr += it->second.sent.size() - 1;
	}

	return retr;
}



uint32_t flowdata::max_num_retrans() const
{
	uint32_t max = 0;

	for (range_map::const_iterator it = ranges.begin(); it != ranges.end(); it++)
	{
		uint32_t size = it->second.sent.size() - 1;
		if (size > max)
		{
			max = size;
		}
	}

	return max;
}



uint32_t flowdata::total_dupacks() const
{
	uint32_t dupacks = 0;

	for (range_map::const_iterator it = ranges.begin(); it != ranges.end(); it++)
	{
		dupacks += it->second.ackd.size() - 1;
	}

	return dupacks;
}



uint64_t flowdata::unique_bytes_sent() const
{
	uint64_t byte_count = 0;

	for (range_map::const_iterator it = ranges.begin(); it != ranges.end(); it++)
	{
		byte_count += it->first.seqno_hi - it->first.seqno_lo;
	}

	return byte_count;
}
