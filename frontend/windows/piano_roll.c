#include "ui.h"

#define DEBUG_NUM_PATTERNS 64

static const char* tones[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

void window_piano_roll(float w, float h) {
    ui_middleclick();
    ui_update_zoom(128);
    ui_limit_scroll(0, 0, DEBUG_NUM_PATTERNS * ui_zoom() * 160 + 128, 16*12*9 + 32);
    ui_setup_offset(false, false);
    ui_item(128, 32);
        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, RGB(32, 32, 32));
    ui_end();
    float width = ui_zoom() * 160;
    ui_subwindow(w - 128, 32);
        ui_flow(UIFlow_LeftToRight);
        ui_setup_offset(true, false);
        for (int i = 0; i < DEBUG_NUM_PATTERNS; i++) {
            int color = i % 2 ? 32 : 48;
            ui_item(width, 16);
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, RGB(color, color, color));
                if (ui_zoom() >= 1) {
                    for (int j = 0; j < 4; j++) {
                        ui_text(width * (j / 4.f) + 2, 4, RGB(255, 255, 255), "%d.%d", i + 1, j + 1);
                    }
                }
                else ui_text_positioned(AUTO, AUTO, AUTO, AUTO, 0.5, 0, 0, 4, RGB(255, 255, 255), "%d", i + 1);
            ui_end();
            ui_item(width, 16);
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, RGB(color, color, color));
            ui_end();
            ui_next();
        }
    ui_end();
    ui_next();
    ui_subwindow(128, h - 32);
        ui_flow(UIFlow_LeftToRight);
        ui_setup_offset(false, true);
        for (int i = 8; i >= 0; i--) {
            for (int j = 0; j < 12; j++) {
                ui_item(128, 16);
                    ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, RGB(224, 224, 224));
                    ui_text_positioned(AUTO, AUTO, AUTO, AUTO, 1, 0, -4, 4, RGB(16, 16, 16), "%s%d", tones[11 - j], i);
                    if (j % 2 == (j < 7)) ui_draw_rectangle(0, AUTO, 64, AUTO, RGB(16, 16, 16));
                ui_end();
            }
        }
    ui_end();
    ui_subwindow(w - 128, h - 32);
        ui_flow(UIFlow_LeftToRight);
        ui_setup_offset(true, true);
        for (int i = 0; i < DEBUG_NUM_PATTERNS; i++) {
            if (ui_inview(width, 16*12*9)) for (int j = 8; j >= 0; j--) {
                for (int k = 0; k < 12; k++) {
                    int color = k % 2 == (k < 7) ? 32 : 48;
                    ui_item(width, 16);
                        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, RGB(color, color, color));
                        ui_draw_line(width * 0 / 4, 0, width * 0 / 4, 16, RGB(16, 16, 16));
                        ui_draw_line(width * 1 / 4, 0, width * 1 / 4, 16, RGB(16, 16, 16));
                        ui_draw_line(width * 2 / 4, 0, width * 2 / 4, 16, RGB(16, 16, 16));
                        ui_draw_line(width * 3 / 4, 0, width * 3 / 4, 16, RGB(16, 16, 16));
                    ui_end();
                }
            }
            else ui_dummy(width, 0);
            ui_next();
        }
    ui_end();
}
