#ifndef __RANGE_H__
#define __RANGE_H__

#include <vector>
#include <sys/time.h>
#include <tr1/cstdint>


class stream;


class range
{
	public:
		range(uint64_t seqno_start, uint64_t seqno_end, const timeval& registered);

		range(const range& other)
		{
			*this = other;
		};

		range& operator=(const range& other);

		bool operator<(const range& other);

		bool operator<(const range& other) const
		{
			return const_cast<range*>(this)->operator<(other);
		};

	private:

		std::vector<timeval> registered;
		timeval acked;

		uint64_t seq_lo;
		uint64_t seq_hi;

		friend class stream;
		friend struct comparator;
};

#endif
