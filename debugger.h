#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <stdint.h>
#include <stdbool.h>
#include "SDL.h"
#include "cpu.h"

typedef struct {
    bool enabled;
    bool paused;
    bool step;
    uint16_t breakpoint;
} Debugger;

extern Debugger dbg;

void debugger_init(void);
void debugger_handle_event(char key, Cpu *c);
bool debugger_should_execute(Cpu *c);
void debugger_render(SDL_Renderer *renderer);

#endif
