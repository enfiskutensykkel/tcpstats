#include "stream.h"
#include <string>


using std::string;



stream::stream(uint32_t src, uint32_t dst, uint16_t sport, uint16_t dport)
	: src(src), dst(dst), sport(sport), dport(dport)
{
}



stream& stream::operator=(const stream& rhs)
{
	src = rhs.src;
	dst = rhs.dst;
	sport = rhs.sport;
	dport = rhs.dport;

	return *this;
}



bool stream::operator<(const stream& rhs)
{
	if (src < rhs.src)
		return true;
	if (src > rhs.src)
		return false;

	if (dst < rhs.dst)
		return true;
	if (dst > rhs.dst)
		return false;

	if (sport < rhs.sport)
		return true;
	if (sport > rhs.sport)
		return false;

	if (dport < rhs.dport)
		return true;
	if (dport > rhs.dport)
		return false;

	return false;
}
