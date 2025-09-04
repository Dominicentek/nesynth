#include <SDL3/SDL.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_render.h>
#include <stdlib.h>
#include <stdio.h>

#include "ui.h"
#include "tiler.h"
#include "imageloader.h"

typedef enum {
    UINodeType_Tile,
    UINodeType_Window,
    UINodeType_Subwindow,
    UINodeType_Item,
} UINodeType;

typedef struct UIWindowInfo {
    struct UIWindowInfo* next;
    UIWindow window;
    float zoom;
    float scroll_x, scroll_y;
    float drag_x, drag_y;
    bool dragging;
} UIWindowInfo;

typedef struct UIEvent {
    struct UIEvent* next;
    SDL_Event event;
} UIEvent;

typedef struct UILinkedList {
    struct UILinkedList* next;
} UILinkedList;

typedef struct UINode {
    struct UINode* parent;
    struct UINode* next;
    UIWindowInfo* info;
    UINodeType type;
    UIFlow flow;
    float x, y, w, h;
    float cursor_x, cursor_y, cursor_max;
    float cursor_offset_x, cursor_offset_y;
    int tiler_id;
} UINode;

typedef struct UIClip {
    struct UIClip* parent;
    SDL_Rect rect;
} UIClip;

static UINode* curr_node;
static UIClip* curr_clip;
static UIWindowInfo *window_info, *window_info_head;
static UIEvent *events, *events_head;
static SDL_Renderer* curr_renderer;
static uint64_t start_time;

#define comp(a, op, b) ((a) op (b) ? (a) : (b))
#define min(a, b) comp(a,<,b)
#define max(a, b) comp(a,>,b)

