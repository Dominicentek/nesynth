#include "ui.h"

#define DEBUG_NUM_PATTERNS 64

static const char* tones[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

static void draw_line(int count, int color, float offset, float width) {
    for (int i = 1; i < count; i++) {
        ui_draw_line(width * i / count + offset, AUTO, AUTO, AUTO, color);
    }
}

void window_piano_roll(float w, float h) {
    ui_middleclick();
    ui_update_zoom(128);
    ui_limit_scroll(0, 0, DEBUG_NUM_PATTERNS * ui_zoom() * 160 + 128, 12*12*9 + 32);
    ui_setup_offset(false, false);
    ui_item(128, 32);
        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(32));
    ui_end();
    float width = ui_zoom() * 160;
    ui_subwindow(w - 128, 32);
        ui_flow(UIFlow_LeftToRight);
        ui_setup_offset(true, false);
        for (int i = 0; i < DEBUG_NUM_PATTERNS; i++) {
            int color = i % 2 ? 32 : 48;
            ui_item(width, 16);
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(color));
                if (ui_zoom() >= 1) {
                    for (int j = 0; j < 4; j++) {
                        ui_text(width * (j / 4.f) + 2, 4, GRAY(255), "%d.%d", i + 1, j + 1);
                    }
                }
                else ui_text_positioned(AUTO, AUTO, AUTO, AUTO, 0.5, 0, 0, 4, GRAY(255), "%d", i + 1);
            ui_end();
            ui_item(width, 16);
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(color));
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
                ui_item(128, 12);
                    int white = ui_hovered(false, true) ? 255 : 224;
                    int black = ui_hovered(false, true) ? 64 : 16;
                    ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(white));
                    ui_text_positioned(AUTO, AUTO, AUTO, AUTO, 1, 0, -2, 2, GRAY(black), "%s%d", tones[11 - j], i);
                    if (j % 2 == (j < 7)) ui_draw_rectangle(0, AUTO, 64, AUTO, GRAY(16));
                ui_end();
            }
        }
    ui_end();
    ui_subwindow(w - 128, h - 32);
        ui_flow(UIFlow_LeftToRight);
        ui_setup_offset(true, true);
        for (int i = 0; i < DEBUG_NUM_PATTERNS; i++) {
            if (ui_inview(width, 12*12*9)) for (int j = 8; j >= 0; j--) {
                for (int k = 0; k < 12; k++) {
                    int color = k % 2 == (k < 7) ? 32 : 48;
                    ui_item(width, 12);
                        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(color));
                        ui_draw_line(width * 0 / 4, 0, width * 0 / 4, 12, GRAY(16));
                        if (ui_zoom() >= 0.25) {
                            draw_line(4, GRAY(16), 0, width);
                            if (ui_zoom() >= 2) {
                                color -= 8;
                                draw_line(4, GRAY(color), width * 0 / 4, width / 4);
                                draw_line(4, GRAY(color), width * 1 / 4, width / 4);
                                draw_line(4, GRAY(color), width * 2 / 4, width / 4);
                                draw_line(4, GRAY(color), width * 3 / 4, width / 4);
                            }
                        }
                        ui_draw_line(width * 4 / 4, 0, width * 4 / 4, 12, GRAY(16));
                    ui_end();
                }
            }
            else ui_dummy(width, 0);
            ui_next();
        }
    ui_end();
}
