#include "ui.h"

#include "state.h"

bool item(const char* tex, const char* menu, void(*callback)(int item, void* data)) {
    bool clicked = false;
    ui_item(36, 36);
        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, ui_hovered(true, true) ? GRAY(48) : GRAY(32));
        ui_image(tex, 2, 2, AUTO, AUTO);
        if (menu && ui_right_clicked()) ui_menu(menu, callback, NULL);
        clicked = ui_clicked();
    ui_end();
    return clicked;
}

static void menu_play(int index, void* data) {
    state.playing = true;
    float* pos = nesynth_beat_position(state.synth);
    switch (index) {
        case 0: *pos = 0;                                          break; // play from start
        case 1: *pos = floorf(*pos / 4) * 4;                       break; // play from pattern
        case 2: *pos = *nesynth_song_loop_point(state_song()) * 4; break; // play from loop point
    }
}

static void menu_rewind(int index, void* data) {
    *nesynth_beat_position(state.synth) = floorf(*nesynth_beat_position(state.synth) / 4) * 4;
}

void window_menubar(float w, float h) {
    item("images/new.png", NULL, NULL);
    item("images/load.png", NULL, NULL);
    item("images/save.png", "Save As\0", NULL);
    item("images/export.png", NULL, NULL);
    item("images/undo.png", NULL, NULL);
    item("images/redo.png", NULL, NULL);
    item("images/copy.png", NULL, NULL);
    item("images/cut.png", NULL, NULL);
    item("images/paste.png", NULL, NULL);
    ui_item(128, 36);
        float time = nesynth_tell(state.synth);
        int min = (int)floorf(time / 60);
        int sec = (int)floorf(time) % 60;
        int mil = (int)floorf(time * 1000) % 1000;
        ui_draw_rectangle(0, 0, 127, 35, GRAY(224));
        ui_draw_rectangle(2, 2, 123, 31, GRAY(16));
        ui_text_aligned(AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, 2, 2, 2, GRAY(64), "%02d:%02d.%03d", min, sec, mil);
        ui_text_aligned(AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, 0, 0, 2, GRAY(224), "%02d:%02d.%03d", min, sec, mil);
    ui_end();
    ui_item(128, 36);
        ui_draw_rectangle(0, 0, 127, 35, GRAY(224));
        ui_draw_rectangle(2, 2, 123, 31, GRAY(16));
    ui_end();
    if (item(
        state.playing ? "images/pause.png" : "images/play.png",
        state.playing ? NULL : "Play from Start\0Play from Pattern\0Play from Loop Point\0", menu_play
    )) state.playing ^= 1;
    if (item("images/rewind.png", "Rewind to Pattern\0", menu_rewind))
        *nesynth_beat_position(state.synth) = 0;
}
