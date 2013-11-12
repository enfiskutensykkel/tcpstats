#ifndef DEBUG
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

#else
#include "testfuncs.h"
#include <string.h>
#include <cstdio>
#include <sstream>
#include <vector>

#define test(x) test_case(#x, x)

struct test_case
{
	const char* name;
	bool (*func)(std::ostringstream&);
	std::ostringstream output;
	bool run;
	bool status;

	test_case(const char* name, bool (*test)(std::ostringstream&))
		: name(name), func(test), run(false), status(false)
	{
	};

	test_case(const test_case& rhs)
	{
		*this = rhs;
	};

	test_case& operator=(const test_case& rhs)
	{
		name = rhs.name;
		func = rhs.func;
		run = false;
		status = false;
		return *this;
	};

	bool operator()()
	{
		if (!run)
		{
			status = func(output);
			run = true;
		}
		return status;
	};
};



int main(int argc, char** argv)
{
	std::vector<test_case> tests;
	tests.push_back(test(test_range_matching));

	if (argc > 1)
	{
		for (std::vector<test_case>::iterator it = tests.begin(); it != tests.end(); it++)
		{
			if (strcmp(it->name, argv[1]) == 0)
			{
				fprintf(stderr, "Running test '%s'... ", it->name);
				bool result = (*it)();
				fprintf(stderr, "%4s\n", result ? "\033[0;92mPASS\033[0m" : "\033[0;91mFAIL\033[0m");
				const char* sep = "=================";
				fprintf(stderr, "\n%s\n", sep);
				fprintf(stdout, "%s", it->output.str().c_str());
				fflush(stdout);
				fprintf(stderr, "\n%s\n", sep);

				return result ? 0 : 1;
			}
		}

		fprintf(stderr, "Test '%s' unknown\n", argv[1]);
		return 1;
	}
	else
	{
		unsigned failed = 0;

		fprintf(stderr, "Running tests...\n");
		for (std::vector<test_case>::iterator test = tests.begin(); test != tests.end(); test++)
		{
			bool result = (*test)();
			fprintf(stderr, "  %24s%s%4s\n", test->name, "   ", result ? "\033[0;92mPASS\033[0m" : "\033[0;91mFAIL\033[0m");
			failed += !result;
		}
		fprintf(stderr, "%lu out of %lu tests passed\n", (tests.size() - failed), tests.size());

		return failed > 0;
	}
}

#endif
