#ifndef RECEIVER_H
#define RECEIVER_H

#include "radio.h"

#define SLEEP_TIME 5
#define LOOKUP_SIZE 19
#define MAX_NUM_RETMIX 13105

typedef struct audio_package {
    uint64_t offset;
    char *audio;
    size_t audio_size;
} audio_package;

typedef struct transmitter_info {
    char name[MAX_NAME_SIZE];
    char *dotted_address;
    in_port_t remote_port;
    time_t last_answer;
} transmitter_info;

typedef struct current_transmitter {
    uint64_t session_id;
    audio_package **cyclic_buffer;
    size_t buffer_size;
    int read_idx;           // Next index to be read
    size_t audio_size;      // psize from transmitter
    transmitter_info *curr_transmitter_info;
    uint64_t last_received;
    uint64_t byte0;
} current_transmitter;


#endif
