#include "trace.h"
#include "flow.h"
#include <pcap.h>
#include <tr1/cstdint>
#include <arpa/inet.h>



void set_filter(pcap_t* handle, const filter& filter)
{
	bpf_program prog_code;

	// TODO
	if (pcap_compile(handle, &prog_code, "", 0, PCAP_NETMASK_UNKNOWN) == -1)
	{
		//return -1;
	}

	if (pcap_setfilter(handle, &prog_code) == -1)
	{
		pcap_freecode(&prog_code);
		//return -2;
	}

	pcap_freecode(&prog_code);
	//return 0;
}



void analyze_trace(pcap_t* handle)
{
	pcap_pkthdr* hdr;
	const u_char* pkt;
	flowdata* data;

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

		// TODO: Check flags, if SYN or FIN don't call register sent
		// TODO: Check flags, if SYN create connection

		// Locate connection data and update it
		data = flow::find_connection(src_addr, src_port, dst_addr, dst_port);
		data->register_sent(seq_no, seq_no + data_len, hdr->ts);

		data = flow::find_connection(dst_addr, dst_port, src_addr, src_port);
		data->register_ack(ack_no, hdr->ts);
	}
}

