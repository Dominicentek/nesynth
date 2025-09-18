#include "list.h"
#include "ui.h"
#include "state.h"

void window_list(float w, float h, const char* title, List* list, int* selected, const char* menu, void(*menu_handler)(int item, void* data), void(*create_item)()) {
    ui_scrollwheel();
    ui_setup_offset(false, false);
    ui_limit_scroll(0, 0, 0, 16 + list->num_items * 16);
    ui_item(w - 16, 16);
        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(32));
        ui_text_positioned(AUTO, AUTO, AUTO, AUTO, 0.5, 0, 0, 4, GRAY(255), title);
    ui_end();
    ui_item(16, 16);
        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, ui_hovered(true, true) ? GRAY(48) : GRAY(32));
        ui_text_positioned(AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, GRAY(255), "+");
        if (ui_clicked()) create_item();
    ui_end();
    ui_next();
    ui_subwindow(w, h - 16);
        ui_flow(UIFlow_LeftToRight);
        ui_setup_offset(false, true);
        for (int i = 0; i < list->num_items; i++) {
            ui_item(w, 16);
                if (ui_clicked()) *selected = i;
                if (ui_right_clicked()) ui_menu(menu, menu_handler, list->items[i].item);
                ui_dragndrop(ui_idptr(list->items[i].item));
                if (ui_is_dragndropped()) {
                    int new_i = (ui_mouse_y(UI_ParentRelative) + ui_scroll_y()) / 16;
                    if (new_i < 0) new_i = 0;
                    if (new_i >= list->num_items) new_i = list->num_items - 1;
                    state_move(list, i, new_i);
                }
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, list->items[i].color);
                if (i == *selected) ui_text(4, 4, GRAY(16), ">");
                ui_text(16, 4, GRAY(16), list->items[i].name);
            ui_end();
        }
        ui_item(w, h);
            ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(32));
        ui_end();
    ui_end();
}
