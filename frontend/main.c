#include <SDL3/SDL.h>

#include "ui.h"
#include "tiler.h"
#include "windows/windows.h"

static void debug_rect(SDL_Renderer* renderer, int node, int r, int g, int b) {
    SDL_FRect rect;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, 127);
    tiler_bounds(node, &rect.x, &rect.y, &rect.w, &rect.h);
    SDL_RenderFillRect(renderer, &rect);
}

int main() {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_CreateWindowAndRenderer("NESynth", 1280, 720, 0, &window, &renderer);
    SDL_SetRenderVSync(renderer, 1);
    SDL_ShowWindow(window);
    while (1) {
        SDL_Event event;
        ui_clear_events();
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) goto quit;
            ui_process_event(&event);
        }
        ui_begin(window, renderer);
        ui_splith(0, 32);
            ui_window(window_menubar);
            ui_splitv(1, -300);
                ui_splith(0.25, 0);
                    ui_window(window_patterns);
                    ui_window(window_piano_roll);
                ui_splith(0.5, 0);
                    ui_window(window_songs);
                    ui_window(window_instruments);
    }
    quit:
    SDL_DestroyWindow(window);
    return 0;
}
