#ifndef __PCAP_H__
#define __PCAP_H__

#include <pcap.h>

/*
 * Initialize the streams.
 * Returns the number of streams found, or -1 failure.
 */
int initialize_streams(pcap_t* handle);


/*
 * Analyze the streams.
 * Returns 0 on success, or -1 on failure.
 */
int analyze_streams(pcap_t* handle);

#endif
