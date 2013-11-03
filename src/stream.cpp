#include "stream.h"
#include <map>
#include <vector>
#include <string>
#include <tr1/cstdint>
#include <arpa/inet.h>
#include <sstream>

using std::string;
using std::multimap;
using std::vector;
using std::map;

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
typedef map< stream_key, stream > maptype;





/* A map over all streams/connections */
static maptype connections;



/* Create a stream */
bool stream::create_connection(uint32_t src, uint16_t sport, uint32_t dst, uint16_t dport, uint32_t seqno, const timeval& ts)
{
	stream_key key(src, dst, sport, dport);

	// Try to find stream in the connection map
	maptype::iterator lower_bound = connections.lower_bound(key);

	if (lower_bound != connections.end() && !(connections.key_comp()(key, lower_bound->first)))
	{
		// Stream was found
		lower_bound->second.first_seqno = seqno;
		lower_bound->second.first_segment = ts;
		lower_bound->second.last_segment = ts;
		return false;
	}

	// Stream was not found, we have to create it
	maptype::iterator element = connections.insert(lower_bound, maptype::value_type(key, stream(src, sport, dst, dport)));
	element->second.first_seqno = seqno;
	element->second.first_segment = ts;
	element->second.last_segment = ts;
	return true;
}



/* Find a stream or create it if it doesn't exist */
stream* stream::find_connection(uint32_t src, uint16_t sport, uint32_t dst, uint16_t dport)
{
	maptype::iterator found = connections.find(stream_key(src, dst, sport, dport));

	if (found != connections.end())
	{
		return &found->second;
	}

	return NULL;
}



/* Create a list with references to the existing streams */
vector<stream*> stream::list_connections()
{
	vector<stream*> conns;

	for (maptype::iterator it = connections.begin(); it != connections.end(); it++)
	{
		conns.push_back(&(it->second));
	}

	return conns;
}



void stream::register_sent(uint32_t start, uint32_t end, const timeval& ts)
{
}



void stream::register_ack(uint32_t ackno, const timeval& ts)
{
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

	return *this;
}



string stream::id()
{
	union
	{
		uint32_t addr;
		uint8_t str[4];
	} info;

	std::ostringstream connstr;

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
