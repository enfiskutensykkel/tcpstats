#ifndef __RANGE_H__
#define __RANGE_H__

#include <vector>
#include <sys/time.h>
#include <tr1/cstdint>


/* Forward declaration of flowdata */
class flowdata;



/*
 * A range object represents a byte range (typically payload of a TCP segment).
 */
class range
{
	friend class flowdata;

	private:
		uint64_t seqno_lo;	// the lower sequence number in the range (range start)
		uint64_t seqno_hi;	// the upper sequence number in the range (range end)

	public:
		inline range(uint64_t seqno_start, uint64_t seqno_end)
			: seqno_lo(seqno_start), seqno_hi(seqno_end)
		{
		};

		inline range(const range& rhs)
		{
			*this = rhs;
		};

		inline range& operator=(const range& rhs)
		{
			seqno_lo = rhs.seqno_lo;
			seqno_hi = rhs.seqno_hi;
			return *this;
		};

		inline bool operator<(const range& rhs)
		{
			if (seqno_hi <= rhs.seqno_lo)
				return true;
			if (seqno_lo >= rhs.seqno_hi)
				return false;
			return false;
		};

		inline bool operator<(const range& rhs) const
		{
			return const_cast<range*>(this)->operator<(rhs);
		};
};



/*
 * A rangedata object represents statistics about a byte range.
 */
class rangedata
{
	friend class flowdata;

	public:
	
		/* The elapsed time between when the range first was sent until it got ACK'ed */
		timeval latency();

		/* Constructors and overloads for comparison operators */
		inline rangedata(const timeval& timestamp)
		{
			sent.push_back(timestamp);
		};

		inline rangedata(const rangedata& other)
		{
			*this = other;
		};

		inline rangedata& operator=(const rangedata& rhs)
		{
			sent = rhs.sent;
			ackd = rhs.ackd;
			return *this;
		};

	private: 
		std::vector<timeval> sent;	// the timestamps this range was registered as sent
		std::vector<timeval> ackd;	// the timestamps this range was acknowledged
};

#endif
