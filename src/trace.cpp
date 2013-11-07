#include "trace.h"
#include "flow.h"
#include <stdexcept>
#include <string>
#include <pcap.h>
#include <tr1/cstdint>
#include <arpa/inet.h>
#include <cstdio>
#include <assert.h>


using std::string;



static void set_filter(pcap_t* handle, const char* filter)
{
	bpf_program prog_code;

	if (pcap_compile(handle, &prog_code, filter, 0, PCAP_NETMASK_UNKNOWN) == -1)
	{
		throw std::runtime_error(string(pcap_geterr(handle)));
	}

	if (pcap_setfilter(handle, &prog_code) == -1)
	{
		pcap_freecode(&prog_code);
		throw std::runtime_error(string(pcap_geterr(handle)));
	}

	pcap_freecode(&prog_code);
}



static void process_sent(pcap_t* handle)
{
	pcap_pkthdr* hdr;
	const u_char* pkt;

	flowdata* data;
	const flow* conn;

	while (pcap_next_ex(handle, &hdr, &pkt) == 1)
	{
		// Find offset to TCP header and TCP payload
		uint32_t tcp_off = (*((uint8_t*) pkt + ETHERNET_FRAME_SIZE) & 0x0f) * 4; // IP header size = offset to IP payload/TCP header
		uint32_t data_off = ((*((uint8_t*) (pkt + ETHERNET_FRAME_SIZE + tcp_off + 12)) & 0xf0) >> 4) * 4; // TCP header size = offset to TCP payload

		// Find IP addresses and TCP ports
		uint32_t src_addr = *((uint32_t*) (pkt + ETHERNET_FRAME_SIZE + 12)); // source address
		uint32_t dst_addr = *((uint32_t*) (pkt + ETHERNET_FRAME_SIZE + 16)); // destination address
		uint16_t src_port = *((uint16_t*) (pkt + ETHERNET_FRAME_SIZE + tcp_off)); // source port
		uint16_t dst_port = *((uint16_t*) (pkt + ETHERNET_FRAME_SIZE + tcp_off + 2)); // destination port

		// Find TCP sequence number
		uint32_t seq_no = ntohl(*((uint32_t*) (pkt + ETHERNET_FRAME_SIZE + tcp_off + 4))); // TCP sequence number

		// Find TCP payload length
		uint16_t data_len = ntohs(*((uint16_t*) (pkt + ETHERNET_FRAME_SIZE + 2))) - tcp_off - data_off; // Ethernet frame size - total size of headers
		
		// Locate connection and update connection data
		flow::find_connection(conn, data, src_addr, src_port, dst_addr, dst_port);
		data->register_sent(seq_no, seq_no + data_len, hdr->ts);
	}
}




static void process_acks(pcap_t* handle)
{
	pcap_pkthdr* hdr;
	const u_char* pkt;

	flowdata* data;
	const flow* conn;

	while (pcap_next_ex(handle, &hdr, &pkt) == 1)
	{
		// Find offset to TCP header and TCP payload
		uint32_t tcp_off = (*((uint8_t*) pkt + ETHERNET_FRAME_SIZE) & 0x0f) * 4; // IP header size = offset to IP payload/TCP header

		// Find IP addresses and TCP ports
		uint32_t src_addr = *((uint32_t*) (pkt + ETHERNET_FRAME_SIZE + 12)); // source address
		uint32_t dst_addr = *((uint32_t*) (pkt + ETHERNET_FRAME_SIZE + 16)); // destination address
		uint16_t src_port = *((uint16_t*) (pkt + ETHERNET_FRAME_SIZE + tcp_off)); // source port
		uint16_t dst_port = *((uint16_t*) (pkt + ETHERNET_FRAME_SIZE + tcp_off + 2)); // destination port

		// Find TCP acknowledgement number
		uint32_t ack_no = ntohl(*((uint32_t*) (pkt + ETHERNET_FRAME_SIZE + tcp_off + 8))); // TCP acknowledgement number

		// Locate connection and update connection data
		flow::find_connection(conn, data, dst_addr, dst_port, src_addr, src_port);
		data->register_ack(ack_no, hdr->ts);
	}
}



void analyze_trace(FILE* fp, const filter& filter)
{
	string filterstr;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t* handle;

	// FIXME: Do a call to pcap_next_ex and find the first timestamp

	rewind(fp);
   	if ((handle = pcap_fopen_offline(fp, errbuf)) == NULL)
	{
		throw std::runtime_error(string(errbuf));
	}

	filterstr = filter.str();
	filterstr += " and tcp[tcpflags] & (tcp-syn|tcp-fin) = 0 and tcp[tcpflags] & (tcp-ack) != 0";
	set_filter(handle, filterstr.c_str());

	process_sent(handle);

	rewind(fp);
   	if ((handle = pcap_fopen_offline(fp, errbuf)) == NULL)
	{
		throw std::runtime_error(string(errbuf));
	}

	filterstr = filter.str();
	filterstr += " and tcp[tcpflags] & (tcp-syn|tcp-fin) = 0 and tcp[tcpflags] & (tcp-ack) != 0";
	set_filter(handle, filterstr.c_str());

	process_acks(handle);
}



string filter::str() const
{
	string str("tcp");

	return str;
}
