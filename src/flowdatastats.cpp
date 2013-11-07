#include "flow.h"
#include "range.h"
#include <tr1/cstdint>
#include <sys/time.h>


uint32_t flowdata::total_retransmissions() const
{
	uint32_t retr = 0;

	for (range_map::const_iterator it = ranges.begin(); it != ranges.end(); it++)
	{
		retr += it->second.sent.size() - 1;
	}

	return retr;
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
