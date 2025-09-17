#include "list.h"
#include "state.h"

static void menu_handler(int index, void* data) {
    switch (index) {
        case 0: {
            if (songs.num_items == 1) return;
            if (state.song == data) {
                state.song = songs.items[songs.items[0].item == data ? 1 : 0].item;
                state.channel = nesynth_get_channel(state.song, 0);
            }
            nesynth_delete_song(data);
            state_delete(&songs, data);
        }
    }
}

void window_songs(float w, float h) {
    NESynthSong* song = state.song;
    window_list(w, h, "Songs", &songs, &state.song, "Delete\0", menu_handler, state_add_song);
    if (song != state.song) state.channel = nesynth_get_channel(state.song, 0);
}
