#include "ui.h"
#include "state.h"

#include <stdlib.h>

static int magnet = 16;
static bool magnet_enabled = true;
static int right_click_index = -1;

static void set_magnet(int index, void* data) {
    switch (index) {
        case 0: magnet = 1;   break;
        case 1: magnet = 2;   break;
        case 2: magnet = 4;   break;
        case 3: magnet = 6;   break;
        case 4: magnet = 8;   break;
        case 5: magnet = 16;  break;
        case 6: magnet = 32;  break;
        case 7: magnet = 64;  break;
        case 8: magnet = 128; break;
        case 9: magnet = 256; break;
    }
}

static void set_timescale(int index, void* data) {
    *nesynth_nodetable_timescale(state_nodetable()) = index;
}

static void manage_node(int index, void* data) {
    switch (index) {
        case 0: // delete
            nesynth_nodetable_remove(data, right_click_index);
            break;
    }
}

static void draw_lines(int count, int color, float offset, float width) {
    for (int i = 1; i < count; i++) {
        ui_draw_line(width * i / count + offset, AUTO, AUTO, AUTO, color);
    }
}

static int update_nodes(float width, NESynthNodeTable* nodetable, int padding, int min, int max, int multiplier) {
    static int dragging = -1;
    static int sliding = -1;
    int count = nesynth_nodetable_num_nodes(nodetable);
    int x = ui_mouse_x(UI_ItemRelative) + *ui_scroll_x();
    int y = ui_mouse_y(UI_ItemRelative) + *ui_scroll_y() - padding;
    float pos = x / width;
    float val = ceilf(max - (y / 12.f));
    if (magnet_enabled) pos = floorf(x / width * magnet) / magnet;
    if (val < min) val = min;
    if (val > max) val = max;
    val /= multiplier;
    int hover = -1;
    int last_node = -1;
    for (int i = 0; i < count; i++) {
        int pos = nesynth_nodetable_pos(nodetable, i) * width;
        if (x >= pos && x < pos + 8) hover = i;
        if (x > pos - 1 && i != count - 1) last_node = i;
    }
    if (ui_clicked() && !ui_right_mouse_down()) {
        if (hover == -1) dragging = hover = nesynth_nodetable_insert(nodetable, pos, val);
        else dragging = hover;
    }
    if (ui_right_clicked() && !ui_mouse_down()) {
        if (hover == -1) sliding = last_node;
        else {
            right_click_index = hover;
            ui_menu("Delete\0Set Value\0Set Slide\0Set Position\0", manage_node, nodetable);
        }
    }
    if (ui_mouse_down() && dragging != -1) {
        float* value = nesynth_nodetable_value(nodetable, dragging);
        float* slide = nesynth_nodetable_slide(nodetable, dragging);
        float prev = *value;
        *value = val;
        *slide += *value - prev;
        if (*slide < min) *slide = min;
        if (*slide > max) *slide = max;
    }
    else dragging = -1;
    if (ui_right_mouse_down() && sliding != -1) *nesynth_nodetable_slide(nodetable, sliding) = val;
    else sliding = -1;
    return hover;
}

static void draw_curve(float width, NESynthNodeTable* nodetable, int index, int padding, int max, int scale) {
    float from = *nesynth_nodetable_value(nodetable, index);
    float to   = *nesynth_nodetable_slide(nodetable, index);
    float from_y = (max - from * scale) * 12 - *ui_scroll_y() + padding;
    float to_y   = (max - to   * scale) * 12 - *ui_scroll_y() + padding;
    float from_x = nesynth_nodetable_pos(nodetable, index + 0) * width - *ui_scroll_x() + 8;
    float to_x   = nesynth_nodetable_pos(nodetable, index + 1) * width - *ui_scroll_x();
    float length = to_x - from_x;
    if (length <= 0) return;
    switch ((to - from > 0) - (to - from < 0)) {
        case -1: // from > to
            ui_draw_rectangle(from_x, to_y, length, AUTO, GRAYA(224, 0.25));
            ui_draw_triangle(from_x, from_y, from_x, to_y, to_x, to_y, GRAYA(224, 0.25));
            break;
        case 0: // from = to
            ui_draw_rectangle(from_x, from_y, length, AUTO, GRAYA(224, 0.25));
            break;
        case 1: // from < to
            ui_draw_rectangle(from_x, from_y, length, AUTO, GRAYA(224, 0.25));
            ui_draw_triangle(from_x, from_y, to_x, from_y, to_x, to_y, GRAYA(224, 0.25));
            break;
    }
}

static void draw_node(float width, NESynthNodeTable* nodetable, int index, int padding, int max, int scale, bool hovered) {
    int x = nesynth_nodetable_pos(nodetable, index) * width - *ui_scroll_x();
    int y = (max - *nesynth_nodetable_value(nodetable, index) * scale) * 12 - *ui_scroll_y() + padding;
    ui_draw_rectangle(x - 1, y - 1, 9, AUTO, hovered ? GRAY(224) : GRAY(16));
    ui_draw_rectangle(x + 0, y + 0, 7, AUTO, GRAY(224));
}

