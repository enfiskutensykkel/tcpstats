#ifndef __FLOW_H__
#define __FLOW_H__

#include <tr1/cstdint>
#include <string>
#include <sys/time.h>
#include <vector>
#include <map>
#include <list>
#include "range.h"


/*
 * Macro to convert a struct timeval into a number of microseconds
 */
#define USECS(tv) (((uint64_t) tv.tv_sec) * 1000000 + ((uint64_t) tv.tv_usec))



class flowdata;


/* 
 * A flow object represents a one-way connection.
 * A TCP flow will have two corresponding flow objects, one per direction.
 */
class flow
{
	public:
		/* Retrieve a connection or create it if it doesn't exist */
		static bool find_connection(const flow*& flow, flowdata*& data, uint32_t src_addr, uint16_t src_port, uint32_t dst_addr, uint16_t dst_port);

		/* Get a list of all existing connections */
		static uint32_t list_connections(std::vector<const flow*>& connections, std::vector<const flowdata*>& data);

		/* 
		 * Human readable string identifying the flow.
		 * Example output: 10.0.0.1:8888=>10.0.0.2:9999
		 */
		std::string id();
		inline std::string id() const 
		{ 
			return const_cast<flow*>(this)->id(); 
		};

		/* Ctors, operators and const-correctness stuff */
		flow& operator=(const flow& other);
		flow(uint32_t src_addr, uint16_t src_port, uint32_t dst_addr, uint16_t dst_port);
		inline flow(const flow& other) 
		{ 
			*this = other; 
		};

		bool operator<(const flow& other);
		inline bool operator<(const flow& other) const 
		{ 
			return const_cast<flow*>(this)->operator<(other); 
		};

	private:
		/* Connection identifiers */
		uint32_t src;			// source IP address
		uint32_t dst;			// destination IP address
		uint16_t sport;			// source port
		uint16_t dport;			// destination port

		/* Map of existing connections  */
		typedef std::map< flow, flowdata > flow_map;
		static flow_map connections;
};



/*
 * A flowdata object holds information about a flow, including a map of all
 * byte ranges sent and acknowledged.
 */
class flowdata
{
	public:
		/* Register a sent byte range */
		void register_sent(uint32_t seqno_start, uint32_t seqno_end, const timeval& timestamp);

		/* Register an acknowledgement (ACK) */
		void register_ack(uint32_t ackno, const timeval& timestamp);

		/* Various statistics of raw data */
		uint32_t total_retrans() const;
		uint32_t max_num_retrans() const;
		uint32_t total_dupacks() const;
		uint32_t max_num_dupacks() const;
		uint64_t unique_bytes_sent() const;
		uint64_t rtt() const;
		uint64_t duration() const;

		/* Ctors, operators and const-correctness stuff */
		flowdata();

		inline flowdata(const flowdata& other)
		{
			*this = other;
		};

		flowdata& operator=(const flowdata& other);

	private:
		/* Flow properties */
		uint32_t abs_seqno_min;	// first absolute sequence number (used to handle seqno wrapping)
		uint32_t abs_seqno_max;	// latest absolute sequence number registered (seqno wrapping)
		uint64_t rel_seqno_max;	// latest relative sequence number registered (seqno wrapping)

		uint32_t abs_ackno_min;	// first absolute acknowledgement number (ackno wrapping)
		uint32_t abs_ackno_max;	// latest absolute acknowledgement number (ackno wrapping)
		uint64_t curr_ack;  	// the current highest acknowledged (relative) sequence number
		uint64_t prev_ack;  	// the previous highest acknowledged (relative) seqno

		timeval ts_first,		// flow duration (first registered segment, and last registered segment)
				ts_last;

		/* A map over byte ranges and data about them */
		typedef std::multimap< range, rangedata > range_map;
		range_map ranges;

		/* Helper methods to match and split ranges */
		typedef std::list< rangedata* > range_list;
		inline void find_and_split_ranges(range_list& list, const range& key, bool include_new_data);

		/* Data aggregated over intervals/time slices */
		std::vector<uint64_t> throughput;
		std::vector<uint64_t> goodput;
		std::vector<uint64_t> latency;
		std::vector<uint64_t> loss;
};

#endif
