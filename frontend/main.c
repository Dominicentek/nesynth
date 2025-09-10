#include <SDL3/SDL.h>

#include "ui.h"
#include "windows.h"
#include "state.h"

int main() {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_CreateWindowAndRenderer("NESynth", 1280, 720, 0, &window, &renderer);
    SDL_SetRenderVSync(renderer, 1);
    SDL_ShowWindow(window);
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
                ui_splith(0.25, 0);
                    ui_window(window_patterns);
                    ui_window(window_piano_roll);
                ui_splith(0.5, 0);
                    ui_window(window_songs);
                    ui_window(window_instruments);
    }
}
