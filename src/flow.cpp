#include "flow.h"
#include <map>
#include <vector>
#include <string>
#include <tr1/cstdint>
#include <arpa/inet.h>
#include <sstream>

using std::vector;


/* A map of all connections */
flow::flow_map flow::connections;



flow::flow(uint32_t src, uint16_t sport, uint32_t dst, uint16_t dport)
	: src(src), dst(dst), sport(sport), dport(dport)
{
}



bool flow::operator<(const flow& rhs)
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



flow& flow::operator=(const flow& rhs)
{
	src = rhs.src;
	dst = rhs.dst;
	sport = rhs.sport;
	dport = rhs.dport;

	return *this;
}



bool flow::find_connection(const flow*& conn, flowdata*& data, uint32_t src, uint16_t sport, uint32_t dst, uint16_t dport)
{
	flow key(src, sport, dst, dport);

	// Try to find flow in the connection map
	flow_map::iterator f = connections.lower_bound(key);

	if (f != connections.end() && !(connections.key_comp()(key, f->first)))
	{
		// Flow was found
		conn = &f->first;
		data = &f->second;
		return false;
	}

	// Flow was not found, we have to create it
	f = connections.insert(f, flow_map::value_type(key, flowdata()));
	conn = &f->first;
	data = &f->second;
	return true;
}



uint32_t flow::list_connections(vector<const flow*>& conns, vector<const flowdata*>& fdata)
{
	uint32_t flows = 0;

	for (flow_map::iterator it = connections.begin(); it != connections.end(); it++)
	{
		fdata.push_back(&it->second);
		conns.push_back(&it->first);
		++flows;
	}

	return flows;
}



std::string flow::id()
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
