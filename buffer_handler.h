#ifndef BUFFER_HANDLER_H
#define BUFFER_HANDLER_H

extern void increment_pointer(void);

extern uint64_t find_idx(uint64_t offset);

extern uint64_t decrement_pointer(uint64_t curr_pointer);

extern bool add_to_cyclic_buffer(ssize_t rcv_len, char *buffer);

#endif
