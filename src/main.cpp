#include <cstdio>
#include <vector>
#include <pcap.h>
#include "stream.h"
#include "pcap.h"


int main(int argc, char** argv)
{
	using std::vector;

	char errbuf[PCAP_ERRBUF_SIZE];

	pcap_t* handle = pcap_open_offline(argv[1], errbuf);
	initialize_streams(handle);
	pcap_close(handle);

	vector<stream*> streams = stream::list_connections();
	for (vector<stream*>::iterator i = streams.begin(); i != streams.end(); i++)
	{
		printf("%s\n", (*i)->id().c_str());
	}

	handle = pcap_open_offline(argv[1], errbuf);
	analyze_streams(handle);
	pcap_close(handle);

	return 0;
}
