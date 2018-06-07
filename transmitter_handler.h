#ifndef TRANSMITTER_HANDLER_H
#define TRANSMITTER_HANDLER_H

#include <stdbool.h>
#include <stdint.h>

#include "receiver.h"

extern void destroy_transmitter (transmitter_info *transmitter);
extern transmitter_info *create_transmitter(const char *buffer);
extern void add_transmitter(transmitter_info *new_transmitter);
extern bool exists(transmitter_info *new_transmitter);
extern void destroy_audio(uint64_t buffer_idx);
extern void destroy_my_transmitter(void);
extern void create_my_transmitter (ssize_t rcv_len, const char *buffer);

#endif
