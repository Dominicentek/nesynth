#include "list.h"
#include "state.h"

static void menu_handler(int index, void* data) {
    switch (index) {
        case 0:
            state_delete_song(data);
            break;
    }
}

void window_songs(float w, float h) {
    int song = state.song;
    window_list(w, h, "Songs", &state.songs, &state.song, "Delete\0Rename\0Change Color\0Set BPM\0Set Length\0", menu_handler, state_add_song);
    if (song != state.song) {
        state.note_type = NESynthNoteType_Melodic;
        state.channel = 0;
        nesynth_select_song(state.synth, state_song());
    }
}
