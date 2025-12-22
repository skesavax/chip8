#include "emulator.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

/* ---------- cpu_clock_from_str ---------- */
uint64_t cpu_clock_from_str(const char *s) {
    if (s == NULL) return 0ULL;

    char *endptr = NULL;
    unsigned long long v = strtoull(s, &endptr, 10);
    if (endptr == s || *endptr != '\0') {
        fprintf(stderr, "[clock] must be an Integer within [300, 1000]. You provided \"%s\"\n", s);
        return 0ULL;
    }

    if (v < 300ULL || v > 1000ULL) {
        fprintf(stderr, "[clock] must be within [300, 1000]. You provided \"%s\"\n", s);
        return 0ULL;
    }

    return (uint64_t)v;
}

/* ---------- emulate_chip8 ---------- */
/*
 * This function mirrors the Rust version:
 *  - Initializes SDL subsystems
 *  - Reads program file into memory
 *  - Runs an outer loop that (re)initializes emulator state on restart
 *  - Runs timers at 60 Hz and CPU at config->cpu_clock Hz
 *
 * It assumes the following external APIs are available in C:
 *  - InputHandler, input_init(SDL_Context*), input_poll(...)
 *  - DisplayHandler, display_new(SDL_Context*, scale, theme), display_draw(...)
 *  - SoundHandler, sound_new(SDL_Context*, muted), sound_resume(), sound_pause()
 *  - Memory, memory_new(&mem, data, size)
 *  - Timer, timer_init(&t)
 *  - VMemory, vmemory_init(&vm)
 *  - RandomByte, random_byte_init(&rb)
 *  - Cpu*, cpu_new(&mem, &timer, &vmemory, &rng)
 *  - cpu_update_timers(cpu) -> int (0/1)
 *  - cpu_cycle(cpu, keypad_state, &draw_pixels) -> int (0 ok, non-zero error)
 *
 * Because you requested plain SDL2, this code uses SDL for timing where convenient,
 * and standard file I/O for reading the ROM.
 */
