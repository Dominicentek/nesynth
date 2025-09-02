#include "ui.h"

#define DEBUG_NUM_PATTERNS 64
#define DEBUG_NUM_CHANNELS 4

void window_patterns(float w, float h) {
    ui_middleclick();
    ui_update_zoom(128);
    ui_limit_scroll(0, 0, ui_zoom() * 160 * DEBUG_NUM_PATTERNS + 128, 64 * DEBUG_NUM_CHANNELS + 16);
    ui_setup_offset(false, false);
    ui_item(128, 16);
        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, RGB(32, 32, 32));
    ui_end();
    ui_subwindow(w - 128, 16);
        ui_setup_offset(true, false);
        for (int i = 0; i < DEBUG_NUM_PATTERNS; i++) {
            int color = i % 2 ? 32 : 48;
            ui_item(ui_zoom() * 160, 16);
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, RGB(color, color, color));
                ui_text_positioned(AUTO, AUTO, AUTO, AUTO, 0.5, 0, 0, 4, "%d", i + 1);
            ui_end();
        }
    ui_end();
    ui_next();
    ui_subwindow(128, h - 16);
        ui_setup_offset(false, true);
        for (int i = 0; i < DEBUG_NUM_CHANNELS; i++) {
            ui_item(128, 64);
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, RGB(32, 32, 32));
            ui_end();
            ui_next();
        }
    ui_end();
    ui_subwindow(w - 128, h - 16);
        ui_setup_offset(true, true);
        ui_flow(UIFlow_LeftToRight);
        for (int i = 0; i < DEBUG_NUM_PATTERNS; i++) {
            int color = i % 2 ? 32 : 48;
            for (int i = 0; i < DEBUG_NUM_CHANNELS; i++) {
                ui_item(ui_zoom() * 160, 64);
                    ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, RGB(color, color, color));
                ui_end();
            }
            ui_next();
        }
    ui_end();
}
