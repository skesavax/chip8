#include "SDL.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "memory.h"
#include "cpu.h"
#include "chip8.h"
#include "memory.h"
#include "timer.h"
#include "vmemory.h"
#include "random_byte.h"
#include "display.h"
#include "input.h"
#include "sound.h"


uint64_t cpu_clock_from_str(const char* str) {
    long val = strtol(str, NULL, 10);
    if(val < 300 || val > 1000) {
        fprintf(stderr, "[clock] must be in [300,1000], got \"%s\"\n", str);
        exit(1);
    }
    return (uint64_t)val;
}

void emulate_chip8(Config config) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    DisplayHandler display;
    InputHandler input;
    SoundHandler sound;

    display_init(&display, config.scale, config.theme);
    input_init(&input);
    SoundHandler *s= sound_create(config.muted);
    memcpy(&sound, s, sizeof(SoundHandler));

    FILE* f = fopen(config.program_filename, "rb");
    if(!f) {
        fprintf(stderr, "Failed to open ROM: %s\n", config.program_filename);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long rom_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* program = malloc(rom_size);
    fread(program, 1, rom_size, f);
    fclose(f);

    // 1000 frames div by 60 frames = ~16frames per second; emulator must update timers (delay & sound timer)
    // roughly around 16 millseconds. CHIP-8 has two hardware timers:
    uint32_t cycle_duration_timer = 1000 / 60;//iterpreter updates at 60Hz, redrew when instruction change display data
    //This sets how fast your emulator runs the CPU (instruction execution).
    uint32_t cycle_duration_cpu = 1000 / config.cpu_clock; //1000 ms / 500 = 2 ms per instruction; cpu_clock=500Hz

    while(1) {
        //chip8 has following components:
        //1. Memory 4kb RAM, 2.Display 64x32, 3. PC 12bits, 4. I 12bits index register loc in mem
        //5. Stack 16bit address, 6. Delay timer decrement at 60Hz rate until reach zero
        //7. Sound timer wich is 8 bit like a delay timer, give beep sound when not zero
        //8. 16 8-bit general purpose variable register, 0 to F. called V0-VF, VF like a carry flag register
        Memory mem;
        Timer timer;
        VMemory vmemory;
        RandomByte rng;
        Cpu cpu;

        memory_new(&mem, program, rom_size);
        timer_init(&timer);
        vmemory_init(&vmemory);
        random_byte_init(&rng);
        Cpu *c = cpu_new(&mem, &timer, &vmemory, &rng);
        memcpy(&cpu, c, sizeof(Cpu));

        int sound_delay = 0;

        while(1) {
            uint32_t start_ticks = SDL_GetTicks();
            bool beep = cpu_update_timers(&cpu);

            if(beep) {
                sound_delay = 3;
                sound_resume(&sound);
            } else if(sound_delay == 0) {
                sound_pause(&sound);
            }
            if(sound_delay > 0) sound_delay--;

            while(1) {
                InputEvent ev;
                input_poll(&input, &ev);

                if(ev.quit) {
                    free(program);
                    return;
                }
                if(ev.restart) break;
                EmulatorState state;
                cpu_cycle(&cpu, ev.keypad, &state);

                if(state.draw_pixels!=NULL) {
                    display_draw(&display, state.draw_pixels);
                }

                uint32_t elapsed = SDL_GetTicks() - start_ticks;
                if(elapsed < cycle_duration_cpu) {
                    SDL_Delay(cycle_duration_cpu - elapsed);
                }

                if(elapsed >= cycle_duration_timer) break;
            }
        }
    }

    free(program);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
