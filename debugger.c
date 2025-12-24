// debugger.c
#include "debugger.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

Debugger dbg;

void debugger_init(void) {
    dbg.enabled    = true;
    dbg.paused     = false;
    dbg.step       = false;
    dbg.breakpoint = 0;
}

/*
 * Call from your SDL event loop or main loop
 * (non-blocking commands can be added later)
 */
void debugger_handle_event(char key, Cpu *c) {
    if (!dbg.enabled) return;

    switch (key) {
        case 'o':   // pause
            dbg.paused = true;
            fprintf(stdout,"[DBG] Paused\n");
            break;

        case 'u':   // resume
            dbg.paused = false;
             fprintf(stdout,"[DBG] Resumed\n");
            break;

        case 'i':   // step
            dbg.step   = true;
            dbg.paused = true;
            fprintf(stdout,"[DBG] Step\n");
            break;

        case 'b':   // breakpoint at current PC
            dbg.breakpoint = c->pc;
            fprintf(stdout,"[DBG] Breakpoint set at 0x%03X\n", dbg.breakpoint);
            break;

        case 'n':   // clear breakpoint
            dbg.breakpoint = 0;
            fprintf(stdout,"[DBG] Breakpoint cleared\n");
            break;

        default:
            break;
    }
}

/*
 * Print CPU state
 */
void debugger_print_state(Cpu *c) {
    fprintf(stdout,"PC: %03X  I: %03X\n", c->pc, c->i);

    for (int i = 0; i < 16; i++) {
        fprintf(stdout,"V%X:%02X ", i, c->v[i]);
        if ((i & 3) == 3) fprintf(stdout,"\n");
    }

    fprintf(stdout,"DT:%02X  ST:%02X\n",
           c->timer.delay_timer,
           c->timer.sound_timer);

    fprintf(stdout,"---------------------------------\n");
}

/*
 * Call BEFORE executing each opcode
 */
bool debugger_should_execute(Cpu *c) {
    if (!dbg.enabled)
        return true;

    if (dbg.breakpoint && c->pc == dbg.breakpoint) {
        dbg.paused = true;
        fprintf(stdout,"\n[DBG] BREAK @ PC=0x%03X\n", c->pc);
        debugger_print_state(c);
    }

    if (dbg.step) {
        dbg.step = false;
        fprintf(stdout,"\n[DBG] STEP @ PC=0x%03X\n", c->pc);
        debugger_print_state(c);
        return true;
    }

    return !dbg.paused;
}

