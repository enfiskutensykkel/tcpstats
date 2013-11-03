#include "range.h"



bool range::operator<(const range& rhs)
{
	if (seq_lo < rhs.seq_lo)
		return true;
	if (seq_lo > rhs.seq_lo)
		return false;

	if (seq_hi < rhs.seq_hi)
		return true;
	if (seq_hi > rhs.seq_hi)
		return false;

	return false;
}
