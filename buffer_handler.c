#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <zconf.h>
#include <malloc.h>
#include <memory.h>
#include "buffer_handler.h"
#include "receiver.h"
#include "transmitter_handler.h"

extern int num_of_transmitters;
extern transmitter_info **available_transmitters;
extern current_transmitter *my_transmitter;
extern size_t bsize;

extern pthread_mutex_t missing_mutex;
extern pthread_mutex_t audio_data_mutex;

void increment_pointer(void) {
    // TODO: Mutex
    if (my_transmitter->read_idx == my_transmitter->buffer_size) {
        my_transmitter->read_idx = 0;
    }

    my_transmitter->read_idx++;
}

uint64_t find_idx (uint64_t offset) {
    return (offset / my_transmitter->audio_size) % my_transmitter->buffer_size;
}

uint64_t decrease_pointer(uint64_t curr_pointer) {
    if (curr_pointer == 0) {
        return my_transmitter->buffer_size;
    }

    return --curr_pointer;
}

void start_music(void) {
    destroy_my_transmitter();
    my_transmitter = (current_transmitter *)calloc(sizeof(current_transmitter), 1);

    if (available_transmitters != NULL) {
        // TODO: Mutex
        my_transmitter->curr_transmitter_info = available_transmitters[0];
    }
}

// Returns true if recieved new, greater session id
bool add_to_cyclic_buffer(ssize_t rcv_len, char *buffer) {
    if (rcv_len <= 2 * sizeof(uint64_t)) {
        return false;
    }

    uint64_t session_id = be64toh(*(uint64_t *) buffer);
    uint64_t offset = be64toh(*(uint64_t *) (buffer + sizeof(uint64_t)));

    audio_package *new_audio = (audio_package *) malloc(sizeof(audio_package));
    size_t audio_size = rcv_len - 2 * sizeof(uint64_t);

    if (audio_size != my_transmitter->audio_size) {
        return false;
    }

    char *audio = malloc(audio_size);

    memcpy(audio, buffer + 2 * sizeof(uint64_t), audio_size);

    new_audio->audio = audio;
    new_audio->audio_size = audio_size;
    new_audio->offset = offset;

    if (session_id == my_transmitter->session_id) {

        uint64_t write_idx = find_idx(offset);
        destroy_audio(write_idx);
        my_transmitter->cyclic_buffer[write_idx] = new_audio;
        return false;
    } else if (session_id > my_transmitter->session_id) {
        start_music();
        return true;
    }
}
