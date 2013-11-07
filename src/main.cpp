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
	}

	vector<const flow*> connections;
	vector<const flowdata*> data;
	
	unsigned count = flow::list_connections(connections, data);
	printf("Connections found: %d\n", count);

	for (unsigned i = 0; i < count; ++i)
	{
		printf("%s has %u (%u) retransmissions and %u dupacks (%lu)\n", connections[i]->id().c_str(), data[i]->total_retrans(), data[i]->max_num_retrans(), data[i]->total_dupacks(), data[i]->unique_bytes_sent());
	}

	return 0;
}
