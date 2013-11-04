#include <cstdio>
#include <vector>
#include <pcap.h>
#include "trace.h"
#include "flow.h"

using std::vector;


int main(int argc, char** argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s tracefile\n", argv[0]);
		return 1;
	}

	char errbuf[PCAP_ERRBUF_SIZE];
	filter f;

	pcap_t* handle = pcap_open_offline(argv[1], errbuf);

	set_filter(handle, f);

	analyze_trace(handle);

	vector<const flow*> connections;
	vector<const flowdata*> data;
	
	int count = flow::list_connections(connections, data);
	printf("Connections found: %d\n", count);

	pcap_close(handle);

	return 0;
}
