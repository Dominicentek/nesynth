#include <SDL3/SDL.h>

#include "state.h"

#include <stdlib.h>

void audio_provider(void* data, SDL_AudioStream* stream, int bytes, int total_bytes) {
    if (bytes == 0) return;
    NESynthSample* buf = malloc(bytes);
    if (state.playing) nesynth_get_samples(state.synth, buf, bytes / 2, .4f);
    else memset(buf, 0, bytes);
    SDL_PutAudioStreamData(stream, buf, bytes);
    free(buf);
}

void audio_init(int sample_rate) {
    state.synth = nesynth_create(sample_rate);
    SDL_Init(SDL_INIT_AUDIO);
    SDL_AudioSpec spec = { SDL_AUDIO_S16, 1, sample_rate };
    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    SDL_SetAudioStreamGetCallback(stream, audio_provider, NULL);
    SDL_ResumeAudioStreamDevice(stream);
}
