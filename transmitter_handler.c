#include "transmitter_handler.h"
#include "receiver.h"

static void destroy_transmitter (transmitter_info *transmitter) {
    free(transmitter);
}

static transmitter_info *create_transmitter(const char *buffer) {
    transmitter_info *new_transmitter = (transmitter_info *) malloc(sizeof(transmitter_info));
    char *new_name = (char *) malloc(MAX_NAME_SIZE);
    char *dotted_address = (char *) malloc(MAX_DOTTEN_ADDRESS_SIZE);

    size_t start = 13;

    size_t middle1 = 14;
    while (buffer[middle1] != ' ') {
        middle1++;
    }

    size_t middle2 = middle1 + 1;
    while (buffer[middle2] != ' ') {
        middle2++;
    }

    size_t end = middle2;
    while (buffer[end] != '\n') {
        end++;
    }

    strncpy(new_name, buffer + middle2 + 1, end - middle2);
    strncpy(dotted_address, buffer + start + 1, middle1 - start);

    new_transmitter->dotted_address = dotted_address;
    new_transmitter->remote_port = (in_port_t) strtol(buffer + middle1 + 1, NULL, 10);
    new_transmitter->last_answer = time(NULL);

    return new_transmitter;
}

static void add_transmitter(transmitter_info *new_transmitter) {
    // TODO: Mutex
    num_of_transmitters++;
    available_transmitters = realloc(available_transmitters, num_of_transmitters * sizeof(transmitter_info *));
    available_transmitters[num_of_transmitters - 1] = new_transmitter;
}

static bool exists(transmitter_info *new_transmitter) {
    for (int i = 0; i < num_of_transmitters; i++) {
        if (strncmp(available_transmitters[i]->name, new_transmitter->name, MAX_NAME_SIZE) == 0);
    }
}

static void destroy_audio(uint64_t buffer_idx) {
    free(my_transmitter->cyclic_buffer[buffer_idx]->audio);
    free(my_transmitter->cyclic_buffer[buffer_idx]);
}

static void destroy_my_transmitter(void) {
    for (uint64_t i = 0; i < my_transmitter->buffer_size; i ++) {
        destroy_audio(i);
    }

    free(my_transmitter);
}
