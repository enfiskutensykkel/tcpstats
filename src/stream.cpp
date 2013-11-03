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



void stream::get_ranges(range_list& list, range& key, bool include)
{
	range_map::iterator it, last, lo, hi, inserted;

	lo = ranges.lower_bound(key);
	hi = ranges.upper_bound(key);

	if (lo == hi)
	{
		return;
	}

	last = hi--;

	// Check if we have new leading data
	if (key.seqno_lo < lo->first.seqno_lo)
	{
		inserted = ranges.insert(lo, range_map::value_type(range(key.seqno_lo, lo->first.seqno_lo), lo->second));
		if (include)
			list.push_back(inserted);
	}

	// Check if we have new trailing data
	if (key.seqno_hi > hi->first.seqno_hi)
	{
		last = ranges.insert(hi, range_map::value_type(range(hi->first.seqno_hi, key.seqno_hi), hi->second));
		if (include)
			list.push_back(last);
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
			ranges.insert(it, range_map::value_type(range(curr.seqno_lo, key.seqno_lo), data));
			inserted = ranges.insert(it, range_map::value_type(range(key.seqno_lo, key.seqno_hi), data));
			ranges.insert(it, range_map::value_type(range(key.seqno_hi, curr.seqno_hi), data));
			ranges.erase(it++);
		}

		else if (key.seqno_lo > it->first.seqno_lo)
		{
			// Existing range: |-----|
			// New range:        |---|
			ranges.insert(it, range_map::value_type(range(curr.seqno_lo, key.seqno_lo), data));
			inserted = ranges.insert(it, range_map::value_type(range(key.seqno_lo, curr.seqno_hi), data));
			ranges.erase(it++);
		}

		else if (key.seqno_hi < it->first.seqno_hi)
		{
			// Existing range: |-----|
			// New range:      |---|
			inserted = ranges.insert(it, range_map::value_type(range(curr.seqno_lo, key.seqno_hi), data));
			ranges.insert(it, range_map::value_type(range(key.seqno_hi, curr.seqno_hi), data));
			ranges.erase(it++);
		}

		else
		{
			// This really shouldn't happen!
			assert(false);
		}

		list.push_back(inserted);
	}

	return;
}



void stream::register_sent(uint32_t start, uint32_t end, const timeval& ts)
{
	range key(adjust(start), adjust(end));
	range_list ranges;
	get_ranges(ranges, key, false);

	// TODO: update last_segment if ts > last_segment

	if (ranges.empty())
	{
		// We have a completely new range
		this->ranges.insert(pair<range,rangedata>(key, ts));
	}
	else
	{
		// Update existing ranges
		for (range_list::iterator i = ranges.begin(); i != ranges.end(); ++i)
		{
			(*i)->second.sent.push_back(ts);
		}
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
typedef map< stream_key, stream > stream_map;



/* A map over all streams/connections */
static stream_map connections;



/* Create a stream */
bool stream::create_connection(uint32_t src, uint16_t sport, uint32_t dst, uint16_t dport, uint32_t seqno, const timeval& ts)
{
	stream_key key(src, dst, sport, dport);

	// Try to find stream in the connection map
	stream_map::iterator lower_bound = connections.lower_bound(key);

	if (lower_bound != connections.end() && !(connections.key_comp()(key, lower_bound->first)))
	{
		// Stream was found
		lower_bound->second.first_seqno = seqno;
		lower_bound->second.highest_ackd = seqno;
		lower_bound->second.first_segment = ts;
		lower_bound->second.last_segment = ts;
		return false;
	}

	// Stream was not found, we have to create it
	stream_map::iterator element = connections.insert(lower_bound, stream_map::value_type(key, stream(src, sport, dst, dport)));
	element->second.first_seqno = seqno;
	element->second.highest_ackd = seqno;
	element->second.first_segment = ts;
	element->second.last_segment = ts;
	return true;
}



/* Find a stream or create it if it doesn't exist */
stream* stream::find_connection(uint32_t src, uint16_t sport, uint32_t dst, uint16_t dport)
{
	stream_map::iterator found = connections.find(stream_key(src, dst, sport, dport));

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

	for (stream_map::iterator it = connections.begin(); it != connections.end(); it++)
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

