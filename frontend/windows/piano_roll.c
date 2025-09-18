#include "ui.h"
#include "state.h"

#include <stdlib.h>

typedef struct {
    NESynthNote* note;
    float start, end;
    int pattern_pos, index;
    float cutoff;
} NoteProperties;

static const char* tones[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
static int magnet = 4;
static bool magnet_enabled = true;

#define EDGE_TOLERANCE 8

static void draw_line(int count, int color, float offset, float width) {
    for (int i = 1; i < count; i++) {
        ui_draw_line(width * i / count + offset, AUTO, AUTO, AUTO, color);
    }
}

static void set_magnet(int index, void* data) {
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

static void note_menu(int index, void* data) {
    NESynthNote* note = data;
    switch (index) {
        case 0: // delete
            nesynth_delete_note(note);
            break;
        case 1: // toggle attack
            *nesynth_attack_note(note) ^= 1;
            break;
        case 2: // toggle slide
            *nesynth_slide_note(note) =
                *nesynth_slide_note(note) == *nesynth_base_note(note)
                ? *nesynth_base_note(note) - 4
                : *nesynth_base_note(note);
            break;
        case 3: // make instrument current
            state_select_instrument(*nesynth_note_instrument(note));
            break;
        case 4: // replace with current instrument
            *nesynth_note_instrument(note) = state_instrument();
            break;
        case 5: // snap
            nesynth_set_note_start(note, floorf(nesynth_get_note_start(note) * magnet) / magnet);
            *nesynth_note_length(note) = ceilf(*nesynth_note_length(note) * magnet) / magnet;
            break;
    }
}

static NESynthNote* update_notes(float width, NoteProperties* notes, int num_notes, NESynthNoteType type) {
    static NoteProperties* curr_note = NULL;
    static float drag_offset;
    static enum {
        Drag_ResizeEnd,
        Drag_ResizeStart,
        Drag_Move
    } curr_drag_action;

    int pitch = NESYNTH_NOTE(C, 9) - ui_mouse_y(UI_ItemRelative) / 12;
    float pos = ui_mouse_x(UI_ItemRelative) / width * 4;
    float csnap_pos = magnet_enabled ?  ceilf(pos * magnet) / magnet : pos;
    float fsnap_pos = magnet_enabled ? floorf(pos * magnet) / magnet : pos;
    if (ui_mouse_down() && curr_note) {
        NESynthNote* note = curr_note->note;
        float prev_base = *nesynth_base_note(note);
        float prev_start = nesynth_get_note_start(note);
        float frel_pos = fsnap_pos - curr_note->pattern_pos * 4;
        float crel_pos = csnap_pos - curr_note->pattern_pos * 4;
        switch (curr_drag_action) {
            case Drag_Move: {
                float new_start = nesynth_get_note_start(note) + frel_pos - drag_offset;
                if (new_start < 0) new_start = 0;
                else if (new_start + *nesynth_note_length(note) > 4) new_start = 4 - *nesynth_note_length(note);
                else drag_offset = frel_pos;
                *nesynth_base_note(note) = pitch;
                *nesynth_slide_note(note) += *nesynth_base_note(note) - prev_base;
                nesynth_set_note_start(note, new_start);
            } break;
            case Drag_ResizeStart: {
                float new_start = frel_pos < 0 ? 0 : frel_pos;
                float new_length = *nesynth_note_length(note) - new_start + prev_start;
                if (new_length > 0) {
                    nesynth_set_note_start(note, new_start);
                    *nesynth_note_length(note) = new_length;
                }
            } break;
            case Drag_ResizeEnd: {
                float new_length = crel_pos - nesynth_get_note_start(note);
                if (new_length <= 0) new_length = *nesynth_note_length(note);
                if (crel_pos > 4) new_length = 4 - nesynth_get_note_start(note);
                *nesynth_note_length(note) = new_length;
            } break;
        }
        return note;
    }
    if (curr_note) free(curr_note);
    curr_note = NULL;
    NoteProperties* hover = NULL;
    for (int i = 0; i < num_notes; i++) {
        if (pos >= notes[i].start && pos < notes[i].end && pitch == *nesynth_base_note(notes[i].note)) {
            hover = &notes[i];
        }
    }
    if (ui_clicked()) {
        if (hover) {
            float edge_tolerance = EDGE_TOLERANCE / width * 4;
            curr_note = malloc(sizeof(NoteProperties));
            drag_offset = fsnap_pos - hover->pattern_pos * 4;
            memcpy(curr_note, hover, sizeof(NoteProperties));
            if (pos >= hover->end - edge_tolerance) curr_drag_action = Drag_ResizeEnd;
            else if (pos < hover->start + edge_tolerance) curr_drag_action = Drag_ResizeStart;
            else curr_drag_action = Drag_Move;
            nesynth_set_note_start(hover->note, nesynth_get_note_start(hover->note));
        }
        else {
            drag_offset = fsnap_pos;
            curr_drag_action = Drag_ResizeEnd;
            curr_note = malloc(sizeof(NoteProperties));
            *curr_note = (NoteProperties){
                .note = nesynth_insert_note(
                    nesynth_get_pattern_at(state_channel(), fsnap_pos / 4),
                    type, state_instrument(), pitch, fmodf(fsnap_pos, 4), 1.f / magnet
                ),
                .start = fsnap_pos, .end = csnap_pos + 1.f / magnet,
                .pattern_pos = fsnap_pos / 4, .index = -1,
            };
        }
    }
    return hover ? hover->note : NULL;
}

static NoteProperties* get_notes(float pattern_width, float width, int* num_notes, NESynthNoteType note_type) {
    float start = ui_scroll_x() / pattern_width * 4;
    float end = (ui_scroll_x() + width) / pattern_width * 4;
    NoteProperties* notes = NULL;
    *num_notes = 0;
    for (int i = 0; i < nesynth_song_get_length(state_song()); i++) {
        if (!nesynth_any_pattern_at(state_channel(), i)) continue;
        NESynthPattern* pattern = nesynth_get_pattern_at(state_channel(), i);
        NESynthIter* note_iter = nesynth_iter_notes(pattern, note_type);
        int index = -1;
        while (nesynth_iter_next(note_iter)) {
            index++;
            NESynthNote* note = nesynth_iter_get(note_iter);
            if (start - i * 4 >= nesynth_get_note_start(note) + *nesynth_note_length(note) || end - i * 4 < nesynth_get_note_start(note)) continue;
            if (*num_notes != 0) {
                NoteProperties* prev = &notes[*num_notes - 1];
                prev->cutoff = (nesynth_get_note_start(note) + i * 4 - prev->start) / *nesynth_note_length(prev->note);
                if (prev->cutoff > 1) prev->cutoff = 1;
            }
            (*num_notes)++;
            notes = realloc(notes, sizeof(NoteProperties) * *num_notes);
            notes[*num_notes - 1] = (NoteProperties){
                .note = note,
                .start = i * 4 + nesynth_get_note_start(note),
                .end   = i * 4 + nesynth_get_note_start(note) + *nesynth_note_length(note),
                .index = index,
                .pattern_pos = i,
                .cutoff = 1,
            };
        }
    }
    return notes;
}

void window_piano_roll(float w, float h) {
    int patterns = nesynth_song_get_length(state_song());
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
            if (ui_right_clicked()) ui_menu("1/1\0" "1/2\0" "1/4\0" "1/6\0" "1/8\0" "1/16\0" "1/32\0" "1/64\0", set_magnet, NULL);
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
            if (!ui_inview(width, 32)) {
                ui_dummy(width, 32);
                ui_next();
                continue;
            }
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
                NESynthPattern* pattern = nesynth_any_pattern_at(state_channel(), i) ? nesynth_get_pattern_at(state_channel(), i) : NULL;
                if (pattern) ui_text_positioned(AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, GRAY(255), "Pattern %d", nesynth_get_pattern_id(pattern) + 1);
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
        ui_item(width * patterns, 12*12*9);
            int num_notes = 0;
            NoteProperties* notes = get_notes(width, w - 128, &num_notes, NESynthNoteType_Instrument);
            NESynthNote* hover = update_notes(width, notes, num_notes, NESynthNoteType_Instrument);
            ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(48));
            for (int i = 0; i < 12 * 9; i++) {
                int tone = i % 12;
                if (tone % 2 == (tone < 7)) ui_draw_rectangle(AUTO, i * 12, AUTO, 12, GRAY(32));
                ui_draw_line(AUTO, i * 12 + 11, AUTO, AUTO, GRAY(16));
            }
            for (int i = ui_scroll_x() / width; i < (ui_scroll_x() + w - 128) / width; i++) {
                if (i >= patterns) break;
                ui_draw_rectangle(i * width - 2, AUTO, 3, AUTO, GRAY(16));
                if (ui_zoom() >= 0.25) {
                    draw_line(4, GRAY(16), i * width, width);
                    int num_lines = magnet;
                    if (ui_zoom() < 8 && num_lines > 32) num_lines = 32;
                    if (ui_zoom() < 4 && num_lines > 16) num_lines = 16;
                    if (ui_zoom() >= 2) draw_line(num_lines * 4, GRAYA(16, 0.5), i * width, width);
                }
            }
            for (int i = 0; i < num_notes; i++) {
                float base = *nesynth_base_note(notes[i].note);
                float slide = *nesynth_slide_note(notes[i].note);
                float x = notes[i].start * width / 4;
                float y = (NESYNTH_NOTE(C, 9) - 1 - *nesynth_base_note(notes[i].note)) * 12;
                float w = (notes[i].end - notes[i].start) * width / 4;
                float cutoff = notes[i].cutoff * (w - 1);
                float slide_y = (NESYNTH_NOTE(C, 9) - 1 - *nesynth_slide_note(notes[i].note)) * 12;
                if (slide < base) ui_draw_triangle(x, y + 12, x + w, y + 12, x + w, slide_y + 12, GRAYA(224, 0.25));
                if (slide > base) ui_draw_triangle(x, y, x + w, y, x + w, slide_y, GRAYA(224, 0.25));
                ui_draw_rectangle(x + 0, y - 1, w + 1, 13, notes[i].note == hover ? GRAY(224) : GRAY(16));
                ui_draw_rectangle(x + 1, y + 0, cutoff, 11, GRAY(224));
                ui_draw_rectangle(x + 1 + cutoff, y + 0, ceilf((w - 1) - cutoff), 11, GRAYA(224, 0.25));
                int octave = (base - NESYNTH_NOTE(C, 0)) / 12;
                int tone = roundf((base - NESYNTH_NOTE(C, 0)) - octave * 12);
                if (w > 8) {
                    if (*nesynth_attack_note(notes[i].note)) ui_draw_rectangle(x + 2, y + 1, 4, 9, GRAY(16));
                    else {
                        ui_draw_rectangle(x + 2, y + 1, 4, 1, GRAY(16));
                        ui_draw_rectangle(x + 2, y + 1, 1, 9, GRAY(16));
                        ui_draw_rectangle(x + 2, y + 1 + 8, 4, 1, GRAY(16));
                        ui_draw_rectangle(x + 2 + 3, y + 1, 1, 9, GRAY(16));
                    }
                }
                if (w > 30) ui_text(x + 8, y + 2, GRAY(16), "%s%d", tones[tone], octave);
                if (ui_right_clicked() && notes[i].note == hover) ui_menu("Delete\0Toggle Attack\0Toggle Slide\0Make Instrument Current\0Replace with Current Instrument\0Snap", note_menu, hover);
            }
            free(notes);
        ui_end();
    ui_end();
}
