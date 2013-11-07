#ifndef __TRACE_H__
#define __TRACE_H__

#include <cstdio>
#include <tr1/cstdint>
#include <string>



/*
 * A wrapper class for creating a processing filter.
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



/*
 * Analyze the streams.
 */
void analyze_trace(FILE* trace_file, const filter& processing_filter);

#endif
