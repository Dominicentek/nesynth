#include "list.h"
#include "state.h"

static void menu_handler(int index, void* data) {
    switch (index) {
        case 0:
            state_delete_instrument(data);
            break;
    }
}

void window_instruments(float w, float h) {
    window_list(w, h, "Instruments", &instruments, &state.instrument, "Delete\0", menu_handler, state_add_instrument);
}
