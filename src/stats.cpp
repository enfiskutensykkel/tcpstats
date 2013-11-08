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
		int32_t r = it->second.tx - it->second.rx;
		retr += r > 0 ? r : 0;
	}

	return retr;
}



uint32_t flowdata::max_num_retrans() const
{
	int32_t max = 0;

	for (range_map::const_iterator it = ranges.begin(); it != ranges.end(); it++)
	{
		int32_t r = it->second.tx - it->second.rx;
		max = r > max ? r : max;
	}

	return (uint32_t) max;
}



uint32_t flowdata::max_num_dupacks() const
{
	uint32_t dupacks = 0;

	for (range_map::const_iterator it = ranges.begin(); it != ranges.end(); it++)
	{
		int32_t d = it->second.rx - it->second.tx;
		if (d > 0 && (uint32_t) d > dupacks)
		{
			dupacks = d;
		}
	}

	return dupacks;
}



uint32_t flowdata::total_dupacks() const
{
	uint32_t dupacks = 0;

	for (range_map::const_iterator it = ranges.begin(); it != ranges.end(); it++)
	{
		int32_t d = it->second.rx - it->second.tx;
		dupacks += d > 0 ? d : 0;
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
	return rtt_min;
	uint64_t rtt = UINT64_MAX;

	for (range_map::const_iterator it = ranges.begin(); it != ranges.end(); it++)
	{
		if (it->second.rx > 0)
		{
			const rangedata& data = it->second;

			uint64_t idx = 0;
			uint64_t tx_count = data.tx;

			while (tx_count > 0)
			{
				if (data.evt[idx])
				{
					--tx_count;
				}
			}

			uint64_t last_tx = data.ts.at(idx);
			uint64_t last_rx = data.ts.at(idx+1);

			if ((last_rx - last_tx) < rtt)
			{
				rtt = last_rx - last_tx;
			}
		}
	}

	return rtt != UINT64_MAX ? rtt : 0;
}



uint64_t flowdata::duration() const
{
	return USECS(ts_last) - USECS(ts_first);
}
