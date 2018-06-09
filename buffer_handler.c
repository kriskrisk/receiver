#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <zconf.h>
#include <malloc.h>
#include <memory.h>
#include <pthread.h>
#include "buffer_handler.h"
#include "receiver.h"
#include "transmitter_handler.h"

extern int num_of_transmitters;
extern transmitter_info **available_transmitters;
extern current_transmitter *my_transmitter;
extern size_t bsize;
extern pthread_mutex_t play_music_mutex;
extern pthread_cond_t almost_full;
extern uint64_t byte0;

void increment_pointer(void) {
    // If called from audio_to_stdout than in mutex
    if (my_transmitter->read_idx == my_transmitter->buffer_size) {
        my_transmitter->read_idx = 0;
    }

    my_transmitter->read_idx++;
}

uint64_t find_idx (uint64_t offset) {
    return (offset / my_transmitter->audio_size) % my_transmitter->buffer_size;
}

uint64_t decrement_pointer(uint64_t curr_pointer) {
    // If called from handle_retransmission_requests than already has mutex
    if (curr_pointer == 0) {
        return my_transmitter->buffer_size;
    }

    return curr_pointer - 1;
}

/*
void start_music(void) {
    destroy_my_transmitter();
    my_transmitter = (current_transmitter *)calloc(sizeof(current_transmitter), 1);

    if (available_transmitters != NULL) {
        // If called from handle_audio than in mutex
        my_transmitter->curr_transmitter_info = available_transmitters[0];
    }
}
*/

// Returns true if received new, greater session id
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

    if (my_transmitter->byte0 + (my_transmitter->buffer_size * 3 * audio_size) / 4 <= offset) {
        pthread_cond_signal(&almost_full);
    }

    if (session_id == my_transmitter->session_id) {

        uint64_t write_idx = find_idx(offset);
        destroy_audio(write_idx);
        my_transmitter->cyclic_buffer[write_idx] = new_audio;

        return false;
    } else if (session_id > my_transmitter->session_id) {
        // TODO: Handle this case
        //pthread_cond_wait(&almost_full, &play_music_mutex);

        return true;
    }

    free(new_audio);
    return false;
}
