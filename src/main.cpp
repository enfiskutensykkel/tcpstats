#include <stdexcept>
#include <cstdio>
#include <vector>
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

	filter f;

	try
	{
		FILE* fp = fopen(argv[1], "r");
		analyze_trace(fp, f);
		fclose(fp);
	}
	catch (const std::runtime_error& e)
	{
		fprintf(stderr, "Unexpected error: %s\n", e.what());
		return 2;
	}

	vector<const flow*> connections;
	vector<const flowdata*> data;
	
	unsigned count = flow::list_connections(connections, data);
	printf("Connections found: %d\n\n", count);

	for (unsigned i = 0; i < count; ++i)
	{
		const flow* f = connections[i];
		const flowdata* d = data[i];

		printf("%s has sent %lu unique bytes\n", f->id().c_str(), d->unique_bytes_sent());
		printf("%s has %u (%u) retransmissions\n", f->id().c_str(), d->total_retrans(), d->max_num_retrans());
		printf("%s has RTT %.2f ms\n", f->id().c_str(), d->rtt() / 1000.0);
		printf("%s has %u (%u) dupacks\n", f->id().c_str(), d->total_dupacks(), d->max_num_dupacks());
		printf("%s lasted %.2f seconds\n", f->id().c_str(), d->duration() / 1000000.0);

		printf("\n");
	}

	return 0;
}
