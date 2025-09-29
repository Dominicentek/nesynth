#include "ui.h"
#include "state.h"

static const char* channel_names[] = {
    "Square",
    "Triangle",
    "Noise",
    "Waveform"
};

static void add_channel(int type, void* data) {
    state_add_channel(type);
}

static void channel_menu(int item, void* channel) {
    switch (item) {
        case 0: // delete
            state_delete_channel(channel);
            break;
    }
}

static void set_points(int item, void* pattern_id) {
    int id = (uintptr_t)pattern_id;
    switch (item) {
        case 0: // set loop point
            *nesynth_song_loop_point(state_song()) = id;
            break;
    }
}

static void modify_pattern(int item, void* ptr_data) {
    int data = (uintptr_t)ptr_data;
    int pattern = (data >> 16) & 0xFFFF;
    int channel = (data >>  0) & 0xFFFF;
    switch (item) {
        case 0:
            nesynth_assign_pattern(state_list_channels()->items[channel].item, -1, pattern);
            break;
    }
}

void draw_channel_type(NESynthChannelType type, float height) {
    int noise_data[] = { 13, 12, 15, 1, 14, 12, 11, 11, 12, 10, 0, 10, 12, 15, 12, 7, 2, 14, 7, 6, 13, 14, 4, 14, 15, 11, 11, 5, 14, 4, 15, 0 };
    int wavef_data[] = { 9, 2, 2, 14, 9, 6, 14, 4, 5, 13, 7, 3, 13, 11, 13, 4, 11, 2, 10, 12, 2, 2, 6, 6, 0, 1, 0, 13, 1, 9, 0, 0 };
    switch (type) {
        case NESynthChannelType_Square:
            for (int i = 0; i < 4; i++) {
                ui_draw_rectangle(i * 32 + 0, height - 6, 16, 2, GRAYA(255, 0.25));
                ui_draw_rectangle(i * 32 + 16, height - 18, 2, 14, GRAYA(255, 0.25));
                ui_draw_rectangle(i * 32 + 16, height - 20, 16, 2, GRAYA(255, 0.25));
                ui_draw_rectangle(i * 32 + 32, height - 20, 2, 14, GRAYA(255, 0.25));
            }
            break;
        case NESynthChannelType_Triangle:
            for (int i = 0; i < 4; i++) {
                ui_draw_mesh(GRAYA(255, 0.25), 6, (float[]){
                    i * 32 + 0,  height - 4,
                    i * 32 + 16, height - 19,
                    i * 32 + 32, height - 4,
                    i * 32 + 0,  height - 7,
                    i * 32 + 16, height - 22,
                    i * 32 + 32, height - 7,
                }, 12, (int[]){
                    0, 3, 4,
                    0, 1, 4,
                    2, 5, 4,
                    2, 1, 4,
                });
            }
            break;
        case NESynthChannelType_Noise:
            for (int i = 0; i < 32; i++) {
                if (i != 0) {
                    float from = noise_data[i - 1] + height - 20;
                    float to = noise_data[i] + height - 20;
                    if (to > from) to   += 2;
                    else           from += 2;
                    ui_draw_rectangle(i * 4 + 0, from, 2, to - from, GRAYA(255, 0.25));
                }
                float x = i * 4 + (i == 0 ? 0 : 2);
                float w = i == 0 ? 4 : 2;
                ui_draw_rectangle(x, noise_data[i] + height - 20, w, 2, GRAYA(255, 0.25));
            }
            break;
        case NESynthChannelType_Waveform:
            for (int i = 0; i < 32; i++) {
                ui_draw_rectangle(i * 4, wavef_data[i] + height - 20, 3, 20, GRAYA(255, 0.25));
            }
            break;
    }
}

