#include "stream.h"
#include "range.h"
#include <map>
#include <vector>
#include <string>
#include <tr1/cstdint>
#include <arpa/inet.h>
#include <sstream>
#include <assert.h>

#ifndef NDEBUG
#include <cstdio>
#endif

using namespace std;



typedef multimap< range, rangedata > rmaptype;


void stream::register_sent(uint32_t start, uint32_t end, const timeval& ts)
{
	range key(adjust(start), adjust(end));
	rmaptype::iterator it, last, lo, hi, inserted;

	lo = ranges.lower_bound(key);
	hi = ranges.upper_bound(key);

	if (lo == hi)
	{
		// We have a completely new range
		ranges.insert(pair<range,rangedata>(key, ts));
		return;
	}

	last = hi--;

	// Check if we have new leading data
	if (key.seqno_lo < lo->first.seqno_lo)
	{
		ranges.insert(lo, rmaptype::value_type(range(key.seqno_lo, lo->first.seqno_lo), lo->second));
	}

	// Check if we have new trailing data
	if (key.seqno_hi > hi->first.seqno_hi)
	{
		last = ranges.insert(hi, rmaptype::value_type(range(hi->first.seqno_hi, key.seqno_hi), hi->second));
	}

	// Find partial matches and split existing ranges
	for (it = lo; it != last;)
	{
		range curr(it->first);
		rangedata& data = it->second;

		if (key.seqno_lo <= curr.seqno_lo && key.seqno_hi >= curr.seqno_hi)
		{
			// Existing range:  |-----|
			// New range:       |-----| 
			inserted = it++;
		}

		else if (key.seqno_lo > curr.seqno_lo && key.seqno_hi < curr.seqno_hi)
		{
			// Existing range: |-----|
			// New range:        |-| 
			ranges.insert(it, rmaptype::value_type(range(curr.seqno_lo, key.seqno_lo), data));
			inserted = ranges.insert(it, rmaptype::value_type(range(key.seqno_lo, key.seqno_hi), data));
			ranges.insert(it, rmaptype::value_type(range(key.seqno_hi, curr.seqno_hi), data));
			ranges.erase(it++);
		}

		else if (key.seqno_lo > it->first.seqno_lo)
		{
			// Existing range: |-----|
			// New range:        |---|
			ranges.insert(it, rmaptype::value_type(range(curr.seqno_lo, key.seqno_lo), data));
			inserted = ranges.insert(it, rmaptype::value_type(range(key.seqno_lo, curr.seqno_hi), data));
			ranges.erase(it++);
		}

		else if (key.seqno_hi < it->first.seqno_hi)
		{
			// Existing range: |-----|
			// New range:      |---|
			inserted = ranges.insert(it, rmaptype::value_type(range(curr.seqno_lo, key.seqno_hi), data));
			ranges.insert(it, rmaptype::value_type(range(key.seqno_hi, curr.seqno_hi), data));
			ranges.erase(it++);
		}

		else
		{
			// This really shouldn't happen!
			assert(false);
		}

		inserted->second.sent.push_back(ts);
	}

}



void stream::register_ack(uint32_t ackno, const timeval& ts)
{
}





/* Lighweight object used to map connections to addresses and ports */
struct stream_key
{
	uint32_t src, dst;
	uint16_t sport, dport;

	stream_key(uint32_t src, uint32_t dst, uint16_t sport, uint16_t dport)
		: src(src), dst(dst), sport(sport), dport(dport)
	{
	};

	bool operator<(const stream_key& rhs)
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
	};

	bool operator<(const stream_key& rhs) const
	{
		return const_cast<stream_key*>(this)->operator<(rhs);
	};

	stream_key(const stream_key& rhs)
	{
		*this = rhs;
	};

	stream_key& operator=(const stream_key& rhs)
	{
		src = rhs.src;
		dst = rhs.dst;
		sport = rhs.sport;
		dport = rhs.dport;

		return *this;
	};
};



/* Define our map type */
typedef map< stream_key, stream > smaptype;



/* A map over all streams/connections */
static smaptype connections;



/* Create a stream */
bool stream::create_connection(uint32_t src, uint16_t sport, uint32_t dst, uint16_t dport, uint32_t seqno, const timeval& ts)
{
	stream_key key(src, dst, sport, dport);

	// Try to find stream in the connection map
	smaptype::iterator lower_bound = connections.lower_bound(key);

	if (lower_bound != connections.end() && !(connections.key_comp()(key, lower_bound->first)))
	{
		// Stream was found
		lower_bound->second.first_seqno = seqno;
		lower_bound->second.first_segment = ts;
		lower_bound->second.last_segment = ts;
		return false;
	}

	// Stream was not found, we have to create it
	smaptype::iterator element = connections.insert(lower_bound, smaptype::value_type(key, stream(src, sport, dst, dport)));
	element->second.first_seqno = seqno;
	element->second.first_segment = ts;
	element->second.last_segment = ts;
	return true;
}



/* Find a stream or create it if it doesn't exist */
stream* stream::find_connection(uint32_t src, uint16_t sport, uint32_t dst, uint16_t dport)
{
	smaptype::iterator found = connections.find(stream_key(src, dst, sport, dport));

	if (found != connections.end())
	{
		return &found->second;
	}

	return NULL;
}



/* Create a list with references to the existing streams */
vector<const stream*> stream::list_connections()
{
	vector<const stream*> conns;

	for (smaptype::iterator it = connections.begin(); it != connections.end(); it++)
	{
		conns.push_back(&(it->second));
	}

	return conns;
}





stream::stream(uint32_t src, uint16_t sport, uint32_t dst, uint16_t dport)
	: src(src), dst(dst), sport(sport), dport(dport)
	, first_seqno(0), rtt(-1), highest_ackd(0)
{
}



stream& stream::operator=(const stream& rhs)
{
	src = rhs.src;
	dst = rhs.dst;
	sport = rhs.sport;
	dport = rhs.dport;
	first_seqno = rhs.first_seqno;
	first_segment = rhs.first_segment;

	return *this;
}



string stream::id()
{
	union
	{
		uint32_t addr;
		uint8_t str[4];
	} info;

	ostringstream connstr;

	info.addr = src;
	connstr << ((int) info.str[0]);
	connstr << ".";
	connstr << ((int) info.str[1]);
	connstr << ".";
	connstr << ((int) info.str[2]);
	connstr << ".";
	connstr << ((int) info.str[3]);
	connstr << ":";
	connstr << ntohs(sport);

	connstr << "=>";

	info.addr = dst;
	connstr << ((int) info.str[0]);
	connstr << ".";
	connstr << ((int) info.str[1]);
	connstr << ".";
	connstr << ((int) info.str[2]);
	connstr << ".";
	connstr << ((int) info.str[3]);
	connstr << ":";
	connstr << ntohs(dport);

	return connstr.str();
}

