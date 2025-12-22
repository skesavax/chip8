#ifndef RANDOM_BYTE_H
#define RANDOM_BYTE_H

#include <stdint.h>

typedef struct {
    // nothing special needed, rand() is global
    int dummy;
} RandomByte;

void random_byte_init(RandomByte *rb);
uint8_t random_byte_sample(RandomByte *rb);

#endif
