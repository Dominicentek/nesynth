#include <SDL3/SDL.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_render.h>
#include <stdlib.h>
#include <stdio.h>

#include "ui.h"
#include "tiler.h"
#include "imageloader.h"

#define RVAL_PTR(...) (typeof(__VA_ARGS__)[]){__VA_ARGS__}

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

typedef enum {
    UIDrawType_None,
    UIDrawType_Rectangle,
    UIDrawType_GradientH,
    UIDrawType_GradientV,
    UIDrawType_Line,
    UIDrawType_Image,
    UIDrawType_Text,
    UIDrawType_SetClip,
    UIDrawType_ClearClip,
} UIDrawType;

typedef struct UIDrawList {
    struct UIDrawList *next, *prev;
    UIDrawType type;
    int priority;
    union { struct { int x, y; }; struct { int x1, y1; }; };
    union { struct { int w, h; }; struct { int x2, y2; }; };
    union { char* text; Image* image; };
    union { int color, color_from; }; int color_to;
    int srcx, srcy, srcw, srch;
} UIDrawList;

static UINode* curr_node;
static UIClip* curr_clip;
static UIWindowInfo *window_info, *window_info_head;
static UIEvent *events, *events_head;
static UIDrawList *drawlist, *drawlist_head;
static SDL_Renderer* curr_renderer;
static uint64_t start_time;
static char** curr_menu = NULL;
static float menu_pos_x, menu_pos_y;
static float menu_width, menu_height;
static void(*menu_func)(int index);
static bool menu_just_opened;
static int draw_priority, num_draw_commands;

#define comp(a, op, b) ((a) op (b) ? (a) : (b))
#define min(a, b) comp(a,<,b)
#define max(a, b) comp(a,>,b)

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
    return x >= curr_clip->rect.x && y >= curr_clip->rect.y && x < curr_clip->rect.x + curr_clip->rect.w && y < curr_clip->rect.y + curr_clip->rect.h;
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

static UIDrawList* ui_push_drawlist(UIDrawType type) {
    if (!drawlist) drawlist = drawlist_head = calloc(sizeof(UIDrawList), 1);
    UIDrawList* node = drawlist_head;
    node->next = calloc(sizeof(UIDrawList), 1);
    node->next->prev = node;
    node->type = type;
    node->priority = draw_priority;
    drawlist_head = node->next;
    num_draw_commands++;
    return node;
}

static void ui_push_clip() {
    SDL_Rect rect = { .x = curr_node->x, .y = curr_node->y, .w = curr_node->w, .h = curr_node->h };
    UIClip* clip = calloc(sizeof(UIClip), 1);
    clip->parent = curr_clip;
    if (clip->parent) ui_clip(&rect, &rect, &clip->parent->rect);
    UIDrawList* cmd = ui_push_drawlist(UIDrawType_SetClip);
    cmd->x = rect.x; cmd->y = rect.y;
    cmd->w = rect.w; cmd->h = rect.h;
    clip->rect = rect;
    curr_clip = clip;
}

static void ui_pop_clip() {
    UIClip* clip = curr_clip->parent;
    free(curr_clip);
    curr_clip = clip;
    UIDrawList* cmd = ui_push_drawlist(curr_clip ? UIDrawType_SetClip : UIDrawType_ClearClip);
    if (curr_clip) {
        cmd->x = curr_clip->rect.x; cmd->y = curr_clip->rect.y;
        cmd->w = curr_clip->rect.w; cmd->h = curr_clip->rect.h;
    }
    SDL_SetRenderClipRect(curr_renderer, curr_clip ? &curr_clip->rect : NULL);
}

static int ui_compare_drawlist(const void* a, const void* b) {
    UIDrawList* _a = *(UIDrawList**)a;
    UIDrawList* _b = *(UIDrawList**)b;
    return _a->priority - _b->priority;
}

static UIDrawList** ui_sort_drawlist() {
    if (!drawlist) return NULL;
    UIDrawList* curr = drawlist;
    UIDrawList** list = malloc(sizeof(UIDrawList*) * num_draw_commands);
    int ptr = 0;
    while (true) {
        list[ptr++] = curr;
        curr = curr->next;
        if (!curr->next) {
            free(curr);
            break;
        }
    }
    qsort(list, num_draw_commands, sizeof(UIDrawList*), ui_compare_drawlist);
    return list;
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
            SDL_FRect dst = { .x = (int)(x + off), .y = (int)y, .w = 6, .h = 8 };
            SDL_RenderTexture(curr_renderer, font, &src, &dst);
            off += 6;
        }
        text++;
    }
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

