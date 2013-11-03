#ifndef __RANGE_H__
#define __RANGE_H__

#include <vector>
#include <sys/time.h>
#include <tr1/cstdint>


// Forward declaration of stream
class stream;



/*
 * A rangedata object represents statistics about a byte range.
 */
class rangedata
{
	friend class stream;

	public:
	
		/* The elapsed time between when the range first was sent until it got ACK'ed */
		timeval latency();

		/* Number of times the range was retransmitted */
		int retransmissions();

		/* Number of times the range was lost */
		int lost();

		/* Number of times the range was ACK'ed (more than 1 means that we got dupACKs for this range) */
		int ack_count();

		/* Constructors and overloads for comparison operators */
		rangedata(const timeval& timestamp);

		rangedata(const rangedata& other)
		{
			*this = other;
		};

		rangedata& operator=(const rangedata& other);

	private:
		std::vector<timeval> sent;	// the timestamps this range was registered as sent
		std::vector<timeval> ackd;	// the timestamps this range was acknowledged
};



/*
 * A range represents a byte range (typically a TCP segment).
 */
class range
{
	friend class rangedata;
	friend class stream;

	private:
		uint32_t seqno_lo;	// the lower sequence number in the range (range start)
		uint32_t seqno_hi;	// the upper sequence number in the range (range end)

	public:
		range(uint32_t lo, uint32_t hi)
			: seqno_lo(lo), seqno_hi(hi)
		{
		};

		range(const range& other)
		{
			*this = other;
		};

		range& operator=(const range& other)
		{
			seqno_lo = other.seqno_lo;
			seqno_hi = other.seqno_hi;
			return *this;
		};

		bool operator<(const range& other)
		{
			if (seqno_hi <= other.seqno_lo)
				return true;
			if (seqno_lo >= other.seqno_hi)
				return false;

			return false;
		};

		bool operator<(const range& other) const
		{
			return const_cast<range*>(this)->operator<(other);
		};
};

#endif
