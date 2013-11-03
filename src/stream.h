#ifndef __STREAM_H__
#define __STREAM_H__

#include <tr1/cstdint>
#include <string>
#include "range.h"


class stream
{
	public:
		stream(uint32_t src_addr, uint32_t dst_addr, uint16_t src_port, uint16_t dst_port);

		stream(const stream& other)
		{
			*this = other;
		};

		void register_range(uint64_t seqno_start, uint64_t seqno_end);

		bool operator<(const stream& other);

		bool operator<(const stream& other) const
		{
			return const_cast<stream*>(this)->operator<(other);
		};

		stream& operator=(const stream& other);

		std::string id();

	private:
		/* Stream identifiers */
		uint32_t src;
		uint32_t dst;
		uint16_t sport;
		uint16_t dport;
};

#endif