static void ui_process_drawlist() {
    UIDrawList** list = ui_sort_drawlist();
    for (int i = 0; i < num_draw_commands; i++) {
        UIDrawList* curr = list[i];
        switch (curr->type) {
            case UIDrawType_Rectangle:
                SDL_SetRenderDrawColor(curr_renderer, (curr->color >> 24) & 0xFF, (curr->color >> 16) & 0xFF, (curr->color >> 8) & 0xFF, curr->color & 0xFF);
                SDL_RenderFillRect(curr_renderer, RVAL_PTR((SDL_FRect){ .x = curr->x, .y = curr->y, .w = curr->w, .h = curr->h }));
                break;
            case UIDrawType_GradientH:
                for (int i = 0; i < curr->w; i++) {
                    int color = ui_interpolate_color(curr->color_from, curr->color_to, i / (float)curr->w);
                    SDL_SetRenderDrawColor(curr_renderer, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
                    SDL_RenderLine(curr_renderer, curr->x + i, curr->y, curr->x + i, curr->y + curr->h);
                }
                break;
            case UIDrawType_GradientV:
                for (int i = 0; i < curr->h; i++) {
                    int color = ui_interpolate_color(curr->color_from, curr->color_to, i / (float)curr->h);
                    SDL_SetRenderDrawColor(curr_renderer, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
                    SDL_RenderLine(curr_renderer, curr->x, curr->y + i, curr->x + curr->w, curr->y + i);
                }
                break;
            case UIDrawType_Line:
                SDL_SetRenderDrawColor(curr_renderer, (curr->color >> 24) & 0xFF, (curr->color >> 16) & 0xFF, (curr->color >> 8) & 0xFF, curr->color & 0xFF);
                SDL_RenderLine(curr_renderer, curr->x1, curr->y1, curr->x2, curr->y2);
                break;
            case UIDrawType_Image:
                SDL_RenderTexture(curr_renderer, img_generate_texture(curr_renderer, curr->image),
                    RVAL_PTR((SDL_FRect){ .x = curr->srcx, .y = curr->srcy, .w = curr->srcw, .h = curr->srch }),
                    RVAL_PTR((SDL_FRect){ .x = curr->x,    .y = curr->y,    .w = curr->w,    .h = curr->h    })
                );
                break;
            case UIDrawType_Text:
                ui_render_text(curr->x, curr->y, curr->color, curr->text);
                free(curr->text);
                break;
            case UIDrawType_SetClip:
                SDL_SetRenderClipRect(curr_renderer, RVAL_PTR((SDL_Rect){ .x = curr->x, .y = curr->y, .w = curr->w, .h = curr->h }));
                break;
            case UIDrawType_ClearClip:
                SDL_SetRenderClipRect(curr_renderer, NULL);
                break;
            default: break;
        }
        free(curr);
    }
    drawlist = NULL;
    num_draw_commands = 0;
    free(list);
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
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
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

static void ui_print_index(int index) {
    printf("Selected menu index %d\n", index);
}

static void ui_handle_menu() {
    float x, y;
    bool clicked = false;
    UIEvent* curr = events;
    SDL_GetMouseState(&x, &y);
    while (curr && curr->next) {
        if (curr->event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (curr->event.button.button == SDL_BUTTON_LEFT) clicked = true;
        }
        curr = curr->next;
    }
    if (menu_just_opened) clicked = false;
    menu_just_opened = false;
    ui_push_node(UINodeType_Item, menu_pos_x, menu_pos_y, menu_width, menu_height);
    ui_push_clip();
    ui_draw_rectangle(0, 0, menu_width - 0, menu_height - 0, GRAY(16));
    ui_draw_rectangle(1, 1, menu_width - 2, menu_height - 2, GRAY(32));
    int ptr = 0;
    int selected = x >= menu_pos_x && x < menu_pos_x + menu_width ? (y - menu_pos_y - 3) / 10 : -1;
    while (curr_menu[ptr]) {
        if (ptr == selected) {
            ui_draw_rectangle(1, ptr * 10 + 1, menu_width - 2, 12, GRAY(128));
            if (clicked) (menu_func ? menu_func : ui_print_index)(ptr);
        }
        ui_text(3, ptr * 10 + 3, GRAY(255), "%s", curr_menu[ptr]);
        ptr++;
    }
    if (clicked) {
        int ptr = 0;
        while (curr_menu[ptr]) free(curr_menu[ptr++]);
        free(curr_menu);
        curr_menu = NULL;
    }
    ui_pop_clip();
    ui_pop_node();
}

void ui_window(UIWindow window) {
    if (curr_node->type != UINodeType_Tile) return;
    curr_node->type = UINodeType_Window;
    curr_node->info = ui_find_window_info(window);
    ui_push_clip();
    window(curr_node->w, curr_node->h);
    ui_pop_clip();
    ui_pop_node();
    if (!curr_node) {
        if (curr_menu) ui_handle_menu();
        ui_process_drawlist();
        SDL_RenderPresent(curr_renderer);
    }
}

void ui_scrollwheel() {
    if (curr_menu) return;
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
    if (curr_menu) return;
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
    draw_priority = 0;
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
    if (curr_menu) return;
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
    if (curr_menu) return false;
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
    if (curr_menu) return false;
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

bool ui_mouse_down() {
    return SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_LMASK;
}

float ui_zoom() {
    if (curr_node->type != UINodeType_Window && curr_node->type != UINodeType_Subwindow && curr_node->type != UINodeType_Item) return 0;
    return curr_node->info->zoom;
}

float ui_scroll_x() {
    if (curr_node->type != UINodeType_Window && curr_node->type != UINodeType_Subwindow && curr_node->type != UINodeType_Item) return 0;
    return curr_node->info->scroll_x;
}

float ui_scroll_y() {
    if (curr_node->type != UINodeType_Window && curr_node->type != UINodeType_Subwindow && curr_node->type != UINodeType_Item) return 0;
    return curr_node->info->scroll_y;
}

float ui_mouse_x(UIRelativity relativity) {
    float x;
    UINode* node = curr_node;
    SDL_GetMouseState(&x, NULL);
    if (curr_node->type != UINodeType_Item) return x;
    switch (relativity) {
        case UI_GlobalRelative: node = NULL; break;
        case UI_ParentRelative: node = node->parent; break;
        case UI_WindowRelative: while (node->type != UINodeType_Window) node = node->parent; break;
        default: break;
    }
    return x - (node ? node->x : 0);
}

float ui_mouse_y(UIRelativity relativity) {
    float y;
    UINode* node = curr_node;
    SDL_GetMouseState(NULL, &y);
    if (curr_node->type != UINodeType_Item) return y;
    switch (relativity) {
        case UI_GlobalRelative: node = NULL; break;
        case UI_ParentRelative: node = node->parent; break;
        case UI_WindowRelative: while (node->type != UINodeType_Window) node = node->parent; break;
        default: break;
    }
    return y - (node ? node->y : 0);
}

static char* curr_dragndrop = NULL;
static char* last_dragndrop = NULL;
static float dragndrop_pos_x, dragndrop_pos_y;

void ui_dragndrop(char* id) {
    if (curr_menu || curr_node->type != UINodeType_Item) {
        free(id);
        return;
    }
    float x, y;
    bool holding = SDL_GetMouseState(&x, &y) & SDL_BUTTON_LMASK;
    if (last_dragndrop) free(last_dragndrop);
    last_dragndrop = strdup(id);
    free(id);
    if (!holding) {
        free(curr_dragndrop);
        curr_dragndrop = NULL;
        return;
    }
    if (ui_clicked()) {
        curr_dragndrop = strdup(last_dragndrop);
        dragndrop_pos_x = x - curr_node->x;
        dragndrop_pos_y = y - curr_node->y;
    }
    else if (!ui_is_dragndropped()) return;
    curr_node->x = x - dragndrop_pos_x;
    curr_node->y = y - dragndrop_pos_y;
    curr_clip->rect.x = x - dragndrop_pos_x;
    curr_clip->rect.y = y - dragndrop_pos_y;
    draw_priority = 1;
    UIDrawList* cmd = ui_push_drawlist(UIDrawType_SetClip);
    cmd->x = curr_clip->rect.x;
    cmd->y = curr_clip->rect.y;
    cmd->w = curr_clip->rect.w;
    cmd->h = curr_clip->rect.h;
}

bool ui_is_dragndropped() {
    if (curr_menu) return false;
    if (!curr_dragndrop || !last_dragndrop) return false;
    return strcmp(curr_dragndrop, last_dragndrop) == 0;
}

char* ui_idstr(const char* str) {
    return strdup(str);
}

char* ui_idstrnum(const char* str, int num) {
    return ui_idstrnums(str, 1, num);
}

char* ui_idstrnums(const char* str, int count, ...) {
    va_list list;
    va_start(list, count);
    char buf[1024];
    strcpy(buf, str);
    int len = strlen(buf);
    for (int i = 0; i < count; i++) {
        if (len == 1023) break;
        len += snprintf(buf + len, 1023 - len, ",%d", va_arg(list, int));
    }
    va_end(list);
    return ui_idstr(buf);
}

char* ui_idptr(const void* ptr) {
    int len = snprintf(NULL, 0, "%p", ptr);
    char* str = malloc(len + 1);
    sprintf(str, "%p", ptr);
    return str;
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
    UIDrawList* cmd = ui_push_drawlist(UIDrawType_Rectangle);
    cmd->x = x + curr_node->x;
    cmd->y = y + curr_node->y;
    cmd->w = w;
    cmd->h = h;
    cmd->color = color;
}

static void ui_draw_gradient(float x, float y, float w, float h, int from, int to, UIDrawType drawtype) {
    ui_resolve_auto(&x, &w, curr_node->w);
    ui_resolve_auto(&y, &h, curr_node->h);
    UIDrawList* cmd = ui_push_drawlist(drawtype);
    cmd->x = x + curr_node->x;
    cmd->y = y + curr_node->y;
    cmd->w = w;
    cmd->h = h;
    cmd->color_from = from;
    cmd->color_to   = to;
}

void ui_draw_gradienth(float x, float y, float w, float h, int from, int to) {
    ui_draw_gradient(x, y, w, h, from, to, UIDrawType_GradientH);
}

void ui_draw_gradientv(float x, float y, float w, float h, int from, int to) {
    ui_draw_gradient(x, y, w, h, from, to, UIDrawType_GradientV);
}

void ui_draw_line(float x1, float y1, float x2, float y2, int color) {
    if (isnan(x1) && isnan(x2)) { x1 = 0; x2 = curr_node->w; }
    if (isnan(y1) && isnan(y2)) { y1 = 0; y2 = curr_node->h; }
    if (isnan(x1) && !isnan(x2)) x1 = x2;
    if (!isnan(x1) && isnan(x2)) x2 = x1;
    if (isnan(y1) && !isnan(y2)) y1 = y2;
    if (!isnan(y1) && isnan(y2)) y2 = y1;
    UIDrawList* cmd = ui_push_drawlist(UIDrawType_Line);
    cmd->x1 = x1 + curr_node->x;
    cmd->y1 = y1 + curr_node->y;
    cmd->x2 = x2 + curr_node->x;
    cmd->y2 = y2 + curr_node->y;
    cmd->color = color;
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
    UIDrawList* cmd = ui_push_drawlist(UIDrawType_Image);
    cmd->x = dx; cmd->srcx = sx;
    cmd->y = dy; cmd->srcy = sy;
    cmd->w = dw; cmd->srcw = sw;
    cmd->h = dh; cmd->srch = sh;
    cmd->image = img;
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
    UIDrawList* cmd = ui_push_drawlist(UIDrawType_Text);
    cmd->x = x + curr_node->x;
    cmd->y = y + curr_node->y;
    cmd->color = color;
    cmd->text = str;
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
    UIDrawList* cmd = ui_push_drawlist(UIDrawType_Text);
    cmd->x = x + curr_node->x;
    cmd->y = y + curr_node->y;
    cmd->color = color;
    cmd->text = str;
}

void ui_menu(const char* items, void(*on_select)(int index)) {
    int num_items = 0;
    int ptr = 0;
    while (true) {
        if (items[ptr] == 0) {
            num_items++;
            if (items[ptr + 1] == 0) break;
        }
        ptr++;
    }
    ptr = 0;
    curr_menu = malloc((num_items + 1) * sizeof(char*));
    int max_len = 0;
    for (int i = 0; i < num_items; i++) {
        int len = strlen(items + ptr);
        if (max_len < len) max_len = len;
        curr_menu[i] = strdup(items + ptr);
        ptr += len + 1;
    }
    menu_width = max_len * 6 + 6;
    menu_height = num_items * 10 + 4;
    menu_func = on_select;
    menu_just_opened = true;
    curr_menu[num_items] = NULL;
    SDL_GetMouseState(&menu_pos_x, &menu_pos_y);
}
