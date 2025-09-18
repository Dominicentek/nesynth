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
        case 0:
            state_delete_channel(channel);
            break;
    }
}

void window_patterns(float w, float h) {
    NESynthIter* iter;
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
        ui_text_positioned(AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, GRAY(255), "Channels");
    ui_end();
    ui_item(16, 16);
        ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, ui_hovered(true, true) ? GRAY(48) : GRAY(32));
        ui_text(4, 4, GRAY(255), "+");
        if (ui_clicked()) ui_menu("Square\0Triangle\0Noise\0Waveform\0", add_channel, NULL);
    ui_end();
    if (patterns == 0) return;
    ui_subwindow(w - 128, 16);
        ui_setup_offset(true, false);
        for (int i = 0; i < patterns; i++) {
            ui_item(ui_zoom() * 160, 16);
                int color = ui_hovered(true, false) ? 64 : i % 2 ? 32 : 48;
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(color));
                ui_text_positioned(AUTO, AUTO, AUTO, AUTO, 0.5, 0, 0, 4, GRAY(255), "%d", i + 1);
            ui_end();
        }
    ui_end();
    ui_next();
    ui_subwindow(128, h - 16);
        ui_setup_offset(false, true);
        iter = nesynth_iter_channels(state_song());
        curr_pos = 0;
        while (nesynth_iter_next(iter)) {
            NESynthChannel* channel = nesynth_iter_get(iter);
            ui_item(128, fmodf(curr_pos + channel_height, 1) < 0.5 ? floorf(channel_height) : ceilf(channel_height));
                ui_dragndrop(ui_idptr(channel));
                if (ui_is_dragndropped()) {

                }
                int color = ui_hovered(false, true) && !ui_is_dragndropped() ? 64 : 32;
                ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(color));
                ui_text_positioned(AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, AUTO, GRAY(255), "%s", channel_names[nesynth_get_channel_type(nesynth_iter_get(iter))]);
                if (ui_clicked()) state_select_channel(channel);
                if (ui_right_clicked()) ui_menu("Delete\0Toggle Mute\0Toggle Solo\0Force Display in Piano Roll\0", channel_menu, channel);
            ui_end();
            ui_next();
            curr_pos += channel_height;
        }
    ui_end();
    ui_subwindow(w - 128, h - 16);
        ui_setup_offset(true, true);
        iter = nesynth_iter_channels(state_song());
        curr_pos = 0;
        while (nesynth_iter_next(iter)) {
            NESynthChannel* channel = nesynth_iter_get(iter);
            for (int i = 0; i < patterns; i++) {
                int color = i % 2 ? 32 : 48;
                ui_item(ui_zoom() * 160, fmodf(curr_pos + channel_height, 1) < 0.5 ? floorf(channel_height) : ceilf(channel_height));
                    ui_draw_rectangle(AUTO, AUTO, AUTO, AUTO, GRAY(color));
                    if (ui_clicked()) state_select_channel(channel);
                    if (nesynth_any_pattern_at(channel, i) || ui_clicked()) {
                        NESynthPattern* pattern = nesynth_get_pattern_at(channel, i);
                        int id = nesynth_get_pattern_id(pattern);
                        float h = (float)id / nesynth_num_unique_patterns(channel);
                        ui_draw_rectangle(AUTO, 0, AUTO, 16, HSV(h, 1, 1));
                        ui_draw_rectangle(AUTO, 16, AUTO, AUTO, HSVA(h, 1, 1, 0.5));
                        ui_text_positioned(AUTO, 0, AUTO, 16, AUTO, AUTO, AUTO, AUTO, GRAY(0), "Pattern %d", id + 1);
                    }
                ui_end();
            }
            ui_next();
            curr_pos += channel_height;
        }
    ui_end();
}
