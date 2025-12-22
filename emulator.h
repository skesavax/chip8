#ifndef EMULATOR_H
#define EMULATOR_H

#include <stdint.h>
#include <stdbool.h>

#include "display.h"   // DisplayHandler
#include "input.h"     // InputHandler, InputEvent
#include "sound.h"     // SoundHandler
#include "memory.h"    // Memory
#include "timer.h"     // Timer
#include "vmemory.h"   // VMemory
#include "random_byte.h" // RandomByte
#include "cpu.h"       // Cpu

#define DEFAULT_CPU_CLOCK 600ULL

typedef struct {
    const char *program_filename;
    int theme;      // matches your ColorTheme representation in C
    uint32_t scale;
    uint64_t cpu_clock;
    bool muted;
} Config;

typedef struct {
    bool draw_flag;     // true when display should update
    uint8_t *pixels;    // pointer to 64×32 display buffer (optional)
} DrawResult;

/* Parse string to cpu clock. Returns parsed value on success,
 * or 0 on failure. Caller may treat 0 as invalid. */
uint64_t cpu_clock_from_str(const char *s);

/* Runs the emulator. Returns 0 on success, non-zero on error. */
int emulate_chip8(const Config *config);

#endif
