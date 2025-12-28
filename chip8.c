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
#include "display.h"
#include "input.h"
#include "sound.h"


int emulate_chip8(Config config) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);

    DisplayHandler display;
    InputHandler input;
    SoundHandler sound;

    display_init(&display, config.scale, config.theme);
    input_init(&input);//do nothing
    sound_create(&sound, config.muted);

    FILE* rom = fopen(config.program_filename, "rb");
    if(!rom) {
        fprintf(stderr, "Failed to open ROM: %s\n", config.program_filename);
        return 1;
    }
    //Get ROM data
    fseek(rom, 0, SEEK_END);
    long rom_size = ftell(rom);
    fseek(rom, 0, SEEK_SET);
    uint8_t* program = malloc(rom_size);
    fread(program, 1, rom_size, rom);
    fclose(rom);

    int running = 1;

    const uint64_t cycle_duration_timer_ns = 1000000000ULL / 60ULL;//1sec/60 = 16.67msec or 16,666,666nsec
    uint64_t cpu_clock = config.cpu_clock ? config.cpu_clock : DEFAULT_CPU_CLOCK;//600Hz 600 instructions per sec
    const uint64_t cycle_duration_cpu_ns = 1000000000ULL / cpu_clock;//16,666,666nsec, desired time per CPU instruction

    while(running) {
        //chip8 has following components:
        //1. Memory 4kb RAM, 2.Display 64x32, 3. PC 12bits, 4. I 12bits index register loc in mem
        //5. Stack 16bit address, 6. Delay timer decrement at 60Hz rate until reach zero
        //7. Sound timer wich is 8 bit like a delay timer, give beep sound when not zero
        //8. 16 8-bit general purpose variable register, 0 to F. called V0-VF, VF like a carry flag register
        Memory mem;
        Timer timer;
        VMemory vmemory;
        Cpu cpu;

        memory_new(&mem, program, rom_size);
        timer_init(&timer);
        vmemory_init(&vmemory);
        cpu_new(&cpu, &mem, &timer, &vmemory);
        debugger_init();

        int sound_delay = 0;

        while(1) {
            //uint32_t start_ticks = SDL_GetTicks();
            bool beep = cpu_update_timers(&cpu);
            uint64_t t0 = SDL_GetPerformanceCounter();
            uint64_t freq = SDL_GetPerformanceFrequency();

            uint64_t elapsed_ns = 0;

            if(beep) {
                sound_delay = 3;//keep beep ON for 3 ticks
                sound_resume(&sound);
            } else if(sound_delay == 0) {
                sound_pause(&sound);
            }
            if(sound_delay > 0) 
                sound_delay--;

            while(1) {
                input_poll(&input, &input.ev);

                if(input.ev.quit) {
                    running = 0;
                    break;
                }
                if(input.ev.restart)
                    break;

                /* Debugger control */
                if (input.ev.dbg_pause)  
                    debugger_handle_event('o', &cpu);
                if (input.ev.dbg_resume) 
                    debugger_handle_event('u', &cpu);
                if (input.ev.dbg_step) {
                    debugger_handle_event('i', &cpu);
                }
                if (input.ev.dbg_break) {
                    debugger_handle_event('b', &cpu);
                }
                if (input.ev.dbg_clear_break) {
                    debugger_handle_event('n', &cpu);
                }

                if (debugger_should_execute(&cpu)) {
                    cpu_cycle(&cpu, input.ev.keypad, &display);
                }
                if(display.draw_pixels!=NULL) {
                    display_draw(&display);
                }

                /* CPU timing: measure time for the cycle and sleep if needed */
                uint64_t t1 = SDL_GetPerformanceCounter();
                //convert to nsec
                uint64_t cycle_time_ns = (uint64_t)((t1 - t0) * 1000000000ULL / freq) - elapsed_ns;
                elapsed_ns += cycle_time_ns;//accumulate elapsed time

                /*//check cpu running too fast (>600 instructions per sec) speed control*/
                if (cycle_time_ns < cycle_duration_cpu_ns) {
                    /* If instruction executed faster than that, we must slow down. */
                    /*calculate how long to sleep*/
                    uint64_t sleep_ns = cycle_duration_cpu_ns - cycle_time_ns;
                    /*convert ns to ms Formulat with roundup: x+(1000000-1)/1000000*/
                    uint32_t sleep_ms = (uint32_t)((sleep_ns + 999999ULL) / 1000000ULL);
                    if (sleep_ms > 0) 
                        SDL_Delay(sleep_ms);
                    elapsed_ns += sleep_ms * 1000000ULL;
                }

                /* Break out of CPU loop if we reached timer tick */
                if (elapsed_ns >= cycle_duration_timer_ns) {
                    /*decrement delay timer, sound timer, screen refresh every ~16.7msec*/
                    break;
                }
            }
            if (!running){
                break;
            }
        }
    }
    free(program);
    display_shutdown(&display);
    SDL_Quit();
    return 0;
}
