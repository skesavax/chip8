#include "cpu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "memory.h"
#include "timer.h"
#include "vmemory.h"
#include "random_byte.h"
#include "display.h"

/* Constants from memory module */
#ifndef PROGRAM_START
#define PROGRAM_START 0x200
#endif

#ifndef FONTSET_ADDRESS
#define FONTSET_ADDRESS 0x000
#endif



/* Forward declarations for opcode handlers (internal) */
static uint16_t cpu_fetch(Cpu *c);
static int cpu_decode_and_execute(Cpu *c, uint16_t op_code, const uint8_t input[16]);

Cpu* cpu_new(Cpu *c, Memory *memory, Timer *timer, VMemory *vmemory) {

    if (!memory || !timer || !vmemory) return NULL;

    /* Initialize registers */
    c->i = 0x0;
    c->pc = (uint16_t)PROGRAM_START;
    c->sp = 0;
    memset(c->stack, 0, sizeof(c->stack));//16 16bit stack
    memset(c->v, 0, sizeof(c->v));//16 8bit General purpose registers VX

    c->memory = *memory;
    c->timer = *timer;
    c->vmemory = *vmemory;
}

void cpu_free(Cpu *cpu) {
    if (!cpu) return;
    /* Note: underlying Memory.mem pointer ownership is assumed to be managed elsewhere
       (same as in your prior memory_new / memory_free conventions). We do not free it here. */
    free(cpu);
}

/* Public API: cpu_cycle */
int cpu_cycle(Cpu *cpu, const uint8_t input[16], DisplayHandler *out) {
    if (!cpu || !input || !out) return 1;

    /* fetch-decode-execute */
    uint16_t op_code = cpu_fetch(cpu);
    int rc = cpu_decode_and_execute(cpu, op_code, input);
    if (rc != 0) 
        return rc;

    /* update draw state */
    if (cpu->vmemory.draw_flag) {
        cpu->vmemory.draw_flag = 0;
        out->draw_pixels = cpu->vmemory.buffer; /* pointer to internal buffer */
    } else {
        out->draw_pixels = NULL;
    }

    return 0;
}

/* Update timer 60hz */
int cpu_update_timers(Cpu *cpu) {
    if (!cpu) return 0;
    return timer_update(&cpu->timer) ? 1 : 0;
}

/* --- internal helpers --- */

static uint16_t cpu_fetch(Cpu *c) {
    uint16_t pc = c->pc;
    /* Read two bytes (big-endian) */
    uint16_t b1 = (uint16_t)c->memory.mem[pc];
    uint16_t b2 = (uint16_t)c->memory.mem[pc + 1];
    c->pc += 2;
    return (uint16_t)((b1 << 8) | b2);
}


