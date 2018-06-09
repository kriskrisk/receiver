#include <malloc.h>
#include <memory.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "transmitter_handler.h"
#include "receiver.h"
#include "radio.h"

extern int num_of_transmitters;
extern transmitter_info **available_transmitters;
extern current_transmitter *my_transmitter;
extern size_t bsize;
extern uint64_t next_to_play;
extern pthread_mutex_t my_transmitter_mutex;
extern pthread_cond_t not_empty_transmitters;

/* TRANSMITTER */

void destroy_transmitter(transmitter_info *transmitter) {
    free(transmitter->dotted_address);
    free(transmitter);
}

bool validate_lookup_message(const char *protocol_msg) {      // protocol_msg is allocated to MAX_UDP_SIZE
    char dummy[MAX_NAME_SIZE];
    int a, b, c, d, e;

    if (sscanf(protocol_msg, "BOREWICZ_HERE %d.%d.%d.%d %d %s\n", &a, &b, &c, &d, &e, dummy) == 6) {
        return true;
    }

    return false;
}

transmitter_info *create_transmitter(const char *buffer) {
    if (validate_lookup_message(buffer)) {
        transmitter_info *new_transmitter = (transmitter_info *) malloc(sizeof(transmitter_info));
        char *dotted_address = (char *) malloc(MAX_DOTTEN_ADDRESS_SIZE);
        int port_number;

        sscanf(buffer, "BOREWICZ_HERE %s %d %s\n", dotted_address, &port_number, new_transmitter->name);

        new_transmitter->dotted_address = dotted_address;
        new_transmitter->remote_port = (in_port_t) port_number;
        new_transmitter->last_answer = time(NULL);

        return new_transmitter;
    }

    return NULL;
}

void add_transmitter(transmitter_info *new_transmitter) {
    // If called from handle_control_read than already has mutex
    num_of_transmitters++;
    available_transmitters = realloc(available_transmitters, num_of_transmitters * sizeof(transmitter_info *));
    available_transmitters[num_of_transmitters - 1] = new_transmitter;
}

bool exists(transmitter_info *new_transmitter) {
    for (int i = 0; i < num_of_transmitters; i++) {
        if (strncmp(available_transmitters[i]->name, new_transmitter->name, MAX_NAME_SIZE) == 0);
    }
}

void destroy_audio(uint64_t buffer_idx) {
    if (my_transmitter->cyclic_buffer[buffer_idx] != NULL) {
        free(my_transmitter->cyclic_buffer[buffer_idx]->audio);
        free(my_transmitter->cyclic_buffer[buffer_idx]);
    }
}

/* MY_TRANSMITTER */

/* I assume that this thread has my_transmitter_mutex */
/* Only called when my_transmitter = NULL */
void choose_my_transmitter(void) {
    if (available_transmitters == NULL) {
        pthread_cond_wait(&not_empty_transmitters, &my_transmitter_mutex);
    }

    my_transmitter = (current_transmitter *)malloc(sizeof(my_transmitter));
    my_transmitter->curr_transmitter_info = available_transmitters[0];
}

void destroy_my_transmitter(void) {
    if (my_transmitter != NULL) {
        for (uint64_t i = 0; i < my_transmitter->buffer_size; i++) {
            destroy_audio(i);
        }

        free(my_transmitter);
    }
}

void create_my_transmitter(ssize_t rcv_len, const char *buffer) {
    if (rcv_len <= 2 * sizeof(uint64_t)) {
        return;
    }

    size_t audio_size = rcv_len - 2 * sizeof(uint64_t);     // psize

    my_transmitter->read_idx = 0;
    my_transmitter->audio_size = audio_size;
    my_transmitter->session_id = be64toh(*(uint64_t *) buffer);
    my_transmitter->buffer_size = bsize / audio_size;
    my_transmitter->last_received = be64toh(*(uint64_t *) (buffer + sizeof(uint64_t)));
    my_transmitter->byte0 = my_transmitter->last_received;

    next_to_play = my_transmitter->last_received;
}
