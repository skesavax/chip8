#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
//#include <SDL2/SDL.h>
#include "SDL.h"
#include "vmemory.h"

/* ColorTheme: primary RGB followed by secondary RGB */
typedef struct {
    uint8_t pr, pg, pb; /* primary color */
    uint8_t sr, sg, sb; /* secondary color */
} ColorTheme;

#define DEFAULT_SCALE 10U
/* Default theme was BWHITE in Rust: primary = white, secondary = black (0,0,0) */
#define DEFAULT_THEME ((ColorTheme){255, 255, 255, 0, 0, 0})

/* Parse theme string into ColorTheme.
 * Returns 0 on success, non-zero on error (and prints an error message).
 */
int theme_from_str(const char *s, ColorTheme *out_theme);

/* Parse scale string into uint32_t (1..=100).
 * Returns 0 on success, non-zero on error (and prints an error message).
 */
int scale_from_str(const char *s, uint32_t *out_scale);

/* DisplayHandler type: holds window/renderer and colors/scale */
typedef struct {
    SDL_Window  *window;
    SDL_Renderer* renderer;
    SDL_Color primary_color;
    SDL_Color secondary_color;
    uint32_t scale;
    uint8_t *draw_pixels;
    
} DisplayHandler;

/* Initialize display handler.
 * - scale: pixel scaling factor
 * - theme: ColorTheme struct
 * Returns 0 on success, non-zero on error.
 */
int display_init(DisplayHandler *dh, uint32_t scale, ColorTheme theme);

/* Draw framebuffer.
 * - buffer: pointer to SCREEN_WIDTH * SCREEN_HEIGHT bytes (0 or 1)
 * Returns 0 on success, non-zero on error.
 */
int display_draw(DisplayHandler *dh);

/* Shutdown and free display resources (window/renderer). Safe to call even if init failed. */
void display_shutdown(DisplayHandler *dh);

#endif /* DISPLAY_H */
