#include "ui.h"

#include "shortcut.h"
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
        case 0: activate_shortcut(SHIFT + SPACE);        break; // play from start
        case 1: activate_shortcut(CTRL + SPACE);         break; // play from pattern
        case 2: activate_shortcut(CTRL + SHIFT + SPACE); break; // play from loop point
    }
}

static void menu_rewind(int index, void* data) {
    activate_shortcut(CTRL + SHIFT + R);
    *nesynth_beat_position(state.synth) = floorf(*nesynth_beat_position(state.synth) / 4) * 4;
}

static void menu_save_as(int index, void* data) {
    activate_shortcut(CTRL + SHIFT + S);
}

void window_menubar(float w, float h) {
    if (item("images/new.png", NULL, NULL))                 activate_shortcut(CTRL + N);
    if (item("images/load.png", NULL, NULL))                activate_shortcut(CTRL + O);
    if (item("images/save.png", "Save As\0", menu_save_as)) activate_shortcut(CTRL + S);
    if (item("images/export.png", NULL, NULL))              activate_shortcut(CTRL + E);
    if (item("images/undo.png", NULL, NULL))                activate_shortcut(CTRL + Z);
    if (item("images/redo.png", NULL, NULL))                activate_shortcut(CTRL + Y);
    if (item("images/copy.png", NULL, NULL))                activate_shortcut(CTRL + C);
    if (item("images/cut.png", NULL, NULL))                 activate_shortcut(CTRL + X);
    if (item("images/paste.png", NULL, NULL))               activate_shortcut(CTRL + V);
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
    if (item(
        state.playing ? "images/pause.png" : "images/play.png",
        state.playing ? NULL : "Play from Start\0Play from Pattern\0Play from Loop Point\0", menu_play
    )) activate_shortcut(SPACE);
    if (item("images/rewind.png", "Rewind to Pattern\0", menu_rewind))
        activate_shortcut(CTRL + R);
}
