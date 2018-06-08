#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <inttypes.h>

#include "err.h"
#include "radio.h"
#include "receiver.h"
#include "transmitter_handler.h"
#include "buffer_handler.h"

current_transmitter *my_transmitter = NULL;

char *discover_addr = DISCOVER_ADDR;
uint16_t data_port = DATA_PORT;
uint16_t ctrl_port = CTRL_PORT;
size_t bsize = BSIZE;
uint rtime = RTIME;

pthread_mutex_t my_transmitter_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t available_transmitters_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t play_music_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t almost_full = PTHREAD_COND_INITIALIZER;

// TODO: Remove not responding transmitters
transmitter_info **available_transmitters = NULL;
int num_of_transmitters = 0;

uint64_t next_to_play;
uint64_t byte0;

static void *handle_control_write(void *args) {
    char buffer[LOOKUP_SIZE];
    int sock = setup_sender(discover_addr, ctrl_port);

    while (true) {
        strncpy(buffer, "ZERO_SEVEN_COME_IN\n", LOOKUP_SIZE);
        if (write(sock, buffer, LOOKUP_SIZE) != LOOKUP_SIZE)
            syserr("write");
        sleep(SLEEP_TIME);
    }
}

static void *handle_control_read(void *args) {
    ssize_t rcv_len;
    int sock = setup_receiver(discover_addr, ctrl_port);
    char r_buffer[REPLY_SIZE];

    while (true) {
        rcv_len = read(sock, r_buffer, REPLY_SIZE);
        if (rcv_len < 0)
            syserr("read");

        transmitter_info *new_transmitter = create_transmitter(r_buffer);

        pthread_mutex_lock(&my_transmitter_mutex);

        if (exists(new_transmitter)) {
            destroy_transmitter(new_transmitter);
        } else {
            add_transmitter(new_transmitter);
        }

        pthread_mutex_unlock(&my_transmitter_mutex);
    }
}

static void *handle_retransmission_requests(void *args) {
    pthread_mutex_lock(&my_transmitter_mutex);

    while (true) {
        sleep(rtime);

        uint64_t start_idx = find_idx(my_transmitter->last_received);
        uint64_t offset_counter = my_transmitter->last_received;
        char missing_packets[MAX_NUM_RETMIX];
        uint64_t num_of_packets = 0;

        int sum_num_read = 0;
        int num_read = sprintf(missing_packets, "LOUDER_PLEASE 512,1024,1536,5632,3584");
        if (num_read < 0) {
            fprintf(stderr, "Error in sprintf: %d (%s)\n", num_read, strerror(num_read));
            exit(EXIT_FAILURE);
        }
        sum_num_read += num_read;

        for (uint64_t i = start_idx; i == start_idx; i = decrement_pointer(i)) {
            offset_counter -= my_transmitter->audio_size;

            if (my_transmitter->cyclic_buffer[i] == NULL && offset_counter >= 0) {
                num_of_packets++;

                if (num_of_packets > MAX_NUM_RETMIX) {
                    break;
                }

                num_read = sprintf(missing_packets, "%" PRIu64 ",", offset_counter);
                if (num_read < 0) {
                    fprintf(stderr, "Error in sprintf: %d (%s)\n", num_read, strerror(num_read));
                    exit(EXIT_FAILURE);
                }
                sum_num_read += num_read;
            }
        }

        missing_packets[sum_num_read - 1] = '\n';

        pthread_mutex_unlock(&my_transmitter_mutex);
    }
}

