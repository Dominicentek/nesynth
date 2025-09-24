#include "list.h"
#include "state.h"

static void menu_handler(int index, void* data) {
    switch (index) {
        case 0:
            state_delete_instrument(data);
            break;
        case 3:
            state_select_instrument(data);
            state.song = -1;
            state.note_type = NESynthNoteType_Volume;
            break;
        case 4:
            state_select_instrument(data);
            state.song = -1;
            state.note_type = NESynthNoteType_Pitch;
            break;
    }
}

void window_instruments(float w, float h) {
    window_list(w, h, "Instruments", &instruments, &state.instrument, "Delete\0Rename\0Change Color\0Volume Envelopes\0Pitch Envelopes\0Upload Samples\0", menu_handler, state_add_instrument);
}
