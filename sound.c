#include "sound.h"
#include <stdlib.h>
#include <stdbool.h>

void init_sound(SoundHandler *sound, bool muted) {
    printf("init_source");
}

// C equivalent of Rust AudioCallback::callback
static void audio_callback(void *userdata, Uint8 *stream, int len_bytes)
{
    SoundHandler *s = (SoundHandler *)userdata;
    float *out = (float *)stream;

    int samples = len_bytes / sizeof(float);

    for (int i = 0; i < samples; i++) {
        out[i] = s->wave.volume *
                 ((s->wave.phase <= 0.5f) ? 1.0f : -1.0f);

        s->wave.phase += s->wave.phase_inc;
        if (s->wave.phase >= 1.0f)
            s->wave.phase -= 1.0f;
    }
}

SoundHandler *sound_create(int muted)
{
    SoundHandler *s = malloc(sizeof(SoundHandler));
    s->muted = muted;

    SDL_AudioSpec want, have;
    SDL_zero(want);

    want.freq = 11025;
    want.format = AUDIO_F32;     // matches Rust output type f32
    want.channels = 1;
    want.samples = 512;
    want.callback = audio_callback;
    want.userdata = s;

    s->device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);

    // Initialize square wave generator (match Rust constructor)
    s->wave.phase_inc = 440.0f / (float)have.freq;
    s->wave.phase = 0.0f;
    s->wave.volume = 0.5f;

    // Do NOT start audio yet (match Rust)
    // Rust device starts paused until .resume()
    SDL_PauseAudioDevice(s->device, 1);

    return s;
}

void sound_resume(SoundHandler *s)
{
    if (!s->muted)
        SDL_PauseAudioDevice(s->device, 0);
}

void sound_pause(SoundHandler *s)
{
    SDL_PauseAudioDevice(s->device, 1);
}

void sound_destroy(SoundHandler *s)
{
    SDL_CloseAudioDevice(s->device);
    free(s);
}
