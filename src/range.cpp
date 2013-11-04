#include "range.h"
#include <vector>
#include <tr1/cstdint>
#include <sys/time.h>



range::range(uint64_t lo, uint64_t hi)
	: seqno_lo(lo), seqno_hi(hi)
{
}



range::range(const range& rhs)
{
	*this = rhs;
}



range& range::operator=(const range& rhs)
{
	seqno_lo = rhs.seqno_lo;
	seqno_hi = rhs.seqno_hi;

	return *this;
}



bool range::operator<(const range& rhs)
{
	if (seqno_hi <= rhs.seqno_lo)
		return true;
	if (seqno_lo >= rhs.seqno_hi)
		return false;

	return false;
}



bool range::operator<(const range& rhs) const
{
	return const_cast<range*>(this)->operator<(rhs);
}





rangedata::rangedata(const timeval& ts)
{
	sent.push_back(ts);
}



rangedata& rangedata::operator=(const rangedata& rhs)
{
	sent = rhs.sent;
	ackd = rhs.ackd;
	return *this;
}
