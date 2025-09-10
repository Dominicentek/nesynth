#include "list.h"
#include "state.h"

void window_instruments(float w, float h) {
    window_list(w, h, "Instruments", &instruments, &state.instrument, state_add_instrument);
}
