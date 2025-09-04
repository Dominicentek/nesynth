#ifndef UI_H
#define UI_H

#include <SDL3/SDL.h>
#include <math.h>

typedef void(*UIWindow)(float w, float h);

#define COLOR(x) (int)(_Generic((x), float: (x) * 255, double: (x) * 255, long double: (x) * 255, default: (x)))
#define RGB(r, g, b) RGBA(r, g, b, 255)
#define HSV(h, s, v) HSVA(h, s, v, 255)
#define RGBA(r, g, b, a) (((COLOR(r) & 0xFF) << 24) | ((COLOR(g) & 0xFF) << 16) | ((COLOR(b) & 0xFF) << 8) | (COLOR(a) & 0xFF))
#define HSVA(h, s, v, a) ({ \
    int i = floor((h) * 6); \
    float f = (h) * 6 - i; \
    float p = (v) * (1 - (s)); \
    float q = (v) * (1 - f * (s)); \
    float t = (v) * (1 - (1 - f) * (s)); \
    float r, g, b; \
    switch (i % 6) { \
        case 0: r = (v), g = (t), b = (p); break; \
        case 1: r = (q), g = (v), b = (p); break; \
        case 2: r = (p), g = (v), b = (t); break; \
        case 3: r = (p), g = (q), b = (v); break; \
        case 4: r = (t), g = (p), b = (v); break; \
        case 5: r = (v), g = (p), b = (q); break; \
    } \
    RGBA(r, g, b, a); \
})

#define AUTO NAN

typedef enum {
    UIFlow_TopToBottom,
    UIFlow_LeftToRight,
} UIFlow;

typedef enum {
    UI_GlobalRelative,
    UI_WindowRelative,
    UI_ParentRelative,
    UI_ItemRelative,
} UIRelativity;

int ui_hsv(float h, float s, float v);

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
void ui_dummy(float w, float h);
void ui_flow(UIFlow flow);
void ui_end();
void ui_next();
void ui_limit_scroll(float min_x, float min_y, float max_x, float max_y);
void ui_setup_offset(bool scroll_x, bool scroll_y);
void ui_update_zoom(float offset_x);
bool ui_inview(float width, float height);
bool ui_hovered(bool x, bool y);
bool ui_clicked();
bool ui_right_clicked();
float ui_zoom();
float ui_mouse_x(UIRelativity relativity);
float ui_mouse_y(UIRelativity relativity);
void ui_dragndrop(char* id);
bool ui_is_dragndropped();

char* ui_idstr(const char* str);
char* ui_idstrnum(const char* str, int num);
char* ui_idstrnums(const char* str, int count, ...);
char* ui_idptr(const void* ptr);

void ui_draw_rectangle(float x, float y, float w, float h, int color);
void ui_draw_gradienth(float x, float y, float w, float h, int from, int to);
void ui_draw_gradientv(float x, float y, float w, float h, int from, int to);
void ui_draw_line(float x1, float y1, float x2, float y2, int color);
void ui_image(const char* img, float x, float y, float w, float h);
void ui_image_cropped(const char* img, float dx, float dy, float dw, float dh, float sx, float sy, float sw, float sh);
void ui_text(float x, float y, int color, const char* fmt, ...);
void ui_text_positioned(float x, float y, float w, float h, float anchor_x, float anchor_y, float off_x, float off_y, int color, const char* fmt, ...);

void ui_menu(const char* items, void(*on_select)(int index));

#endif
