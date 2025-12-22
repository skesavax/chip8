#include "random_byte.h"
#include <stdlib.h>
#include <time.h>

void random_byte_init(RandomByte *rb) {
    (void)rb;               // unused, but keeps API structure similar to Rust
    srand((unsigned)time(NULL));   // seed RNG once
}

uint8_t random_byte_sample(RandomByte *rb) {
    (void)rb;
    return (uint8_t)(rand() % 256);    // random 0–255
}
