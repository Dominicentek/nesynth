#ifndef SHORTCUT_H
#define SHORTCUT_H

#include <SDL3/SDL.h>

#define CTRL  (1 << 31) 
#define SHIFT (1 << 30)
#define ALT   (1 << 29)

#define A SDL_SCANCODE_A
#define B SDL_SCANCODE_B
#define C SDL_SCANCODE_C
#define D SDL_SCANCODE_D
#define E SDL_SCANCODE_E
#define F SDL_SCANCODE_F
#define G SDL_SCANCODE_G
#define H SDL_SCANCODE_H
#define I SDL_SCANCODE_I
#define J SDL_SCANCODE_J
#define K SDL_SCANCODE_K
#define L SDL_SCANCODE_L
#define M SDL_SCANCODE_M
#define N SDL_SCANCODE_N
#define O SDL_SCANCODE_O
#define P SDL_SCANCODE_P
#define Q SDL_SCANCODE_Q
#define R SDL_SCANCODE_R
#define S SDL_SCANCODE_S
#define T SDL_SCANCODE_T
#define U SDL_SCANCODE_U
#define V SDL_SCANCODE_V
#define W SDL_SCANCODE_W
#define X SDL_SCANCODE_X
#define Y SDL_SCANCODE_Y
#define Z SDL_SCANCODE_Z

#define SPACE SDL_SCANCODE_SPACE
#define DELETE SDL_SCANCODE_DELETE

#define CONCAT2(a, b) a##b
#define CONCAT(a, b) CONCAT2(a, b)
#define SHORTCUT(shortcut, ...) void CONCAT(__shortcut_, __LINE__)() { __VA_ARGS__ }

void update_shortcuts(SDL_Event* event);
void activate_shortcut(int shortcut);

#endif