#ifndef SOUND_H
#define SOUND_H

#include "SDL.h"
#include <stdbool.h>

typedef struct {
    float phase_inc;
    float phase;
    float volume;
} SquareWave;

typedef struct {
    SDL_AudioDeviceID device;
    int muted;
    SquareWave wave;
} SoundHandler;

SoundHandler *sound_create(int muted);
void sound_resume(SoundHandler *s);
void sound_pause(SoundHandler *s);
void sound_destroy(SoundHandler *s);
void init_sound(SoundHandler *sound, bool muted);
#endif