static void *handle_audio(void *args) {
    ssize_t rcv_len;
    transmitter_info *chosen_transmitter;

    while (true) {
        pthread_mutex_lock(&available_transmitters_mutex);
        if (available_transmitters != NULL) {
            chosen_transmitter = available_transmitters[0];
            break;
        }
        pthread_mutex_unlock(&available_transmitters_mutex);

        sleep(1);
    }

    int sock = setup_receiver(chosen_transmitter->dotted_address, chosen_transmitter->remote_port);
    char *buffer = malloc(bsize);
    bool isNew = true;

    while (true) {
        rcv_len = read(sock, buffer, bsize);
        if (rcv_len < 0)
            syserr("read");

        if (isNew) {
            pthread_mutex_lock(&my_transmitter_mutex);
            create_my_transmitter(rcv_len, buffer);
            pthread_mutex_unlock(&my_transmitter_mutex);
        }

        pthread_mutex_lock(&my_transmitter_mutex);

        if ((isNew = add_to_cyclic_buffer(rcv_len, buffer))) {
            if (close(sock) < 0) {
                syserr("closing sock");
            }

            sock = setup_receiver(my_transmitter->curr_transmitter_info->dotted_address,
                                  my_transmitter->curr_transmitter_info->remote_port);
        }

        pthread_mutex_unlock(&my_transmitter_mutex);
    }
}

/* Wait until buffer will be almost full and write to stdout */
static void *audio_to_stdout(void *args) {
    while (true) {
        pthread_mutex_lock(&my_transmitter_mutex);
        if (next_to_play != my_transmitter->cyclic_buffer[my_transmitter->read_idx]->offset) {
            start_music();
        }
        pthread_mutex_unlock(&my_transmitter_mutex);

        pthread_mutex_lock(&play_music_mutex);
        pthread_mutex_lock(&my_transmitter_mutex);

        if (next_to_play != my_transmitter->cyclic_buffer[my_transmitter->read_idx]->offset) {

        }

        if (write(STDOUT_FILENO, my_transmitter->cyclic_buffer[my_transmitter->read_idx],
                  my_transmitter->audio_size) != my_transmitter->audio_size)
            syserr("write to stdout");

        increment_pointer();
        next_to_play += my_transmitter->audio_size;

        pthread_mutex_unlock(&my_transmitter_mutex);
        pthread_mutex_unlock(&play_music_mutex);
    }
}

int main(int argc, char **argv) {
    int opt;

    while ((opt = getopt(argc, argv, "d:P:C:U:b:R:")) != -1) {
        switch (opt) {
            case 'd':
                discover_addr = optarg;
                break;
            case 'P':
                data_port = (uint16_t) atoi(optarg);
                break;
            case 'C':
                ctrl_port = (uint16_t) atoi(optarg);
                break;
            case 'U':
                ctrl_port = (uint16_t) atoi(optarg);
                break;
            case 'b':
                bsize = (size_t) atoi(optarg);
                break;
            case 'R':
                rtime = (uint) atoi(optarg);
                break;
        }
    }

    available_transmitters = NULL;
    //missing_packets =

    int control_protocol_write, control_protocol_read, retransmission, audio, audio_write;
    pthread_t control_protocol_write_thread, control_protocol_read_thread, retransmission_thread, audio_data_thread, audio_write_data_thread;

    /* pthread create */
    control_protocol_write = pthread_create(&control_protocol_write_thread, 0, handle_control_write, NULL);
    if (control_protocol_write == -1) {
        syserr("control protocol: pthread_create");
    }

    control_protocol_read = pthread_create(&control_protocol_read_thread, 0, handle_control_read, NULL);
    if (control_protocol_read == -1) {
        syserr("control protocol: pthread_create");
    }

    /*
    retransmission = pthread_create(&retransmission_thread, 0, handle_retransmission_requests, NULL);
    if (control_protocol_read == -1) {
        syserr("control protocol: pthread_create");
    }
    */

    audio = pthread_create(&audio_data_thread, 0, handle_audio, NULL);
    if (audio == -1) {
        syserr("audio: pthread_create");
    }

    audio_write = pthread_create(&audio_write_data_thread, 0, audio_to_stdout, NULL);
    if (audio_write == -1) {
        syserr("audio: pthread_create");
    }

    /* pthread join */
    int err = pthread_join(audio_data_thread, NULL);
    if (err != 0) {
        fprintf(stderr, "Error in pthread_join: %d (%s)\n", err, strerror(err));
        exit(EXIT_FAILURE);
    }

    // TODO: Check what to do with the rest
}
