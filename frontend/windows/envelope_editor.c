#include "ui.h"
#include "state.h"

#include <stdlib.h>

static int magnet = 16;
static bool magnet_enabled = true;

static void set_magnet(int index, void* data) {
    switch (index) {
        case 0:  magnet = 1;    break;
        case 1:  magnet = 2;    break;
        case 2:  magnet = 4;    break;
        case 3:  magnet = 6;    break;
        case 4:  magnet = 8;    break;
        case 5:  magnet = 16;   break;
        case 6:  magnet = 32;   break;
        case 7:  magnet = 64;   break;
    }
}

static void set_timescale(int index, void* data) {
    *nesynth_nodetable_timescale(state_nodetable()) = index;
}

void window_envelope_editor(float w, float h) {
    int num_values = state.note_type == NESynthNoteType_Volume ? 65 : 97;
    int padding = (h - 32 - num_values * 12) / 2;
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
            if (ui_right_clicked()) ui_menu("1/1\0" "1/2\0" "1/4\0" "1/6\0" "1/8\0" "1/16\0" "1/32\0" "1/64\0", set_magnet, NULL);
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
        if (padding >= 0) {
            ui_item(128, padding);
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(32));
            ui_end();
        }
        for (int i = 0; i < num_values; i++) {
            ui_item(128, 12);
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, ui_hovered(false, true) ? GRAY(64) : GRAY(48));

                if (state.note_type == NESynthNoteType_Volume)
                    ui_text_positioned(AUTO, AUTO, AUTO, AUTO, 1, 0, -4, 2, GRAY(255), "%d", 64 - i);
                else {
                    int val = 48 - i;
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
        ui_item(w - 128, h - 32);
            ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(32));
        ui_end();
    ui_end();
}
