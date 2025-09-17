#include "list.h"
#include "state.h"

static void menu_handler(int index, void* data) {
    switch (index) {
        case 0: {
            if (instruments.num_items == 1) return;
            if (state.instrument == data) state.instrument = instruments.items[instruments.items[0].item == data ? 1 : 0].item;
            nesynth_delete_instrument(data);
            state_delete(&instruments, data);
        }
    }
}

void window_instruments(float w, float h) {
    window_list(w, h, "Instruments", &instruments, &state.instrument, "Delete\0", menu_handler, state_add_instrument);
}
