#include "shortcut.h"

#undef SHORTCUT
#define SHORTCUT(shortcut, ...) void CONCAT(__shortcut_, __LINE__)();
#include "shortcuts.c"

struct {
    int shortcut;
    void(*handle)();
} shortcuts[] = {
#undef SHORTCUT
#define SHORTCUT(shortcut, ...) { shortcut, CONCAT(__shortcut_, __LINE__) },
#include "shortcuts.c"
};

static bool ctrl = false;
static bool shift = false;
static bool alt = false;

void update_shortcuts(SDL_Event* event) {
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if      (event->key.scancode == SDL_SCANCODE_LCTRL  || event->key.scancode == SDL_SCANCODE_RCTRL)  ctrl  = true;
        else if (event->key.scancode == SDL_SCANCODE_LSHIFT || event->key.scancode == SDL_SCANCODE_RSHIFT) shift = true;
        else if (event->key.scancode == SDL_SCANCODE_LALT   || event->key.scancode == SDL_SCANCODE_RALT)   alt   = true;
        else activate_shortcut(ctrl * CTRL + alt * ALT + shift * SHIFT + event->key.scancode);
    }
    if (event->type == SDL_EVENT_KEY_UP) {
        if      (event->key.scancode == SDL_SCANCODE_LCTRL  || event->key.scancode == SDL_SCANCODE_RCTRL)  ctrl  = false;
        else if (event->key.scancode == SDL_SCANCODE_LSHIFT || event->key.scancode == SDL_SCANCODE_RSHIFT) shift = false;
        else if (event->key.scancode == SDL_SCANCODE_LALT   || event->key.scancode == SDL_SCANCODE_RALT)   alt   = false;
    }
}

void activate_shortcut(int shortcut) {
    for (int i = 0; i < sizeof(shortcuts) / sizeof(*shortcuts); i++) {
        if (shortcuts[i].shortcut == shortcut) shortcuts[i].handle();
    }
}