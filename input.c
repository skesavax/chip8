#include "input.h"
#include <string.h>

void input_init(InputHandler *ih) {
    /* Nothing special needed in C, but keep symmetry */
    (void)ih;
}

void input_poll(InputHandler *ih, InputEvent *out_event) {

    memset(out_event, 0, sizeof(InputEvent));
    /*Poll SDL events for QUIT*/
    while (SDL_PollEvent(&ih->event)) {
        if (ih->event.type == SDL_QUIT) {
            out_event->quit = 1;
        }
    }

    /* --- Poll keyboard state --- */
    const uint8_t *state = SDL_GetKeyboardState(NULL);

    /* CHIP-8 keypad mapping*/

    if (state[SDL_SCANCODE_1]) out_event->keypad[0x1] = 1;
    if (state[SDL_SCANCODE_2]) out_event->keypad[0x2] = 1;
    if (state[SDL_SCANCODE_3]) out_event->keypad[0x3] = 1;
    if (state[SDL_SCANCODE_4]) out_event->keypad[0xC] = 1;

    if (state[SDL_SCANCODE_Q]) out_event->keypad[0x4] = 1;
    if (state[SDL_SCANCODE_W]) out_event->keypad[0x5] = 1;
    if (state[SDL_SCANCODE_E]) out_event->keypad[0x6] = 1;
    if (state[SDL_SCANCODE_R]) out_event->keypad[0xD] = 1;

    if (state[SDL_SCANCODE_A]) out_event->keypad[0x7] = 1;
    if (state[SDL_SCANCODE_S]) out_event->keypad[0x8] = 1;
    if (state[SDL_SCANCODE_D]) out_event->keypad[0x9] = 1;
    if (state[SDL_SCANCODE_F]) out_event->keypad[0xE] = 1;

    if (state[SDL_SCANCODE_Z]) out_event->keypad[0xA] = 1;
    if (state[SDL_SCANCODE_X]) out_event->keypad[0x0] = 1;
    if (state[SDL_SCANCODE_C]) out_event->keypad[0xB] = 1;
    if (state[SDL_SCANCODE_V]) out_event->keypad[0xF] = 1;

    if (state[SDL_SCANCODE_O]) out_event->dbg_pause = 1;
    if (state[SDL_SCANCODE_U]) out_event->dbg_resume = 1;
    if (state[SDL_SCANCODE_I]) out_event->dbg_step = 1;
    if (state[SDL_SCANCODE_B]) out_event->dbg_break = 1;
    if (state[SDL_SCANCODE_N]) out_event->dbg_clear_break = 1;


    /* Quit on ESC */
    if (state[SDL_SCANCODE_ESCAPE]) {
        out_event->quit = 1;
    }

    /* Restart on Space (same as Rust) */
    if (state[SDL_SCANCODE_SPACE]) {
        out_event->restart = 1;
    }
}
