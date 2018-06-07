#ifndef RADIO_H
#define RADIO_H

#include <stdint.h>

#define DISCOVER_ADDR "255.255.255.255"
#define DATA_PORT 21240
#define CTRL_PORT 31240
#define UI_PORT 11240
#define PSIZE 512
#define BSIZE 65536
#define FSIZE 131072
#define RTIME 250
#define NAME "Nienazwany Nadajnik"
#define MAX_UDP_SIZE 65536
#define MAX_NAME_SIZE 64
#define MAX_DOTTEN_ADDRESS_SIZE 15
#define REPLY_SIZE 100
#define TTL_VALUE 64

extern int setup_sender(char *remote_dotted_address, in_port_t remote_port);
extern int setup_receiver (char *multicast_dotted_address, in_port_t local_port);

#endif