void window_patterns(float w, float h) {
    int patterns = nesynth_song_get_length(state_song());
    int channels = nesynth_num_channels(state_song());
    float channel_height = round(channels == 0 ? 0 : fmax((h - 16) / channels, 32));
    float curr_pos = 0;
    ui_middleclick();
    ui_update_zoom(128);
    ui_limit_scroll(0, 0, ui_zoom() * 160 * patterns + 128, (int)(channel_height * channels + 16) - 1);
    ui_setup_offset(false, false);
    ui_item(112, 16);
        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(32));
        ui_text_aligned(AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, 1, GRAY(255), "Channels");
    ui_end();
    ui_item(16, 16);
        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, ui_hovered(true, true) ? GRAY(48) : GRAY(32));
        ui_text(4, 4, 1, GRAY(255), "+");
        if (ui_clicked()) ui_menu("Square\0Triangle\0Noise\0Waveform\0", add_channel, NULL);
    ui_end();
    if (patterns == 0) return;
    ui_subwindow(w - 128, 16);
        ui_setup_offset(true, false);
        for (int i = 0; i < patterns; i++) {
            ui_item(ui_zoom() * 160, 16);
                int color = ui_hovered(true, false) ? 64 : i % 2 ? 32 : 48;
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(color));
                ui_text_aligned(AUTO, AUTO, AUTO, AUTO, 0.5, 0, 0, 4, 1, GRAY(255), "%d", i + 1);
                if (*nesynth_song_loop_point(state_song()) == i) ui_text(4, 4, 1, GRAY(255), "v");
                if (ui_right_clicked()) ui_menu("Set Loop Point\0Toggle Transition Point\0", set_points, (void*)(uintptr_t)i);
                if (ui_clicked()) *nesynth_song_loop_point(state_song()) = i;
            ui_end();
        }
    ui_end();
    ui_next();
    ui_subwindow(128, h - 16);
        ui_setup_offset(false, true);
        curr_pos = 0;
        List* channel_list = state_list_channels();
        for (int i = 0; i < channel_list->num_items; i++) {
            NESynthChannel* channel = channel_list->items[i].item;
            ui_item(128, fmodf(curr_pos + channel_height, 1) < 0.5 ? floorf(channel_height) : ceilf(channel_height));
                ui_dragndrop(ui_idptr(channel));
                if (ui_is_dragndropped()) {
                    List* channels = state_list_channels();
                    int to = (ui_mouse_y(UI_ParentRelative) + *ui_scroll_y()) / channel_height;
                    if (to < 0) to = 0;
                    if (to >= channel_list->num_items) to = channel_list->num_items - 1;
                    state_move(state_list_channels(), i, to);
                }
                int color = ui_hovered(false, true) && !ui_is_dragndropped() ? 64 : 32;
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(color));
                draw_channel_type(nesynth_get_channel_type(channel), channel_height);
                ui_text_aligned(AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, 1, GRAY(255), "%s", channel_names[nesynth_get_channel_type(channel)]);
                if (ui_clicked()) state_select_channel(channel);
                if (ui_right_clicked()) ui_menu("Delete\0Rename\0Toggle Mute\0Toggle Solo\0Force Display in Piano Roll\0", channel_menu, channel);
            ui_end();
            ui_next();
            curr_pos += channel_height;
        }
    ui_end();
    ui_limit_scroll(0, 0, ui_zoom() * 160 * patterns + 128, (int)(channel_height * channels + 16) - 1);
    ui_subwindow(w - 128, h - 16);
        ui_setup_offset(true, true);
        curr_pos = 0;
        for (int c = 0; c < channel_list->num_items; c++) {
            NESynthChannel* channel = channel_list->items[c].item;
            for (int i = 0; i < patterns; i++) {
                int color = i % 2 ? 32 : 48;
                ui_item(ui_zoom() * 160, fmodf(curr_pos + channel_height, 1) < 0.5 ? floorf(channel_height) : ceilf(channel_height));
                    ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(color));
                    if (ui_clicked()) state_select_channel(channel);
                    if (nesynth_any_pattern_at(channel, i) || ui_clicked()) {
                        NESynthPattern* pattern = nesynth_get_pattern_at(channel, i);
                        int id = nesynth_get_pattern_id(pattern);
                        float h = (float)id / nesynth_num_unique_patterns(channel);
                        int color = state_pattern_item(pattern)->color;
                        int faded = (color & 0xFFFFFF00) | 0x7F;
                        ui_draw_rectangle(AUTO, 0, AUTO, 16, color);
                        ui_draw_rectangle(AUTO, 16, AUTO, AUTO, faded);
                        ui_text_aligned(AUTO, 0, AUTO, 16, AUTO, AUTO, AUTO, AUTO, 1, GRAY(0), "Pattern %d", id + 1);
                        if (ui_right_clicked()) ui_menu("Delete\0Change Color\0", modify_pattern, (void*)(uintptr_t)(i << 16 | c));
                    }
                ui_end();
            }
            ui_next();
            curr_pos += channel_height;
        }
    ui_end();
}
