#include "display.h"
#include <stdio.h>
#include <string.h>

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

int display_init(DisplayHandler *dh, uint32_t scale, ColorTheme theme) {
    if (!dh) return 1;
    /*
    After SDL_Init, SDL_InitSubSystem tells SDL which subsystem to start, SDL_INIT_VIDEO
    acts as a bitmask flag.*/
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL video init failed: %s\n", SDL_GetError());
        return 1;
    }

    /* Default width and height is 64x32, it be scaled to higher or lower */
    uint32_t width = (uint32_t)SCREEN_WIDTH * scale;
    uint32_t height = (uint32_t)SCREEN_HEIGHT * scale;
    /* Create SDL window*/
    SDL_Window *win = SDL_CreateWindow("Welcome to Chip8 Emulator",//title
                                       SDL_WINDOWPOS_CENTERED,//x-position of window
                                       SDL_WINDOWPOS_CENTERED,//y-position of window
                                       (int)width, (int)height,//size of window in pixcels
                                       SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);//window behaviour, 0 for default
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }
    /*Create Render that draw pixels into the window*/
    SDL_Renderer *render = SDL_CreateRenderer(win, 
                            -1,//SDL auto pick rendering driver 
                            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);//use GPU and Vsync enable is typical choice.
    if (!render) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        return 1;
    }

    /* Initialize canvas background with secondary color*/
    SDL_SetRenderDrawColor(render, theme.sr, theme.sg, theme.sb, 255);
    SDL_RenderClear(render);//clear screen
    SDL_RenderPresent(render);//show

    /* Fill handler */
    dh->window = win;
    dh->renderer = render;
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

int display_draw(DisplayHandler *dh) {
    if (!dh || !dh->renderer || !dh->draw_pixels) return 1;

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
            if (dh->draw_pixels[idx(x, y)] != 1) continue;
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