static int cpu_decode_and_execute(Cpu *c, uint16_t op_code, const uint8_t input[16]) {
    uint8_t n = (uint8_t)(op_code & 0x000F);//first nibble, 4 bit number
    size_t y = (size_t)((op_code & 0x00F0) >> 4);//second nibble, look one of 16 vx registers
    size_t x = (size_t)((op_code & 0x0F00) >> 8);//third nibble, look one of 16 vx registers
    uint8_t kk = (uint8_t)(op_code & 0x00FF);//3rd, 4th nibble, 8 bit immediate number
    uint16_t nnn = (uint16_t)(op_code & 0x0FFF);//2nd, 3rd and 4th nibble, 12 bit immediate memory address
    /*
        CHIP-8 uses post incremental stack, meaning:
            push: write to stack[sp] then sp++
            pop: sp-- then read froms stack[sp]
        size_t type is used for indexing
        uint8_t type is used for represent registers
    */

    int unrecognized = 0;

    switch (op_code & 0xF000) {//1st nibble, kind of instruction
        case 0x0000:
            switch (op_code) {
                case 0x00E0: //CLEAR
                    vmemory_clear(&c->vmemory);
                    break;
                case 0x00EE: //RETURN FROM SUBROUTINE
                    if (c->sp == 0) {
                        return -1; /* stack underflow */
                    }
                    c->sp -= 1;
                    c->pc = c->stack[c->sp];
                    break;
                default:
                    /* 0NNN - SYS addr (ignored) */
                    unrecognized = 1;
            }
            break;

        case 0x1000: //JUMP
            c->pc = nnn;//One way jump no build-in wayback
            break;

        case 0x2000: //SUB ROUTINES
            if (c->sp >= STACK_SIZE) 
                return -1; /* stack overflow */
            c->stack[c->sp] = c->pc; //save PC to stack
            c->sp += 1;//increment sp
            c->pc = nnn;
            break;

        case 0x3000: /* SE Vx, byte SKIP*/
            if (c->v[x] == kk) c->pc += 2;
            break;

        case 0x4000: /* SNE Vx, byte SKIP*/
            if (c->v[x] != kk) c->pc += 2;
            break;

        case 0x5000: /* SE Vx, Vy (last nibble must be 0) */
            if (n == 0) {
                if (c->v[x] == c->v[y]) c->pc += 2;
            } else {
                unrecognized = 1;
            }
            break;

        case 0x6000: /* LD Vx, byte */
            c->v[x] = kk;
            break;

        case 0x7000: /* ADD Vx, byte */
            c->v[x] = (uint8_t)(c->v[x] + kk);//overflow can happen 250+10=256 mod 256=4, but its intentional
            break;

        case 0x8000:
            switch (n) {
                case 0x0: /* LD Vx, Vy */
                    c->v[x] = c->v[y];
                    break;
                case 0x1: /* OR Vx, Vy */
                    c->v[x] = c->v[x] | c->v[y];
                    break;
                case 0x2: /* AND Vx, Vy */
                    c->v[x] = c->v[x] & c->v[y];
                    break;
                case 0x3: /* XOR Vx, Vy */
                    c->v[x] = c->v[x] ^ c->v[y];
                    break;
                case 0x4: { /* ADD Vx, Vy with carry */
                    uint16_t res = (uint16_t)c->v[x] + (uint16_t)c->v[y];
                    c->v[0xF] = (res > 0xFF) ? 1 : 0;
                    c->v[x] = (uint8_t)res;
                    break;
                }
                case 0x5: { /* SUB Vx, Vy - VF = NOT borrow */
                    uint8_t vx = c->v[x];
                    uint8_t vy = c->v[y];
                    c->v[0xF] = (vx > vy) ? 1 : 0;
                    c->v[x] = (uint8_t)(vx - vy);
                    break;
                }
                case 0x6: /* SHR Vx {, Vy} - Save the LSB of VX in VF, before shift VX by 1  */
                    c->v[0xF] = c->v[x] & 0x1;
                    c->v[x] >>= 1;
                    break;
                case 0x7: { /* SUBN Vx, Vy - VF = NOT borrow of Vy - Vx */
                    uint8_t vx = c->v[x];
                    uint8_t vy = c->v[y];
                    c->v[0xF] = (vy > vx) ? 1 : 0;
                    c->v[x] = (uint8_t)(vy - vx);
                    break;
                }
                case 0xE: /* SHL Vx {, Vy} - VF = MSB prior to shift */
                    c->v[0xF] = (c->v[x] & 0x80) >> 7;
                    c->v[x] <<= 1;
                    break;
                default:
                    unrecognized = 1;
            }
            break;

        case 0x9000: /* SNE Vx, Vy */
            if (n == 0) {
                if (c->v[x] != c->v[y]) c->pc += 2;
            } else {
                unrecognized = 1;
            }
            break;

        case 0xA000: /* LD I, addr */
            c->i = nnn;
            break;

        case 0xB000: /* JP V0, addr */
            c->pc = (uint16_t)(nnn + (uint16_t)c->v[0]);
            break;

        case 0xC000: /* RND Vx, byte */
            c->v[x] = (uint8_t)((uint8_t)(rand() % 256) & kk);
            break;

        case 0xD000: { /* DRW Vx, Vy, nibble */
            uint16_t start = (uint16_t)c->i;
            uint16_t end = start + (uint16_t)n;
            /* draw_sprite_no_wrap expects pointer to sprite bytes and sprite height n */
            c->v[0xF] = vmemory_draw_sprite_no_wrap(&c->vmemory, c->v[x], c->v[y], &c->memory.mem[start], (int)n);
            break;
        }

        case 0xE000:
            switch (op_code & 0x00FF) {
                case 0x9E: /* SKP Vx */
                    if (input[c->v[x]] != 0) c->pc += 2;
                    break;
                case 0xA1: /* SKNP Vx */
                    if (input[c->v[x]] == 0) c->pc += 2;
                    break;
                default:
                    unrecognized = 1;
            }
            break;

        case 0xF000:
            switch (op_code & 0x00FF) {
                case 0x07: /* LD Vx, DT */
                    c->v[x] = c->timer.delay_timer;
                    break;

                case 0x0A: { /* LD Vx, K - wait for key press, store in Vx */
                    int found = -1;
                    for (int k = 0; k <= 0xF; ++k) {
                        if (input[k] != 0) { 
                            found = k; 
                            break; 
                        }
                    }
                    if (found < 0) {
                        /* No key pressed: step PC back 2 bytes to re-execute this instruction */
                        c->pc -= 2;
                    } else {
                        c->v[x] = (uint8_t)found;
                    }
                    break;
                }

                case 0x15: /* LD DT, Vx */
                    c->timer.delay_timer = c->v[x];
                    break;

                case 0x18: /* LD ST, Vx */
                    c->timer.sound_timer = c->v[x];
                    break;

                case 0x1E: /* ADD I, Vx */
                    c->i = (uint16_t)(c->i + (uint16_t)c->v[x]);
                    break;

                case 0x29: /* LD F, Vx */
                    {
                        uint16_t nibble = (uint16_t)(c->v[x] & 0x0F);
                        c->i = (uint16_t)(FONTSET_ADDRESS + 5 * nibble);//4x5 pixel stores 5 bytes, one byte per row
                    }
                    break;

                case 0x33: /* LD B, Vx (BCD) */
                    {
                        uint8_t tmp = c->v[x];
                        c->memory.mem[c->i + 0] = tmp / 100;//100th digit
                        c->memory.mem[c->i + 1] = (tmp / 10) % 10;//10th digit
                        c->memory.mem[c->i + 2] = tmp % 10;//1th digit
                    }
                    break;

                case 0x55: /* LD [I], Vx */
                    for (size_t nidx = 0; nidx <= x; ++nidx) {
                        c->memory.mem[(size_t)c->i + nidx] = c->v[nidx];
                    }
                    break;

                case 0x65: /* LD Vx, [I] */
                    for (size_t nidx = 0; nidx <= x; ++nidx) {
                        c->v[nidx] = c->memory.mem[(size_t)c->i + nidx];
                    }
                    break;

                default:
                    unrecognized = 1;
            }
            break;

        default:
            unrecognized = 1;
    }

    if (unrecognized) {
        /* Report as an error similar to Rust Err */
        fprintf(stderr, "Instruction 0x%04X unknown\n", op_code);
        return -1;
    }

    return 0;
}

