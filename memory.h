#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *mem;
} Memory;
int memory_new(Memory *m, const uint8_t *program, size_t program_len);
#endif
