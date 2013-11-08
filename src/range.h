#ifndef __RANGE_H__
#define __RANGE_H__

#include <vector>
#include <tr1/cstdint>



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

		/* Constructors and overloads for comparison operators */
		inline rangedata()
			: tx(0), rx(0)
		{
		};

		inline rangedata(uint64_t timestamp)
			: tx(1), rx(0)
		{
			evt.push_back(true);
			ts.push_back(timestamp);
		};

		inline rangedata(const rangedata& rhs)
		{
			*this = rhs;
		};

		inline rangedata& operator=(const rangedata& rhs)
		{
			evt = rhs.evt;
			ts = rhs.ts;
			tx = rhs.tx;
			rx = rhs.rx;
			return *this;
		};

	private: 
		std::vector<bool> evt;
		std::vector<uint64_t> ts;
		uint64_t tx;
		uint64_t rx;
};

#endif
