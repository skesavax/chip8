#ifndef INPUT_H
#define INPUT_H

#include "SDL.h"
#include <stdint.h>

typedef struct {
    int quit;               /* bool */
    int restart;            /* bool */
    uint8_t keypad[16];     /* keypad state (0/1) */
} InputEvent;

typedef struct {
    SDL_Event event;
    /* No need to store SDL_EventPump; SDL_PollEvent does that globally */
} InputHandler;

void input_init(InputHandler *ih);

/* Poll events and fill InputEvent */
void input_poll(InputHandler *ih, InputEvent *out_event);

#endif /* INPUT_H */
