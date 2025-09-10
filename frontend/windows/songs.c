#include "list.h"
#include "state.h"

void window_songs(float w, float h) {
    window_list(w, h, "Songs", &songs, state.song, state_add_song);
}
