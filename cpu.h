#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stddef.h>

/* External dependencies (assumed provided elsewhere in your project) */
#include "memory.h"   // Memory { uint8_t *mem; }
#include "timer.h"    // Timer { uint8_t delay_timer; uint8_t sound_timer; } + timer_init/update
#include "vmemory.h"  // VMemory + vmemory_clear + vmemory_draw_sprite_no_wrap
#include "random_byte.h" // RandomByte + random_byte_sample + random_byte_init

#define STACK_SIZE 16
#define V_REG_COUNT 16

typedef struct {
    /* If draw_pixels is NULL -> no draw update.
       Otherwise points to a framebuffer of size SCREEN_WIDTH*SCREEN_HEIGHT. */
    uint8_t *draw_pixels;
} EmulatorState;

struct Cpu {
    uint16_t i;
    uint16_t pc;
    uint16_t sp;
    uint16_t stack[STACK_SIZE];
    uint8_t v[V_REG_COUNT];

    Memory memory;
    Timer timer;
    VMemory vmemory;

    RandomByte rng;
};
/* Opaque cpu struct; user may store pointer to Cpu returned by cpu_new */
typedef struct Cpu Cpu;

/* Construct / destroy */
Cpu * cpu_new(Memory *memory, Timer *timer, VMemory *vmemory, RandomByte *rng);
void cpu_free(Cpu *cpu);

/* Execute one CPU cycle. 'input' must be an array of 16 uint8_t values (0 or 1).
 * On success returns 0 and fills 'out' (out->draw_pixels == NULL if nothing to draw).
 * On error returns non-zero and out content is unspecified.
 */
int cpu_cycle(Cpu *cpu, const uint8_t input[16], EmulatorState *out);

/* Update timers (to be called at 60Hz). Returns 1 if sound timer caused a beep, 0 otherwise. */
int cpu_update_timers(Cpu *cpu);

#endif
