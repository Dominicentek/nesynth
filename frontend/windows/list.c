#include "list.h"
#include "ui.h"

static void arrmove(void* arr, size_t from, size_t to, size_t size) {
    char item[size];
    memcpy(item, (char*)arr + size * from, size);
    if (from == to) return;
    if (from < to) memmove((char*)arr + size * from, (char*)arr + size * (from + 1), size * (to - from));
    if (from > to) memmove((char*)arr + size * (to + 1), (char*)arr + size * to, size * (from - to));
    memcpy((char*)arr + size * to, item, size);
}

void window_list(float w, float h, const char* title, ListItem** arr, int* size, void** selected, void*(*create_item)()) {
    ui_scrollwheel();
    ui_setup_offset(false, false);
    ui_item(w - 16, 16);
        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, RGB(32, 32, 32));
        ui_text_positioned(AUTO, AUTO, AUTO, AUTO, 0.5, 0, 0, 4, RGB(255, 255, 255), title);
    ui_end();
    ui_item(16, 16);
        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, ui_hovered(true, true) ? RGB(48, 48, 48) : RGB(32, 32, 32));
        ui_text_positioned(AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, RGB(255, 255, 255), "+");
    ui_end();
    ui_next();
    ui_subwindow(w, h - 16);
        ui_flow(UIFlow_LeftToRight);
        ui_setup_offset(false, true);
        for (int i = 0; i < *size; i++) {
            ui_item(w, 16);
                if (ui_clicked()) *selected = (*arr)[i].item;
                if (ui_right_clicked()) ui_menu("Edit\0Rename\0Delete\0", NULL);
                ui_dragndrop(ui_idptr((*arr)[i].item));
                if (ui_is_dragndropped()) {
                    int new_i = ui_mouse_y(UI_ParentRelative) / 16;
                    if (new_i < 0) new_i = 0;
                    if (new_i >= *size) new_i = *size - 1;
                    arrmove(*arr, i, new_i, sizeof(ListItem));
                }
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, ui_hsv((float)i / *size, 1, 1));
                ui_text(4, 4, (*arr)[i].item == *selected ? RGB(255, 255, 255) : RGB(16, 16, 16), (*arr)[i].name);
            ui_end();
        }
        ui_item(w, h);
            ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, RGB(32, 32, 32));
        ui_end();
    ui_end();
}
