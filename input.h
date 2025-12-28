#ifndef INPUT_H
#define INPUT_H

#include "SDL.h"
#include <stdint.h>

typedef struct {//@TODO: make it bool
    int quit;               /* bool */
    int restart;            /* bool */
    uint8_t keypad[16];     /* keypad state (0/1) */

    /* Debugger commands */
    int dbg_pause;
    int dbg_resume;
    int dbg_step;
    int dbg_break;
    int dbg_clear_break;
} InputEvent;

typedef struct {
    SDL_Event event;
    InputEvent ev;
} InputHandler;

void input_init(InputHandler *ih);

/* Poll events and fill InputEvent */
void input_poll(InputHandler *ih, InputEvent *out_event);

#endif /* INPUT_H */
