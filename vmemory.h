#ifndef VMEMORY_H
#define VMEMORY_H

#include <stdint.h>
#include <stdbool.h>

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

typedef struct {
    uint8_t buffer[SCREEN_WIDTH * SCREEN_HEIGHT];//screen buffer /video memory
    bool draw_flag; //Tells emulator screen change- redraw on next frame
} VMemory;

void vmemory_init(VMemory *vm);
void vmemory_clear(VMemory *vm);
uint8_t vmemory_draw_sprite_no_wrap(VMemory *vm, uint8_t x_pos, uint8_t y_pos, const uint8_t *sprite, int sprite_height);

// Helper functions
static inline size_t idx(size_t x, size_t y) {
    return y * SCREEN_WIDTH + x;
}

static inline void normalize_coordinates(uint8_t x, uint8_t y, size_t *nx, size_t *ny) {
    *nx = x % SCREEN_WIDTH;//start of X
    *ny = y % SCREEN_HEIGHT;//start of Y
}

#endif
