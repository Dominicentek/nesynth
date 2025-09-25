#include <SDL3/SDL.h>

#include "ui.h"
#include "windows.h"
#include "state.h"
#include "audio.h"

#include <stdlib.h>
#include <time.h>

int main() {
    srand(time(NULL));
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_CreateWindowAndRenderer("NESynth", 1280, 720, SDL_WINDOW_RESIZABLE, &window, &renderer);
    SDL_SetRenderVSync(renderer, 1);
    SDL_ShowWindow(window);
    audio_init(48000);
    state_init();
    while (1) {
        SDL_Event event;
        ui_clear_events();
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) return 0;
            ui_process_event(&event);
        }
        ui_begin(window, renderer);
        ui_splith(0, 36);
            ui_window(window_menubar);
            ui_splitv(1, -300);
                state.song == -1
                ? ui_window(window_envelope_editor)
                : ({
                    ui_splith(0.25, 0);
                        ui_window(window_patterns);
                        ui_window(window_piano_roll);
                });
                ui_splith(0.5, 0);
                    ui_window(window_songs);
                    ui_window(window_instruments);
    }
}
