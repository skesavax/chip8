#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "display.h" // For ColorTheme

typedef struct {
    const char* program_filename;
    ColorTheme theme;
    uint32_t scale;
    uint64_t cpu_clock;
    bool muted;
} Config;

#define DEFAULT_CPU_CLOCK 600
#endif // CONFIG_H