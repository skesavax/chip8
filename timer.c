#include "timer.h"

void timer_init(Timer *t) {
    t->delay_timer = 0;
    t->sound_timer = 0;
}

bool timer_update(Timer *t) {
    bool beep = false;

    if (t->delay_timer > 0) {
        t->delay_timer--;
    }

    if (t->sound_timer > 0) {
        beep = true;
        t->sound_timer--;
    }

    return beep;
}