int ui_hsv(float h, float s, float v) {
    int i = floor(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    float r, g, b;
    switch (i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }
    return RGB(r, g, b);
}

static void ui_clip(SDL_Rect* out, SDL_Rect* rect1, SDL_Rect* rect2) {
    if (
        rect2->x >= rect1->x + rect1->w ||
        rect2->x + rect2->w <= rect1->x ||
        rect2->y >= rect1->y + rect1->h ||
        rect2->y + rect2->h <= rect1->y
    ) *out = (SDL_Rect){ .x = 0, .y = 0, .w = 0, .h = 0 };
    else {
        out->x = max(rect1->x, rect2->x);
        out->y = max(rect1->y, rect2->y);
        out->w = min(rect1->x + rect1->w, rect2->x + rect2->w) - out->x;
        out->h = min(rect1->y + rect1->h, rect2->y + rect2->h) - out->y;
    }
}

static bool ui_intersects_node(float x, float y) {
    return x >= curr_node->x && y >= curr_node->y && x < curr_node->x + curr_node->w && y < curr_node->y + curr_node->h;
}

static UINode* ui_make_node(UINodeType type, float x, float y, float w, float h) {
    UINode* node = calloc(sizeof(UINode), 1);
    node->parent = curr_node;
    node->info = node->parent ? node->parent->info : NULL;
    node->type = type;
    node->x = x;
    node->y = y;
    node->w = w;
    node->h = h;
    return node;
}

static void ui_push_node(UINodeType type, float x, float y, float w, float h) {
    curr_node = ui_make_node(type, x, y, w, h);
}

static UINode* ui_push_node_beside(UINodeType type, float x, float y, float w, float h) {
    UINode* node = curr_node;
    while (node->next) node = node->next;
    node->next = ui_make_node(type, x, y, w, h);
    node->next->parent = curr_node->parent;
    return node->next;
}

static void ui_pop_node() {
    if (!curr_node) return;
    UINode* next = NULL;
    if (curr_node->type == UINodeType_Tile || curr_node->type == UINodeType_Window) {
        while (!next && curr_node) {
            if (curr_node->next) next = curr_node->next;
            else {
                UINode* next = curr_node->parent;
                free(curr_node);
                curr_node = next;
            }
        }
    }
    else next = curr_node->next ? curr_node->next : curr_node->parent;
    free(curr_node);
    curr_node = next;
}

static void ui_push_clip() {
    SDL_Rect rect = { .x = curr_node->x, .y = curr_node->y, .w = curr_node->w, .h = curr_node->h };
    UIClip* clip = calloc(sizeof(UIClip), 1);
    clip->parent = curr_clip;
    if (clip->parent) ui_clip(&rect, &rect, &clip->parent->rect);
    SDL_SetRenderClipRect(curr_renderer, &rect);
    clip->rect = rect;
    curr_clip = clip;
}

static void ui_pop_clip() {
    UIClip* clip = curr_clip->parent;
    free(curr_clip);
    curr_clip = clip;
    SDL_SetRenderClipRect(curr_renderer, curr_clip ? &curr_clip->rect : NULL);
}

void ui_process_event(SDL_Event* event) {
    if (!events) events = events_head = calloc(sizeof(UIEvent), 1);
    events_head->next = calloc(sizeof(UIEvent), 1);
    events_head->event = *event;
    events_head = events_head->next;
}

void ui_clear_events() {
    UIEvent* curr = events;
    while (curr) {
        UIEvent* next = curr->next;
        free(curr);
        curr = next;
    }
    events = events_head = NULL;
}

void ui_begin(SDL_Window* window, SDL_Renderer* renderer) {
    if (curr_node) return;
    int width, height;
    SDL_SetRenderDrawColor(renderer, 16, 16, 16, 255);
    SDL_RenderClear(renderer);
    SDL_GetWindowSize(window, &width, &height);
    ui_push_node(UINodeType_Tile, 0, 0, width, height);
    tiler_init(width, height);
    curr_node->tiler_id = ROOT_NODE;
    curr_renderer = renderer;
}

static void ui_split(float ratio, float offset, bool(*split_func)(int, float, float, int*, int*)) {
    if (curr_node->type != UINodeType_Tile) return;
    int id = curr_node->tiler_id;
    ui_push_node(UINodeType_Tile, curr_node->x, curr_node->y, curr_node->w, curr_node->h);
    UINode* next_node = ui_push_node_beside(UINodeType_Tile, curr_node->x, curr_node->y, curr_node->w, curr_node->h);
    split_func(id, ratio, offset, &curr_node->tiler_id, &next_node->tiler_id);
    tiler_bounds(curr_node->tiler_id, &curr_node->x, &curr_node->y, &curr_node->w, &curr_node->h);
    tiler_bounds(next_node->tiler_id, &next_node->x, &next_node->y, &next_node->w, &next_node->h);
}

void ui_splith(float ratio, float offset) {
    ui_split(ratio, offset, tiler_splith);
}

void ui_splitv(float ratio, float offset) {
    ui_split(ratio, offset, tiler_splitv);
}

static UIWindowInfo* ui_find_window_info(UIWindow window) {
    UIWindowInfo* curr = window_info;
    while (curr) {
        if (curr->window == window) return curr;
        curr = curr->next;
    }
    if (!window_info) window_info = window_info_head = calloc(sizeof(UIEvent), 1);
    curr = window_info_head;
    curr->next = calloc(sizeof(UIEvent), 1);
    curr->window = window;
    curr->scroll_x = 0;
    curr->scroll_y = 0;
    curr->zoom = 1;
    window_info_head = curr->next;
    return curr;
}

void ui_window(UIWindow window) {
    if (curr_node->type != UINodeType_Tile) return;
    curr_node->type = UINodeType_Window;
    curr_node->info = ui_find_window_info(window);
    ui_push_clip();
    window(curr_node->w, curr_node->h);
    ui_pop_clip();
    ui_pop_node();
    if (!curr_node) SDL_RenderPresent(curr_renderer);
}

void ui_scrollwheel() {
    if (curr_node->type != UINodeType_Window) return;
    UIEvent* curr = events;
    while (curr && curr->next) {
        if (
            curr->event.type == SDL_EVENT_MOUSE_WHEEL &&
            ui_intersects_node(curr->event.wheel.mouse_x, curr->event.wheel.mouse_y)
        ) {
            curr_node->info->scroll_y -= curr->event.wheel.y * 20;
        }
        curr = curr->next;
    }
}

void ui_middleclick() {
    if (curr_node->type != UINodeType_Window) return;
    UIEvent* curr = events;
    while (curr && curr->next) {
        if ((
            curr->event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
            curr->event.type == SDL_EVENT_MOUSE_BUTTON_UP
        ) && curr->event.button.button == SDL_BUTTON_MIDDLE && (
            ui_intersects_node(curr->event.button.x, curr->event.button.y) ||
            curr->event.type == SDL_EVENT_MOUSE_BUTTON_UP
        )) {
            curr_node->info->dragging = curr->event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
            curr_node->info->drag_x = curr->event.button.x;
            curr_node->info->drag_y = curr->event.button.y;
        }
        if (curr->event.type == SDL_EVENT_MOUSE_MOTION && curr_node->info->dragging) {
            curr_node->info->scroll_x -= curr->event.motion.x - curr_node->info->drag_x;
            curr_node->info->scroll_y -= curr->event.motion.y - curr_node->info->drag_y;
            curr_node->info->drag_x = curr->event.motion.x;
            curr_node->info->drag_y = curr->event.motion.y;
        }
        curr = curr->next;
    }
}

static void ui_advance(float width, float height) {
    if (curr_node->flow == UIFlow_TopToBottom) {
        curr_node->cursor_x += width + 1;
        if (curr_node->cursor_max < height) curr_node->cursor_max = height;
    }
    else {
        curr_node->cursor_y += height + 1;
        if (curr_node->cursor_max < width) curr_node->cursor_max = width;
    }
}

void ui_item(float width, float height) {
    if (curr_node->type != UINodeType_Window && curr_node->type != UINodeType_Subwindow) return;
    width -= 1; height -= 1;
    ui_push_node(UINodeType_Item, curr_node->x + curr_node->cursor_x + curr_node->cursor_offset_x, curr_node->y + curr_node->cursor_y + curr_node->cursor_offset_y, width, height);
    ui_push_clip();
}

void ui_subwindow(float width, float height) {
    if (curr_node->type != UINodeType_Window) return;
    width -= 1; height -= 1;
    ui_push_node(UINodeType_Subwindow, curr_node->x + curr_node->cursor_x + curr_node->cursor_offset_x, curr_node->y + curr_node->cursor_y + curr_node->cursor_offset_y, width, height);
    ui_push_clip();
}

void ui_dummy(float width, float height) {
    if (curr_node->type != UINodeType_Window && curr_node->type != UINodeType_Subwindow) return;
    ui_advance(width - 1, height - 1);
}

void ui_next() {
    if (curr_node->flow == UIFlow_TopToBottom) {
        curr_node->cursor_y += curr_node->cursor_max + 1;
        curr_node->cursor_x = 0;
    }
    else {
        curr_node->cursor_x += curr_node->cursor_max + 1;
        curr_node->cursor_y = 0;
    }
    curr_node->cursor_max = 0;
}

void ui_flow(UIFlow flow) {
    if (curr_node->type != UINodeType_Subwindow && curr_node->type != UINodeType_Window) return;
    curr_node->flow = flow;
}

void ui_end() {
    if (curr_node->type != UINodeType_Subwindow && curr_node->type != UINodeType_Item) return;
    float width  = curr_node->w;
    float height = curr_node->h;
    ui_pop_clip();
    ui_pop_node();
    ui_advance(width, height);
}

void ui_limit_scroll(float min_x, float min_y, float max_x, float max_y) {
    if (curr_node->type != UINodeType_Window) return;
    float prev_x = curr_node->info->scroll_x;
    float prev_y = curr_node->info->scroll_y;
    max_x -= curr_node->w;
    max_y -= curr_node->h;
    if (max_x < curr_node->info->scroll_x) curr_node->info->scroll_x = max_x;
    if (max_y < curr_node->info->scroll_y) curr_node->info->scroll_y = max_y;
    if (min_x > curr_node->info->scroll_x) curr_node->info->scroll_x = min_x;
    if (min_y > curr_node->info->scroll_y) curr_node->info->scroll_y = min_y;
    curr_node->info->drag_x -= curr_node->info->scroll_x - prev_x;
    curr_node->info->drag_y -= curr_node->info->scroll_y - prev_y;
}

void ui_setup_offset(bool h, bool v) {
    if (curr_node->type != UINodeType_Window && curr_node->type != UINodeType_Subwindow) return;
    curr_node->cursor_offset_x = -curr_node->info->scroll_x * h;
    curr_node->cursor_offset_y = -curr_node->info->scroll_y * v;
}

void ui_update_zoom(float offset_x) {
    if (curr_node->type != UINodeType_Window) return;
    UIEvent* curr = events;
    float min = powf(2, -4);
    float max = powf(2,  4);
    float prev = curr_node->info->zoom;
    float x = curr_node->x + offset_x;
    float pos = NAN;
    while (curr && curr->next) {
        if (curr->event.type == SDL_EVENT_MOUSE_WHEEL && ui_intersects_node(curr->event.wheel.mouse_x, curr->event.wheel.mouse_y)) {
            pos = curr->event.wheel.mouse_x - x;
            curr_node->info->zoom *= powf(2, curr->event.wheel.y);
            if (curr_node->info->zoom < min) curr_node->info->zoom = min;
            if (curr_node->info->zoom > max) curr_node->info->zoom = max;
        }
        curr = curr->next;
    }
    if (!isnan(pos)) {
        float f = curr_node->info->zoom / prev;
        curr_node->info->scroll_x = (curr_node->info->scroll_x + pos) * (curr_node->info->zoom / prev) - pos;
    }
}

bool ui_inview(float width, float height) {
    float x = curr_node->cursor_x + curr_node->cursor_offset_x;
    float y = curr_node->cursor_y + curr_node->cursor_offset_y;
    return x + width >= 0 && y + height >= 0 && x < curr_node->w && y < curr_node->h;
}

bool ui_hovered(bool x, bool y) {
    if (curr_node->type != UINodeType_Item) return false;
    float mx, my;
    SDL_GetMouseState(&mx, &my);
    UINode* window = curr_node;
    while (window->type != UINodeType_Window) window = window->parent;
    return
        (!x || (mx >= curr_node->x && mx < curr_node->x + curr_node->w)) && (!y || (my >= curr_node->y && my < curr_node->y + curr_node->h)) &&
        (mx >= window->x && my >= window->y && mx < window->x + window->w && my < window->y + window->h);
}

static bool ui_process_click(int button) {
    if (curr_node->type != UINodeType_Item) return false;
    UIEvent* curr = events;
    bool clicked = false;
    while (curr && curr->next) {
        if (curr->event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            clicked = curr->event.button.button == button && ui_intersects_node(curr->event.button.x, curr->event.button.y);
        }
        curr = curr->next;
    }
    return clicked;
}

bool ui_clicked() {
    return ui_process_click(SDL_BUTTON_LEFT);
}

bool ui_right_clicked() {
    return ui_process_click(SDL_BUTTON_RIGHT);
}

float ui_zoom() {
    if (curr_node->type != UINodeType_Window && curr_node->type != UINodeType_Subwindow && curr_node->type != UINodeType_Item) return 0;
    return curr_node->info->zoom;
}

static void ui_resolve_auto(float* pos, float* size, float container) {
    if (isnan(*pos) && isnan(*size)) {
        *pos = 0;
        *size = container;
    }
    else if (isnan(*pos) && !isnan(*size)) *pos = container - *size;
    else if (!isnan(*pos) && isnan(*size)) *size = container - *pos;
}

void ui_draw_rectangle(float x, float y, float w, float h, int color) {
    ui_resolve_auto(&x, &w, curr_node->w);
    ui_resolve_auto(&y, &h, curr_node->h);
    SDL_SetRenderDrawColor(curr_renderer, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    SDL_FRect rect = { .x = (int)(x + curr_node->x), .y = (int)(y + curr_node->y), .w = (int)w, .h = (int)h };
    SDL_RenderFillRect(curr_renderer, &rect);
}

static int ui_interpolate_color(int from, int to, float x) {
    float fr = ((from >> 24) & 0xFF) / 255.f;
    float fg = ((from >> 16) & 0xFF) / 255.f;
    float fb = ((from >>  8) & 0xFF) / 255.f;
    float fa = ((from >>  0) & 0xFF) / 255.f;
    float tr = ((to   >> 24) & 0xFF) / 255.f;
    float tg = ((to   >> 16) & 0xFF) / 255.f;
    float tb = ((to   >>  8) & 0xFF) / 255.f;
    float ta = ((to   >>  0) & 0xFF) / 255.f;
    int r = ((tr - fr) * x + fr) * 255;
    int g = ((tg - fg) * x + fg) * 255;
    int b = ((tb - fb) * x + fb) * 255;
    int a = ((ta - fa) * x + fa) * 255;
    return ((r & 0xFF) << 24) | ((g & 0xFF) << 16) | ((b & 0xFF) << 8) | (a & 0xFF);
}

void ui_draw_gradienth(float x, float y, float w, float h, int from, int to) {
    ui_resolve_auto(&x, &w, curr_node->w);
    ui_resolve_auto(&y, &h, curr_node->h);
    for (int i = 0; i < h; i++) {
        int color = ui_interpolate_color(from, to, i / h);
        SDL_FRect rect = { .x = (int)(x + curr_node->x), .y = (int)(y + i + curr_node->y), .w = (int)w, .h = 1 };
        SDL_SetRenderDrawColor(curr_renderer, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
        SDL_RenderFillRect(curr_renderer, &rect);
    }
}

void ui_draw_gradientv(float x, float y, float w, float h, int from, int to) {
    ui_resolve_auto(&x, &w, curr_node->w);
    ui_resolve_auto(&y, &h, curr_node->h);
    for (int i = 0; i < w; i++) {
        int color = ui_interpolate_color(from, to, i / h);
        SDL_FRect rect = { .x = (int)(x + i + curr_node->x), .y = (int)(y + curr_node->y), .w = 1, .h = (int)h };
        SDL_SetRenderDrawColor(curr_renderer, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
        SDL_RenderFillRect(curr_renderer, &rect);
    }
}

void ui_draw_line(float x1, float y1, float x2, float y2, int color) {
    if (isnan(x1) && isnan(x2)) { x1 = 0; x2 = curr_node->w; }
    if (isnan(y1) && isnan(y2)) { y1 = 0; y2 = curr_node->h; }
    if (isnan(x1) && !isnan(x2)) x1 = x2;
    if (!isnan(x1) && isnan(x2)) x2 = x1;
    if (isnan(y1) && !isnan(y2)) y1 = y2;
    if (!isnan(y1) && isnan(y2)) y2 = y1;
    SDL_SetRenderDrawColor(curr_renderer, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    SDL_RenderLine(curr_renderer, (int)(x1 + curr_node->x), (int)(y1 + curr_node->y), (int)(x2 + curr_node->x), (int)(y2 + curr_node->y));
}

void ui_image(const char* path, float x, float y, float w, float h) {
    ui_image_cropped(path, x, y, w, h, AUTO, AUTO, AUTO, AUTO);
}

void ui_image_cropped(const char* path, float dx, float dy, float dw, float dh, float sx, float sy, float sw, float sh) {
    Image* img = img_get(path);
    if (!img) return;
    if (isnan(dw)) dw = img->width;
    if (isnan(dh)) dh = img->height;
    ui_resolve_auto(&sx, &sw, img->width);
    ui_resolve_auto(&sy, &sh, img->height);
    ui_resolve_auto(&dx, &dw, sw);
    ui_resolve_auto(&dy, &dh, sh);
    dx += curr_node->x;
    dy += curr_node->y;
    SDL_FRect dst = { .x = (int)dx, .y = (int)dy, .w = (int)dw, .h = (int)dh };
    SDL_FRect src = { .x = (int)sx, .y = (int)sy, .w = (int)sw, .h = (int)sh };
    SDL_RenderTexture(curr_renderer, img_generate_texture(curr_renderer, img), &src, &dst);
}

static void ui_render_text(float x, float y, int color, char* text) {
    SDL_Texture* font = img_get_texture(curr_renderer, "images/font.png");
    SDL_SetTextureColorMod(font, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF);
    SDL_SetTextureAlphaMod(font, color & 0xFF);
    float off = 0;
    while (*text != 0) {
        int X = *text % 16;
        int Y = *text / 16 - 2;
        if (Y >= 0 && Y < 6) {
            SDL_FRect src = { .x = (int)(X * 6), .y = (int)(Y * 8), .w = 6, .h = 8 };
            SDL_FRect dst = { .x = (int)(x + off + curr_node->x), .y = (int)(y + curr_node->y), .w = 6, .h = 8 };
            SDL_RenderTexture(curr_renderer, font, &src, &dst);
            off += 6;
        }
        text++;
    }
}

void ui_text(float x, float y, int color, const char* fmt, ...) {
    va_list args1, args2;
    va_start(args1, fmt);
    va_copy(args2, args1);
    int size = vsnprintf(NULL, 0, fmt, args1);
    char* str = malloc(size + 1);
    vsprintf(str, fmt, args2);
    va_end(args1);
    va_end(args2);
    ui_render_text(x, y, color, str);
    free(str);
}

void ui_text_positioned(float x, float y, float w, float h, float anchor_x, float anchor_y, float off_x, float off_y, int color, const char* fmt, ...) {
    ui_resolve_auto(&x, &w, curr_node->w);
    ui_resolve_auto(&y, &h, curr_node->h);
    if (isnan(anchor_x)) anchor_x = 0.5;
    if (isnan(anchor_y)) anchor_y = 0.5;
    if (isnan(off_x)) off_x = 0;
    if (isnan(off_y)) off_y = 0;
    va_list args1, args2;
    va_start(args1, fmt);
    va_copy(args2, args1);
    int size = vsnprintf(NULL, 0, fmt, args1);
    char* str = malloc(size + 1);
    vsprintf(str, fmt, args2);
    va_end(args1);
    va_end(args2);
    x += roundf((w - size * 6) * anchor_x + off_x);
    y += roundf((h -        8) * anchor_y + off_y);
    ui_render_text(x, y, color, str);
    free(str);
}
