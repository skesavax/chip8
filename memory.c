#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "memory.h"

const size_t MEMORY_SIZE = 4096;
const size_t PROGRAM_START = 0x200;

const size_t FONTSET_ADDRESS = 0x000;
//sprite 4 pixcel width and 5 pixcel tall
static const uint8_t FONT_SET[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0 digit
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

static void load_fontset(uint8_t *mem) {
    memcpy(mem + FONTSET_ADDRESS, FONT_SET, 80);
}

static int load_program(uint8_t *mem, const uint8_t *program, size_t program_len) {
    size_t last_address = PROGRAM_START + program_len;

    if (last_address >= MEMORY_SIZE) {
        return 1; // error
    }

    memcpy(mem + PROGRAM_START, program, program_len);
    return 0;
}

int memory_new(Memory *m, const uint8_t *program, size_t program_len) {
    m->mem = malloc(MEMORY_SIZE);
    if (!m->mem) return 1;

    memset(m->mem, 0, MEMORY_SIZE);

    load_fontset(m->mem);

    if (load_program(m->mem, program, program_len) != 0) {
        free(m->mem);
        m->mem = NULL;
        return 1;
    }

    return 0;
}