int emulate_chip8(const Config *config) {
    if (!config) return 1;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    /* Initialize view/input/sound handlers (constructors are assumed) */
    InputHandler input;
    if (input_init(&input /*, SDL context if your API needs it */) != 0) {
        fprintf(stderr, "Failed to initialize input handler\n");
        SDL_Quit();
        return 1;
    }

    DisplayHandler display;
    if (display_init(&display, config->scale, config->theme) != 0) {
        fprintf(stderr, "Failed to initialize display\n");
        input_shutdown(&input);
        SDL_Quit();
        return 1;
    }

    SoundHandler sound;
    if (sound_init(&sound, config->muted) != 0) {
        fprintf(stderr, "Failed to initialize sound\n");
        display_shutdown(&display);
        input_shutdown(&input);
        SDL_Quit();
        return 1;
    }

    /* Read provided ROM file into memory buffer */
    FILE *f = fopen(config->program_filename, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open ROM file: %s\n", config->program_filename);
        sound_shutdown(&sound);
        display_shutdown(&display);
        input_shutdown(&input);
        SDL_Quit();
        return 1;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        fprintf(stderr, "Failed to read ROM file size\n");
        sound_shutdown(&sound);
        display_shutdown(&display);
        input_shutdown(&input);
        SDL_Quit();
        return 1;
    }
    long program_size = ftell(f);
    rewind(f);

    if (program_size <= 0) {
        fclose(f);
        fprintf(stderr, "ROM file empty or invalid\n");
        sound_shutdown(&sound);
        display_shutdown(&display);
        input_shutdown(&input);
        SDL_Quit();
        return 1;
    }

    uint8_t *program = (uint8_t*)malloc((size_t)program_size);
    if (!program) {
        fclose(f);
        fprintf(stderr, "Out of memory reading ROM\n");
        sound_shutdown(&sound);
        display_shutdown(&display);
        input_shutdown(&input);
        SDL_Quit();
        return 1;
    }

    size_t read = fread(program, 1, (size_t)program_size, f);
    fclose(f);
    if (read != (size_t)program_size) {
        free(program);
        fprintf(stderr, "Failed to read ROM file\n");
        sound_shutdown(&sound);
        display_shutdown(&display);
        input_shutdown(&input);
        SDL_Quit();
        return 1;
    }

    /* Timing durations in nanoseconds */
    const uint64_t cycle_duration_timer_ns = 1000000000ULL / 60ULL;
    uint64_t cpu_clock = config->cpu_clock ? config->cpu_clock : DEFAULT_CPU_CLOCK;
    const uint64_t cycle_duration_cpu_ns = 1000000000ULL / cpu_clock;

    int running = 1;

    /* Outer loop that recreates emulator state on restart */
    while (running) {
        /* Initialize emulator components for each run */
        Memory mem;
        if (memory_new(&mem, program, (size_t)program_size) != 0) {
            fprintf(stderr, "Failed to create memory\n");
            free(program);
            sound_shutdown(&sound);
            display_shutdown(&display);
            input_shutdown(&input);
            SDL_Quit();
            return 1;
        }

        Timer timer;
        timer_init(&timer);

        VMemory vmem;
        vmemory_init(&vmem);

        RandomByte rng;
        random_byte_init(&rng);

        Cpu *cpu = cpu_new(&mem, &timer, &vmem, &rng);
        if (!cpu) {
            fprintf(stderr, "Failed to create CPU\n");
            memory_free(&mem);
            free(program);
            sound_shutdown(&sound);
            display_shutdown(&display);
            input_shutdown(&input);
            SDL_Quit();
            return 1;
        }

        int sound_delay = 0;

        /* Emulation loop */
        while (1) {
            /* Start time measurement for timer cycle */
            uint64_t t0 = SDL_GetPerformanceCounter();
            uint64_t freq = SDL_GetPerformanceFrequency();

            uint64_t elapsed_ns = 0;

            /* Handle timers at 60Hz */
            int beep = cpu_update_timers(cpu); /* returns 0 or 1 */

            if (beep) {
                sound_delay = 3;
                sound_resume(&sound);
            } else if (sound_delay == 0) {
                sound_pause(&sound);
            }
            if (sound_delay > 0) sound_delay--;

            /* CPU loop: execute cycles until next timer tick */
            while (1) {
                InputEvent iev = {0};
                input_poll(&input, &iev); /* fills iev.quit, iev.restart, iev.keypad_state */

                if (iev.quit) {
                    running = 0;
                    break; /* break cpu loop -> will exit emulation and outer loop */
                }

                if (iev.restart) {
                    break; /* break cpu loop -> reinitialize emulator (restart) */
                }

                /* Run a single CPU cycle */
                DrawResult draw = {0}; /* type assumed: holds pointer to pixels or NULL */
                int rc = cpu_cycle(cpu, iev.keypad, &draw);
                if (rc != 0) {
                    fprintf(stderr, "CPU cycle returned error %d\n", rc);
                    /* treat as fatal */
                    running = 0;
                    break;
                }

                if (draw.pixels != NULL) {
                    if (display_draw(&display, draw.pixels) != 0) {
                        fprintf(stderr, "Display draw error\n");
                        running = 0;
                        break;
                    }
                }

                /* CPU timing: measure time for the cycle and sleep if needed */
                uint64_t t1 = SDL_GetPerformanceCounter();
                uint64_t cycle_time_ns = (uint64_t)((t1 - t0) * 1000000000ULL / freq) - elapsed_ns;
                elapsed_ns += cycle_time_ns;

                if (cycle_time_ns < cycle_duration_cpu_ns) {
                    uint64_t sleep_ns = cycle_duration_cpu_ns - cycle_time_ns;
                    /* convert to ms for SDL_Delay (at least 1 ms granularity) */
                    uint32_t sleep_ms = (uint32_t)((sleep_ns + 999999ULL) / 1000000ULL);
                    if (sleep_ms > 0) SDL_Delay(sleep_ms);
                    elapsed_ns += sleep_ms * 1000000ULL;
                }

                /* Break out of CPU loop if we reached timer tick */
                if (elapsed_ns >= cycle_duration_timer_ns) {
                    break;
                }
            } /* end cpu loop */

            if (!running) break; /* user quit or fatal error */

            /* Check if restart was requested */
            /* input_poll sets iev.restart; we broke out of CPU loop if restart was requested */
            /* If restart occurred, break emulation loop to reinitialize CPU/memory, etc. */
            /* To detect restart we can poll once more but we rely on the logic above:
             * breaking CPU loop due to iev.restart will land here; we need to check iev.restart.
             * For simplicity we poll once: */
            InputEvent check_ev = {0};
            input_poll(&input, &check_ev);
            if (check_ev.restart) {
                /* stop current emulation and reinitialize in outer loop */
                cpu_free(cpu);
                memory_free(&mem);
                break; /* break emulation loop, but keep running=1 so outer loop continues */
            }

            /* otherwise continue emulation */
        } /* end emulation loop */

        if (!running) {
            /* clean up and exit outer loop */
            cpu_free(cpu);
            memory_free(&mem);
            break;
        }

        /* on restart we already freed cpu and memory; outer loop will reinitialize */
    } /* end outer loop */

    /* cleanup global resources */
    free(program);
    sound_shutdown(&sound);
    display_shutdown(&display);
    input_shutdown(&input);
    SDL_Quit();

    return 0;
}
