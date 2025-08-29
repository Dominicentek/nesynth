#include <SDL3/SDL.h>
#include <unistd.h>

#include "nesynth.h"

int main() {
    SDL_Window* window = SDL_CreateWindow("NESynth", 1280, 720, SDL_WINDOW_RESIZABLE);
    NESynth* nesynth = nesynth_create(48000);
    SDL_ShowWindow(window);
    while (1) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) goto quit;
        }
        usleep(1000);
    }
    quit:
    SDL_DestroyWindow(window);
    nesynth_destroy(nesynth);
    return 0;
}
