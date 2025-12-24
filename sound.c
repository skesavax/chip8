#include "sound.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

void init_sound(SoundHandler *sound, bool muted) {
    fprintf(stdout, "init_source");
}

static void audio_callback(void *userdata, Uint8 *stream, int len_bytes)
{
    SoundHandler *s = (SoundHandler *)userdata;
    float *out = (float *)stream;//Audio output buffer in float*, because we use want.format = AUDIO_F32

    int samples = len_bytes / sizeof(float);//len_bytes is sizeof stream buffer in bytes, samples are number of frames(64)

    for (int i = 0; i < samples; i++) {
        out[i] = s->wave.volume *
                 ((s->wave.phase <= 0.5f) ? 1.0f : -1.0f);//Flat top = phase ≤ 0.5; Flat bottom = phase > 0.5

        s->wave.phase += s->wave.phase_inc;
        if (s->wave.phase >= 1.0f)
            s->wave.phase -= 1.0f;
    }//0.00 → 0.04 → 0.08 → ... → 0.48 → 0.52 → ... → 0.96 → wrap
}

SoundHandler *sound_create(SoundHandler *s, int muted)
{
    s->muted = muted;

    SDL_AudioSpec want, have;
    SDL_zero(want);

    want.freq = 11025;//Sampling rate 11025Hz, good for emulator simple sound low quality
    want.format = AUDIO_F32; //Auto sampling format float -1 to +1, use AUDIO_S16 for signed 16 bit
    want.samples = 512;//Audio buffer size
    want.callback = audio_callback;//SDL call this function to ask for auto samples
    want.userdata = s;//custom ptr passed to callback

    //SDL request OS to get audio device, try to match with want spec.have actual format returned
    s->device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);

    // Initialize square wave generator
    s->wave.phase_inc = 440.0f / (float)have.freq;//440Hz A4 note, freq=11025Hz; Phase advanced per sample is 0.0399
    s->wave.phase = 0.0f;//[0->0.04->0.08->..1] one full wavecycle is ~25samples 
    s->wave.volume = 0.5f;//volume 50%

    //keep audio device open, but don't play audio. 1->pause (stop callback), 0->resume (start callback)
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
