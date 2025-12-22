#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t delay_timer;
    uint8_t sound_timer;
} Timer;

void timer_init(Timer *t);
bool timer_update(Timer *t);

#endif
