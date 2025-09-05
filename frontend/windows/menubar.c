#include "ui.h"

bool item(const char* tex) {
    bool clicked = false;
    ui_item(36, 36);
        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, ui_hovered(true, true) ? GRAY(48) : GRAY(32));
        ui_image(tex, 2, 2, AUTO, AUTO);
        clicked = ui_clicked();
    ui_end();
    return clicked;
}

void window_menubar(float w, float h) {
    item("images/new.png");
    item("images/load.png");
    item("images/save.png");
    item("images/export.png");
    item("images/undo.png");
    item("images/redo.png");
    item("images/copy.png");
    item("images/cut.png");
    item("images/paste.png");
}
