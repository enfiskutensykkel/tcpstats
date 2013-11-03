#include <pcap.h>
#include <arpa/inet.h>
#include <cstdio>
#include <vector>
#include "stream.h"


void analyze_streams(pcap_t* handle)
{
	pcap_pkthdr* hdr;
	const u_char* pkt;

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

		// Find TCP sequence numbers
		uint32_t seq_no = ntohl(*((uint32_t*) (pkt + ETHERNET_FRAME_SIZE + tcp_off + 4))); // TCP sequence number
		uint32_t ack_no = ntohl(*((uint32_t*) (pkt + ETHERNET_FRAME_SIZE + tcp_off + 8))); // TCP acknowledgement number

		// Find TCP payload length
		uint16_t data_len = ntohs(*((uint16_t*) (pkt + ETHERNET_FRAME_SIZE + 2))) - tcp_off - data_off; // Ethernet frame size - total size of headers

		// Process data
		stream* sender = stream::find_connection(src_addr, src_port, dst_addr, dst_port);
		sender->register_sent(seq_no, data_len, hdr->ts);
		stream* receiver = stream::find_connection(dst_addr, dst_port, src_addr, src_port);
		receiver->register_ack(ack_no, hdr->ts);
	}
}



void initialize_streams(pcap_t* handle)
{
	pcap_pkthdr* hdr;
	const u_char* pkt;

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

		// Create and initialize stream object
		stream::create_connection(src_addr, src_port, dst_addr, dst_port, seq_no, hdr->ts);
	}
}



int set_filter(pcap_t* handle, const char* filter)
{
	bpf_program prog_code;

	if (pcap_compile(handle, &prog_code, filter, 0, PCAP_NETMASK_UNKNOWN) == -1)
	{
		return -1;
	}

	if (pcap_setfilter(handle, &prog_code) == -1)
	{
		pcap_freecode(&prog_code);
		return -2;
	}

	pcap_freecode(&prog_code);
	return 0;
}



int main(int argc, char** argv)
{
	using std::vector;

	char errbuf[PCAP_ERRBUF_SIZE];

	pcap_t* handle = pcap_open_offline(argv[1], errbuf);
	set_filter(handle, "tcp and (tcp[tcpflags] & tcp-syn != 0)");
	initialize_streams(handle);
	pcap_close(handle);

	vector<stream*> streams = stream::list_connections();
	for (vector<stream*>::iterator i = streams.begin(); i != streams.end(); i++)
	{
		printf("%s\n", (*i)->id().c_str());
	}

	handle = pcap_open_offline(argv[1], errbuf);
	set_filter(handle, "tcp and (tcp[tcpflags] & (tcp-syn|tcp-fin) == 0)");
	analyze_streams(handle);
	pcap_close(handle);

	return 0;
}
