#include "ui.h"

#define DEBUG_NUM_SONGS 16

static int selected_song = 0;

void window_songs(float w, float h) {
    ui_scrollwheel();
    ui_setup_offset(false, false);
    ui_item(w - 16, 16);
        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, RGB(32, 32, 32));
        ui_text_positioned(AUTO, AUTO, AUTO, AUTO, 0.5, 0, 0, 4, RGB(255, 255, 255), "Songs");
    ui_end();
    ui_item(16, 16);
        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, RGB(32, 32, 32));
        ui_text_positioned(AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, RGB(255, 255, 255), "+");
    ui_end();
    ui_next();
    ui_subwindow(w, h - 16);
        ui_flow(UIFlow_LeftToRight);
        ui_setup_offset(false, true);
        for (int i = 0; i < DEBUG_NUM_SONGS; i++) {
            ui_item(w, 16);
                if (ui_clicked()) selected_song = i;
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, ui_hsv((float)i / DEBUG_NUM_SONGS, 1, 1));
                ui_text(4, 4, i == selected_song ? RGB(255, 255, 255) : RGB(16, 16, 16), "Song %d", i + 1);
            ui_end();
        }
        ui_item(w, h);
            ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, RGB(32, 32, 32));
        ui_end();
    ui_end();
}
