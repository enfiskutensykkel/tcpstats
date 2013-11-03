#ifndef __RANGE_H__
#define __RANGE_H__

#include <vector>
#include <sys/time.h>
#include <tr1/cstdint>


class stream;

class range
{
	public:
		range(uint64_t sequence_number_start, uint64_t sequence_number_end, const timeval& registered);

		range(const range& other)
		{
			*this = other;
		};

		range& operator=(const range& other);

	private:
		std::vector<timeval> reg_times;
		std::vector<timeval> ack_times;

		uint64_t seq_lo;
		uint64_t seq_hi;

		friend class stream;
};

#endif
