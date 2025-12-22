#include "vmemory.h"
#include <string.h>

void vmemory_init(VMemory *vm) {
    memset(vm->buffer, 0, sizeof(vm->buffer));
    vm->draw_flag = true;
}

void vmemory_clear(VMemory *vm) {
    memset(vm->buffer, 0, sizeof(vm->buffer));
    vm->draw_flag = true;
}

uint8_t vmemory_draw_sprite_no_wrap(VMemory *vm, uint8_t x_pos, uint8_t y_pos, const uint8_t *sprite, int sprite_height)
{
    vm->draw_flag = true;

    size_t x, curr_y;
    normalize_coordinates(x_pos, y_pos, &x, &curr_y);

    uint8_t vf = 0;

    for (int row = 0; row < sprite_height; row++) {
        if (curr_y >= SCREEN_HEIGHT) {
            return vf;
        }

        uint8_t byte = sprite[row];
        size_t curr_x = x;
        uint8_t mask = 0x80;   // 1000 0000

        for (int n = 0; n < 8; n++) {
            if (curr_x >= SCREEN_WIDTH) {
                break;
            }

            uint8_t new_pixel = (byte & mask) >> (7 - n);
            uint8_t old_pixel = vm->buffer[idx(curr_x, curr_y)];
            vm->buffer[idx(curr_x, curr_y)] = old_pixel ^ new_pixel;

            vf |= (new_pixel & old_pixel);

            mask >>= 1;
            curr_x++;
        }

        curr_y++;
    }

    return vf;
}
