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
/// @brief 
/// @param vm 
/// @param x_pos : x cordinate (Vx)
/// @param y_pos : y cordinate (Vy)
/// @param sprite : pointer to sprite buffer
/// @param sprite_height : number of rows
/// @return : collision flag VF
uint8_t vmemory_draw_sprite_no_wrap(VMemory *vm, uint8_t x_pos, uint8_t y_pos, const uint8_t *sprite, int sprite_height)
{
    /*
        sprite_height: sprint hight to draw
        Each sprite is 8 pixel width, each sprite row = 1 byte = 8 pixcels
        Use xor drawing and detect collisions(if any pixcel is turned off during drawing, set VF=1)
        return VF (collision flag)
    */
    vm->draw_flag = true;

    size_t x, curr_y;
    normalize_coordinates(x_pos, y_pos, &x, &curr_y);//safe screen indices

    uint8_t vf = 0;//will become 1 if collection happen

    for (int row = 0; row < sprite_height; row++) {//each iteration draw horizontal row of the sprite
        if (curr_y >= SCREEN_HEIGHT) {//stop if it go beyond screen bottom, this is no wrap behaviour
            return vf;
        }

        uint8_t byte = sprite[row];//read 1 sprite row (8 pixels)
        size_t curr_x = x;
        uint8_t mask = 0x80;   // 1000 0000, start with left most pixel

        for (int col = 0; col < 8; col++) {//inner pixel loop
            if (curr_x >= SCREEN_WIDTH) {//stop if go beyond screen right
                break;
            }
            /*
            byte = 10110010
            mask = 10000000 ->new_pixel = 1
            mask = 01000000 ->new_pixel = 0
            mask = 00100000 ->new_pixel = 1
            */
            uint8_t new_pixel = (byte & mask) >> (7 - col);//extract 1 pixcel from sprite byte
            /*
            old = 0 new = 0 res = 0^0=0
            old = 1 new = 0 res = 1
            old = 0 new = 1 res = 1
            old = 1 new = 1 res = 0 <-collision
            */
            uint8_t old_pixel = vm->buffer[idx(curr_x, curr_y)];
            vm->buffer[idx(curr_x, curr_y)] = old_pixel ^ new_pixel;
            //collision happen when new_pixel =1 and old_pixel=1 CHIP8 turn off pixcel and set VF=1 (stay once set) 
            vf |= (new_pixel & old_pixel);

            mask >>= 1;
            curr_x++;//move right on screen
        }

        curr_y++;//move to next row
    }
/*
Sprite: 1 0 1 1 0 0 1 0
Screen: . . # . . # . .
Result: # . . # . . # .
            ↑ collision (1 XOR 1 → 0)
*/
    return vf;
}
