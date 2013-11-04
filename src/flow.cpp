#include "flow.h"
#include <map>
#include <vector>
#include <string>
#include <tr1/cstdint>
#include <arpa/inet.h>
#include <sstream>

using std::vector;



/* A map over all connections */
static flow::flow_map connections;



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



bool flow::create_connection(uint32_t src, uint16_t sport, uint32_t dst, uint16_t dport, uint32_t seqno, const timeval& ts)
{
	flow key(src, dst, sport, dport);

	// Try to find flow in the connection map
	flow_map::iterator lower_bound = connections.lower_bound(key);

	if (lower_bound != connections.end() && !(connections.key_comp()(key, lower_bound->first)))
	{
		// Flow was found
		// Assume that it's data has already been set
		return false;
	}

	// Flow was not found, we have to create it
	connections.insert(lower_bound, flow_map::value_type(key, flowdata(seqno, ts)));
	return true;
}



flowdata* flow::find_connection(uint32_t src, uint16_t sport, uint32_t dst, uint16_t dport)
{
	flow_map::iterator found = connections.find(flow(src, sport, dst, dport));

	if (found != connections.end())
	{
		return &found->second;
	}

	return NULL;
}



int flow::list_connections(vector<const flow*> conns, vector<const flowdata*> fdata)
{
	int flows = 0;

	for (flow_map::iterator it = connections.begin(); it != connections.end(); it++)
	{
		fdata.push_back(&(it->second));
		conns.push_back(&(it->first));
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
