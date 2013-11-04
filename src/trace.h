#ifndef __TRACE_H__
#define __TRACE_H__

#include <pcap.h>
#include <tr1/cstdint>
#include <string>


class filter;



/*
 * Set a BPF processing filter to reduce the data set.
 */
void set_filter(pcap_t* handle, const filter& filter);



/*
 * Analyze the streams.
 */
void analyze_trace(pcap_t* handle);



/*
 * A wrapper for creating a pcap BPF processing filter.
 */
struct filter
{
	uint32_t src_addr;
	uint32_t dst_addr;
	uint16_t src_port_start;
	uint16_t src_port_end;
	uint16_t dst_port_start;
	uint16_t dst_port_end;

	std::string str() const;
};

#endif
