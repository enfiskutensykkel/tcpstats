#include "flow.h"
#include "range.h"
#include <vector>
#include <tr1/cstdint>
#include <sys/time.h>


uint32_t flowdata::total_retrans() const
{
	uint32_t retr = 0;


	for (range_map::const_iterator it = ranges.begin(); it != ranges.end(); it++)
	{
		int32_t retries = it->second.sent.size() - it->second.ackd.size();

		retr += retries > 0 ? retries : 0;
	}

	return retr;
}



uint32_t flowdata::max_num_retrans() const
{
	uint32_t max = 0;

	for (range_map::const_iterator it = ranges.begin(); it != ranges.end(); it++)
	{
		int32_t size = it->second.sent.size() - it->second.ackd.size();
		if (size > 0 && (uint64_t) size > max)
		{
			max = size;
		}
	}

	return max;
}



uint32_t flowdata::max_num_dupacks() const
{
	uint32_t dupacks = 0;

	for (range_map::const_iterator it = ranges.begin(); it != ranges.end(); it++)
	{
		uint32_t n = it->second.ackd.size();
		if (n > 1 && (n - 1) > dupacks)
		{
			dupacks = n;
		}
	}

	return dupacks;
}



uint32_t flowdata::total_dupacks() const
{
	uint32_t dupacks = 0;

	for (range_map::const_iterator it = ranges.begin(); it != ranges.end(); it++)
	{
		int32_t size = it->second.ackd.size() - it->second.sent.size();
		dupacks += size > 0 ? size : 0;
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



uint64_t flowdata::rtt() const
{
	uint64_t rtt = UINT64_MAX;

	for (range_map::const_iterator it = ranges.begin(); it != ranges.end(); it++)
	{
		if (!it->second.ackd.empty())
		{
			const rangedata& data = it->second;
			uint64_t time = USECS(data.ackd.back()) - USECS(data.sent.back());
			if (time < rtt)
			{
				rtt = time;
			}
		}
	}

	return rtt;
}



uint64_t flowdata::duration() const
{
	return USECS(ts_last) - USECS(ts_first);
}
