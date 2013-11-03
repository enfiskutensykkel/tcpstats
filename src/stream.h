#ifndef __STREAM_H__
#define __STREAM_H__

#include <tr1/cstdint>
#include <string>
#include <sys/time.h>
#include <vector>
#include <map>
#include "range.h"


/* 
 * A stream represents a one-way connection.
 * A TCP flow will have two corresponding stream objects, one per direction.
 */
class stream
{
	public:
		/*
		 * Create a stream object.
		 */
		static bool create_connection(uint32_t src_addr, uint16_t src_port, uint32_t dst_addr, uint16_t dst_port, uint32_t first_seqno, const timeval& first_timestamp);

		/*
		 * Retrieve a stream object.
		 */
		static stream* find_connection(uint32_t src_addr, uint16_t src_port, uint32_t dst_addr, uint16_t dst_port);

		/*
		 * Get all stream objects.
		 */
		static std::vector<stream*> list_connections();

		/*
		 * Register a sent byte range.
		 */
		void register_sent(uint32_t seqno_start, uint32_t seqno_end, const timeval& timestamp);

		/*
		 * Register an acknowledgement.
		 */
		void register_ack(uint32_t ackno, const timeval& timestamp);

		/* 
		 * Human readable string identifying the flow.
		 * Example output: 10.0.0.1:8888=>10.0.0.2:9999
		 */
		std::string id();


		/* Constructors, overloads for comparison operators and const-correctness stuff*/
		stream(const stream& other)
		{
			*this = other;
		};

		stream& operator=(const stream& other);

		std::string id() const
		{
			return const_cast<stream*>(this)->id();
		};

	private:
		stream(uint32_t src_addr, uint16_t src_port, uint32_t dst_addr, uint16_t dst_port);

		/* Stream identifiers */
		uint32_t src;			// source IP address
		uint32_t dst;			// destination IP address
		uint16_t sport;			// source port
		uint16_t dport;			// destination port

		/* Stream properties */
		uint32_t first_seqno;	// first sequence number (used to handle sequence number wrapping)
		uint64_t rtt;			// the lowest registered delay (which we assume to be the RTT)
		timeval first_segment,	// stream duration (first registered segment, and last registered segment)
				last_segment;
		uint32_t highest_ackd;	// the highest acknowledgement number received

		// A map over byte ranges
		std::multimap<range,rangedata> ranges;

		/* Data aggregated over intervals/time slices */
		std::vector<uint64_t> throughput;
		std::vector<uint64_t> goodput;
		std::vector<uint64_t> latency;
		std::vector<uint64_t> loss;
};

#endif
