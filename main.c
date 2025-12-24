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

uint64_t cpu_clock_from_str(const char* str);

static void terminate_with_error(const char *msg) {
    fprintf(stderr, "Application error: %s\n", msg);
    exit(1);
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage: %s <ROM> [options]\n", argv[0]);
        printf("\nOptions:\n");
        printf("  -m, --mute           Mutes emulator audio\n");
        printf("  -t, --theme <value>  Color theme (r,g,b,br,bg,bb,bw). Default bw\n");
        printf("  -s, --scale <value>  Pixel scale [1–100]. Default 10\n");
        printf("  -c, --clock <value>  CPU clock [300–1000]. Default 600\n");
        return 1;
    }

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

    int cpu_clock = (clock_str != NULL)? cpu_clock_from_str(clock_str): DEFAULT_CPU_CLOCK;

    Config config;
    config.program_filename = filename;
    config.theme = theme;
    config.scale = scale;
    config.cpu_clock = cpu_clock;
    config.muted = muted;

    if (emulate_chip8(config) != 0) {
        terminate_with_error("Emulator returned an error");
    }

    return 0;
}

int scale_from_str(const char *s, uint32_t *out_scale) {
    if (!s || !out_scale) return 1;
    char *endptr = NULL;
    unsigned long v = strtoul(s, &endptr, 10);
    if (endptr == s || *endptr != '\0') {
        fprintf(stderr, "[scale] must be an Integer within [1, 100]. You provided \"%s\"\n", s);
        return 1;
    }
    if (v < 1 || v > 100) {
        fprintf(stderr, "[scale] must be an Integer within [1, 100]. You provided \"%s\"\n", s);
        return 1;
    }
    *out_scale = (uint32_t)v;
    return 0;
}

uint64_t cpu_clock_from_str(const char* str) {
    long val = strtol(str, NULL, 10);
    if(val < 300 || val > 1000) {
        fprintf(stderr, "[clock] must be in [300,1000], got \"%s\"\n", str);
        exit(1);
    }
    return (uint64_t)val;
}