void window_envelope_editor(float w, float h) {
    int num_values = state.note_type == NESynthNoteType_Volume ? 65 : 97;
    int max_value  = state.note_type == NESynthNoteType_Volume ? 64 : 48;
    int min_value  = state.note_type == NESynthNoteType_Volume ? 0 : -48;
    int multiplier = state.note_type == NESynthNoteType_Volume ? 64 : 1;
    int padding = (h - 32 - num_values * 12) / 2;
    if (padding < 0) padding = 0;
    ui_middleclick();
    ui_update_zoom(128);
    ui_limit_scroll(0, 0, NAN, num_values * 12 + 32);
    ui_flow(UIFlow_LeftToRight);
    ui_subwindow(128, 32);
        ui_setup_offset(false, false);
        ui_flow(UIFlow_LeftToRight);
        ui_item(112, 32);
            ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(32));
            ui_text(4, 4, GRAY(255), state.note_type == NESynthNoteType_Volume ? "Volume" : "Pitch");
            ui_text_positioned(AUTO, AUTO, AUTO, AUTO, 1, 0, -4, 4+16*0, magnet_enabled ? GRAY(255) : GRAY(64), "1/%d", magnet);
            ui_text_positioned(AUTO, AUTO, AUTO, AUTO, 1, 0, -4, 4+16*1, GRAY(255), "%s", (char*[]){ "Seconds", "Beats" }[*nesynth_nodetable_timescale(state_nodetable())]);
        ui_end();
        ui_next();
        ui_item(16, 16);
            ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, ui_hovered(true, true) ? GRAY(48) : GRAY(32));
            ui_image(magnet_enabled ? "images/magnet_on.png" : "images/magnet_off.png", 0, 0, 16, 16);
            if (ui_clicked()) magnet_enabled ^= 1;
            if (ui_right_clicked()) ui_menu("1/1\0" "1/2\0" "1/4\0" "1/6\0" "1/8\0" "1/16\0" "1/32\0" "1/64\0" "1/128\0" "1/256\0", set_magnet, NULL);
        ui_end();
        ui_item(16, 16);
            ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, ui_hovered(true, true) ? GRAY(48) : GRAY(32));
            ui_image("images/clock.png", 0, 0, 16, 16);
            if (ui_clicked() || ui_right_clicked()) ui_menu("Seconds\0Beats\0", set_timescale, NULL);
        ui_end();
    ui_end();
    ui_subwindow(128, h - 32);
        ui_flow(UIFlow_LeftToRight);
        ui_setup_offset(false, true);
        if (padding > 0) {
            ui_item(128, padding);
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(32));
            ui_end();
        }
        for (int i = 0; i < num_values; i++) {
            ui_item(128, 12);
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, ui_hovered(false, true) ? GRAY(64) : GRAY(48));
                int val = max_value - i;
                if (state.note_type == NESynthNoteType_Volume)
                    ui_text_positioned(AUTO, AUTO, AUTO, AUTO, 1, 0, -4, 2, GRAY(255), "%d", val);
                else {
                    const char* fmt = "0";
                    if (val > 0) fmt = "+%d";
                    if (val < 0) fmt = "-%d";
                    ui_text_positioned(AUTO, AUTO, AUTO, AUTO, 1, 0, -4, 2, GRAY(255), fmt, abs(val));
                }
            ui_end();
        }
        ui_item(128, h - 32);
            ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(32));
        ui_end();
    ui_end();
    ui_next();
    ui_subwindow(w - 128, 16);
        ui_setup_offset(false, false);
        ui_item(w - 128, 16);
            ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(32));
        ui_end();
    ui_end();
    float width = ui_zoom() * 320;
    int min = floorf( *ui_scroll_x()            / width);
    int max =  ceilf((*ui_scroll_x() + w - 128) / width);
    ui_subwindow(w - 128, 16);
        ui_setup_offset(true, false);
        for (int i = 0; i < max; i++) {
            if (i < min) {
                ui_dummy(width, 16);
                continue;
            }
            ui_item(width, 16);
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(32));
                ui_text_positioned(AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, GRAY(255), "%d", i + 1);
            ui_end();
        }
    ui_end();
    ui_subwindow(w - 128, h - 32);
        ui_setup_offset(false, false);
        ui_item(w - 128, h - 32);
            NESynthNodeTable* nodetable = state_nodetable();
            int num_nodes = nesynth_nodetable_num_nodes(nodetable);
            ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(32));
            for (int i = 0; i < num_values; i++) {
                int val = num_values - 1 - i;
                if (i % 2 == 0) ui_draw_rectangle(0, padding + i * 12 - *ui_scroll_y() - 1, nesynth_nodetable_pos(nodetable, num_nodes - 1) * width - *ui_scroll_x(), 12, GRAY(48));
                if (i != 0 || padding != 0) ui_draw_line(AUTO, padding + i * 12 - *ui_scroll_y() - 1, AUTO, AUTO, GRAY(16));
            }
            ui_draw_line(AUTO, padding + num_values * 12 - *ui_scroll_y() - 1, AUTO, AUTO, GRAY(16));
            for (int i = min; i <= max; i++) {
                ui_draw_rectangle(i * width - *ui_scroll_x() - 2, AUTO, 3, AUTO, GRAY(16));
                if (i == max) continue;
                int lines = magnet;
                while (ui_zoom() <= lines / 128.f)  lines /= 2;
                draw_lines(lines, GRAY(16), i * width - *ui_scroll_x() - 1, width);
            }
            int hover = update_nodes(width, nodetable, padding, min_value, max_value, multiplier);
            for (int i = 0; i < num_nodes - 1; i++) {
                draw_curve(width, nodetable, i, padding, max_value, multiplier);
            }
            for (int i = 0; i < num_nodes; i++) {
                draw_node(width, nodetable, i, padding, max_value, multiplier, hover == i);
            }
            if (ui_mouse_down() || ui_right_mouse_down()) {
                float mouse_y = ui_mouse_y(UI_ItemRelative);
                if (mouse_y < 64) *ui_scroll_y() -= 4;
                if (mouse_y >= h - 96) *ui_scroll_y() += 4;
            }
        ui_end();
    ui_end();
}
