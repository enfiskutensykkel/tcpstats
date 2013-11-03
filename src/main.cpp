#include <cstdio>
#include <vector>
#include <pcap.h>
#include "stream.h"
#include "pcap.h"

#ifndef NDEBUG
#include <map>
#include "range.h"
#endif


int main(int argc, char** argv)
{
#if 0
	using std::vector;

	char errbuf[PCAP_ERRBUF_SIZE];

	pcap_t* handle = pcap_open_offline(argv[1], errbuf);
	initialize_streams(handle);
	pcap_close(handle);

	vector<const stream*> streams = stream::list_connections();
	for (vector<const stream*>::const_iterator i = streams.begin(); i != streams.end(); i++)
	{
		printf("%s\n", (*i)->id().c_str());
	}

	handle = pcap_open_offline(argv[1], errbuf);
	analyze_streams(handle);
	pcap_close(handle);
#else
	struct timeval tv = {0, 0};
	stream::create_connection(0, 0, 1, 1, 0, tv);

	stream* s = stream::find_connection(0, 0, 1, 1);
	s->register_sent(10, 20, tv);
	s->register_sent(20, 30, tv);
	s->register_sent(30, 40, tv);

	// Partial match within an already registered range
	//s->register_sent(15, 20, tv);

	// Same as above
//	s->register_sent(10, 15, tv);

	// Partial match spanning two ranges
	//s->register_sent(15, 17, tv);

	// Partial match with new trailing data
//	s->register_sent(25, 35, tv);

	// Partial match with both trailing and leading data
	s->register_sent(9, 15, tv);
	


	for (std::multimap<range,rangedata>::iterator it = s->ranges.begin(); it != s->ranges.end(); it++)
	{
		printf("%lu %lu %d\n", it->first.seqno_lo, it->first.seqno_hi, it->second.sent.size());
	}
#endif
	return 0;
}
