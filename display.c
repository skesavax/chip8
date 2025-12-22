#include "display.h"
#include <stdio.h>
#include <string.h>

/* Predefined themes (replicating your Rust constants) */
static const ColorTheme THEME_RED   = {255, 180, 40, 120, 8, 0};
static const ColorTheme THEME_GREEN = {55, 255, 40, 30, 80, 0};
static const ColorTheme THEME_BLUE  = {5, 50, 90, 60, 114, 164};
static const ColorTheme THEME_BRED  = {255, 0, 0, 0, 0, 0};
static const ColorTheme THEME_BGREEN= {0, 255, 0, 0, 0, 0};
static const ColorTheme THEME_BBLUE = {0, 0, 255, 0, 0, 0};
static const ColorTheme THEME_BWHITE= {255, 255, 255, 0, 0, 0};

int theme_from_str(const char *s, ColorTheme *out_theme) {
    if (!s || !out_theme) return 1;
    if (strcmp(s, "r") == 0)      *out_theme = THEME_RED;
    else if (strcmp(s, "g") == 0) *out_theme = THEME_GREEN;
    else if (strcmp(s, "b") == 0) *out_theme = THEME_BLUE;
    else if (strcmp(s, "br") == 0) *out_theme = THEME_BRED;
    else if (strcmp(s, "bg") == 0) *out_theme = THEME_BGREEN;
    else if (strcmp(s, "bb") == 0) *out_theme = THEME_BBLUE;
    else if (strcmp(s, "bw") == 0) *out_theme = THEME_BWHITE;
    else {
        fprintf(stderr, "[theme] \"%s\" is not known. Try --help or -h for a list of themes\n", s);
        return 1;
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

int display_init(DisplayHandler *dh, uint32_t scale, ColorTheme theme) {
    if (!dh) return 1;

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL video init failed: %s\n", SDL_GetError());
        return 1;
    }

    /* width and height are SCREEN_WIDTH * scale etc. */
    uint32_t width = (uint32_t)SCREEN_WIDTH * scale;
    uint32_t height = (uint32_t)SCREEN_HEIGHT * scale;

    SDL_Window *win = SDL_CreateWindow("chip8emu_c",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       (int)width, (int)height,
                                       0);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        return 1;
    }

    /* Initialize canvas background with secondary color (using same ordering as Rust: theme.sr,sg,sb) */
    SDL_SetRenderDrawColor(ren, theme.sr, theme.sg, theme.sb, 255);
    SDL_RenderClear(ren);
    SDL_RenderPresent(ren);

    /* Fill handler */
    dh->window = win;
    dh->renderer = ren;
    dh->primary_color.r = theme.pr;
    dh->primary_color.g = theme.pg;
    dh->primary_color.b = theme.pb;
    dh->primary_color.a = 255;
    dh->secondary_color.r = theme.sr;
    dh->secondary_color.g = theme.sg;
    dh->secondary_color.b = theme.sb;
    dh->secondary_color.a = 255;
    dh->scale = scale;

    return 0;
}

int display_draw(DisplayHandler *dh, const uint8_t *buffer) {
    if (!dh || !dh->renderer || !buffer) return 1;

    /* Clear screen with secondary color */
    SDL_SetRenderDrawColor(dh->renderer,
                           dh->secondary_color.r,
                           dh->secondary_color.g,
                           dh->secondary_color.b,
                           dh->secondary_color.a);
    SDL_RenderClear(dh->renderer);

    /* Set primary color for pixels */
    SDL_SetRenderDrawColor(dh->renderer,
                           dh->primary_color.r,
                           dh->primary_color.g,
                           dh->primary_color.b,
                           dh->primary_color.a);

    /* Draw each set pixel as a filled rectangle of size scale x scale */
    for (size_t y = 0; y < SCREEN_HEIGHT; ++y) {
        for (size_t x = 0; x < SCREEN_WIDTH; ++x) {
            if (buffer[idx(x, y)] != 1) continue;
            SDL_Rect r;
            r.x = (int)(x * (size_t)dh->scale);
            r.y = (int)(y * (size_t)dh->scale);
            r.w = (int)dh->scale;
            r.h = (int)dh->scale;
            SDL_RenderFillRect(dh->renderer, &r);
        }
    }

    SDL_RenderPresent(dh->renderer);
    return 0;
}

void display_shutdown(DisplayHandler *dh) {
    if (!dh) return;
    if (dh->renderer) {
        SDL_DestroyRenderer(dh->renderer);
        dh->renderer = NULL;
    }
    if (dh->window) {
        SDL_DestroyWindow(dh->window);
        dh->window = NULL;
    }
    /* We don't call SDL_Quit here: caller (emulator) is responsible for global SDL_Quit */
}
