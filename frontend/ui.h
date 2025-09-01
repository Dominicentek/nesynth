#ifndef UI_H
#define UI_H

#include <SDL3/SDL.h>
#include <math.h>

typedef void(*UIWindow)(float w, float h);

#define RGB(r, g, b) RGBA(r, g, b, 255)
#define RGBA(r, g, b, a) (((COLOR(r) & 0xFF) << 24) | ((COLOR(g) & 0xFF) << 16) | ((COLOR(b) & 0xFF) << 8) | (COLOR(a) & 0xFF))
#define COLOR(x) (int)(_Generic((x), float: (x) * 255, double: (x) * 255, long double: (x) * 255, default: (x)))

#define AUTO NAN

typedef enum {
    UIFlow_TopToBottom,
    UIFlow_LeftToRight,
} UIFlow;

void ui_process_event(SDL_Event* event);
void ui_clear_events();
void ui_begin(SDL_Window* window, SDL_Renderer* renderer);
void ui_splith(float ratio, float offset);
void ui_splitv(float ratio, float offset);
void ui_window(UIWindow window);

void ui_scrollwheel();
void ui_middleclick();
void ui_item(float w, float h);
void ui_subwindow(float w, float h);
void ui_flow(UIFlow flow);
void ui_end();
void ui_next();
void ui_setup_offset(bool scroll_x, bool scroll_y);
void ui_update_zoom(float offset_x, float offset_y);
float ui_zoom();

void ui_draw_rectangle(float x, float y, float w, float h, int color);
void ui_draw_gradienth(float x, float y, float w, float h, int from, int to);
void ui_draw_gradientv(float x, float y, float w, float h, int from, int to);
void ui_draw_line(float x1, float y1, float x2, float y2, int color);
void ui_image(const char* img, float x, float y, float w, float h);
void ui_image_cropped(const char* img, float dx, float dy, float dw, float dh, float sx, float sy, float sw, float sh);
void ui_text(float x, float y, const char* fmt, ...);
void ui_text_positioned(float x, float y, float w, float h, float anchor_x, float anchor_y, float off_x, float off_y, const char* fmt, ...);

#endif
