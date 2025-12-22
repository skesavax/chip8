#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "chip8.h"
#include "cpu.h"
#include "memory.h"
#include "random_byte.h"
#include "timer.h"
#include "vmemory.h"

#include "display.h"

// from your Rust code: cpu_clock_from_str, emulate_chip8, scale_from_str, theme_from_str,
// DEFAULT_CPU_CLOCK, DEFAULT_SCALE, DEFAULT_THEME

static void terminate_with_error(const char *msg) {
    fprintf(stderr, "Application error: %s\n", msg);
    exit(1);
}

int main(int argc, char *argv[]) {

    // --- CLI Parsing (mirrors clap logic exactly) ---

    if (argc < 2) {
        printf("Usage: %s <ROM> [options]\n", argv[0]);
        printf("\nOptions:\n");
        printf("  -m, --mute           Mutes emulator audio\n");
        printf("  -t, --theme <value>  Color theme (r,g,b,br,bg,bb,bw). Default bw\n");
        printf("  -s, --scale <value>  Pixel scale [1–100]. Default 10\n");
        printf("  -c, --clock <value>  CPU clock [300–1000]. Default 600\n");
        return 1;
    }

    // Required argument
    const char *filename = argv[1];

    bool muted = false;
    const char *theme_str = NULL;
    const char *scale_str = NULL;
    const char *clock_str = NULL;

    for (int i = 2; i < argc; i++) {

        if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--mute")) {
            muted = true;
        }

        else if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--theme")) {
            if (i + 1 < argc) theme_str = argv[++i];
            else terminate_with_error("Missing value for --theme");
        }

        else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--scale")) {
            if (i + 1 < argc) scale_str = argv[++i];
            else terminate_with_error("Missing value for --scale");
        }

        else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--clock")) {
            if (i + 1 < argc) clock_str = argv[++i];
            else terminate_with_error("Missing value for --clock");
        }
    }

    // --- Equivalent to Rust code ---

    uint32_t scale;
    if (scale_str != NULL) {
        if (scale_from_str(scale_str, &scale) != 0) {
            fprintf(stderr, "Invalid scale value: %s\n", scale_str);
            exit(1);
        }
    } else {
        scale = DEFAULT_SCALE;
    }

    ColorTheme theme;
    if (theme_str != NULL) {
        if (theme_from_str(theme_str, &theme) != 0) {
            fprintf(stderr, "Unknown theme: %s\n", theme_str);
            exit(1);
        }
    } else {
        theme = DEFAULT_THEME;
    }

    int cpu_clock = (clock_str != NULL)
        ? cpu_clock_from_str(clock_str)
        : DEFAULT_CPU_CLOCK;

    // Config struct (same fields as Rust)
    Config config;
    config.program_filename = filename;
    config.theme = theme;
    config.scale = scale;
    config.cpu_clock = cpu_clock;
    config.muted = muted;

    // Run emulator
    if (emulate_chip8(config) != 0) {
        terminate_with_error("Emulator returned an error");
    }

    return 0;
}
