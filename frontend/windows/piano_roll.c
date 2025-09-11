#include "ui.h"
#include "state.h"

#include <stdio.h>

static const char* tones[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
static int magnet = 4;
static bool magnet_enabled = true;

static void draw_line(int count, int color, float offset, float width) {
    for (int i = 1; i < count; i++) {
        ui_draw_line(width * i / count + offset, AUTO, AUTO, AUTO, color);
    }
}

static void set_magnet(int index) {
    switch (index) {
        case 0: magnet = 1; break;
        case 1: magnet = 2; break;
        case 2: magnet = 4; break;
        case 3: magnet = 6; break;
        case 4: magnet = 8; break;
        case 5: magnet = 16; break;
        case 6: magnet = 32; break;
        case 7: magnet = 64; break;
    }
}

static void update_notes(int pattern_pos, float w) {
    int note = NESYNTH_NOTE(C, 9) - ui_mouse_y(UI_ItemRelative) / 12;
    float pos = ui_mouse_x(UI_ItemRelative) / w * 4;
    if (magnet_enabled) pos = floorf(pos * magnet) / magnet;
    if (ui_clicked()) nesynth_insert_note(
        nesynth_get_pattern_at(state.channel, pattern_pos),
        NESynthNoteType_Instrument, state.instrument,
        note, pos, 1.f / magnet
    );
}

void window_piano_roll(float w, float h) {
    int patterns = state.channel ? nesynth_song_get_length(state.song) : 0;
    ui_middleclick();
    ui_update_zoom(128);
    ui_limit_scroll(0, 0, patterns * ui_zoom() * 160 + 128, 12*12*9 + 32);
    ui_setup_offset(false, false);
    ui_subwindow(128, 32);
        ui_flow(UIFlow_LeftToRight);
        ui_item(112, 32);
            ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(32));
            ui_text_positioned(AUTO, AUTO, AUTO, AUTO, 1, 0, -4, 4, magnet_enabled ? GRAY(255) : GRAY(64), "1/%d", magnet);
        ui_end();
        ui_next();
        ui_item(16, 16);
            ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, ui_hovered(true, true) ? GRAY(48) : GRAY(32));
            ui_image(magnet_enabled ? "images/magnet_on.png" : "images/magnet_off.png", 0, 0, 16, 16);
            if (ui_clicked()) magnet_enabled ^= 1;
            if (ui_right_clicked()) ui_menu("1/1\0" "1/2\0" "1/4\0" "1/6\0" "1/8\0" "1/16\0" "1/32\0" "1/64\0", set_magnet);
        ui_end();
        ui_item(16, 16);
            ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, ui_hovered(true, true) ? GRAY(48) : GRAY(32));
            ui_text(4, 4, GRAY(255), "v");
        ui_end();
    ui_end();
    float width = ui_zoom() * 160;
    ui_subwindow(w - 128, 32);
        ui_flow(UIFlow_LeftToRight);
        ui_setup_offset(true, false);
        for (int i = 0; i < patterns; i++) {
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
        ui_setup_offset(true, true);
        for (int i = 0; i < patterns; i++) {
            if (!ui_inview(width, 12*12*9)) {
                ui_dummy(width, 0);
                continue;
            }
            ui_item(width, 12*12*9);
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(48));
                for (int i = 0; i < 12 * 9; i++) {
                    int tone = i % 12;
                    if (tone % 2 == (tone < 7)) ui_draw_rectangle(AUTO, i * 12, AUTO, 12, GRAY(32));
                    ui_draw_line(AUTO, i * 12 + 11, AUTO, AUTO, GRAY(16));
                }
                ui_draw_line(width * 0 / 4, AUTO, AUTO, AUTO, GRAY(16));
                if (ui_zoom() >= 0.25) {
                    draw_line(4, GRAY(16), 0, width);
                    int num_lines = magnet;
                    if (ui_zoom() < 8 && num_lines > 32) num_lines = 32;
                    if (ui_zoom() < 4 && num_lines > 16) num_lines = 16;
                    if (ui_zoom() >= 2) {
                        draw_line(num_lines, GRAYA(16, 0.5), width * 0 / 4, width / 4);
                        draw_line(num_lines, GRAYA(16, 0.5), width * 1 / 4, width / 4);
                        draw_line(num_lines, GRAYA(16, 0.5), width * 2 / 4, width / 4);
                        draw_line(num_lines, GRAYA(16, 0.5), width * 3 / 4, width / 4);
                    }
                }
                ui_draw_line(width * 4 / 4, AUTO, AUTO, AUTO, GRAY(16));
                update_notes(i, width);
                NESynthPattern* pattern = nesynth_any_pattern_at(state.channel, i) ? nesynth_get_pattern_at(state.channel, i) : NULL;
                if (pattern) {
                    NESynthIter* iter = nesynth_iter_notes(pattern, NESynthNoteType_Instrument);
                    while (nesynth_iter_next(iter)) {
                        NESynthNote* note = nesynth_iter_get(iter);
                        float note_value = *nesynth_base_note(note) - NESYNTH_NOTE(C, 0);
                        float x = *nesynth_note_start(note) * width / 4;
                        float y = (NESYNTH_NOTE(C, 9) - 1 - *nesynth_base_note(note)) * 12;
                        float w = *nesynth_note_length(note) * width / 4;
                        ui_draw_rectangle(x + 0, y, w - 0, 12, GRAY(16));
                        ui_draw_rectangle(x + 1, y, w - 1, 11, GRAY(224));
                        int octave = note_value / 12;
                        int tone = roundf(note_value - octave * 12);
                        ui_text(x + 2, y + 2, GRAY(16), "%s%d", tones[tone], octave);
                    }
                }
            ui_end();
        }
    ui_end();
}
