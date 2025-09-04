#include "list.h"

#include <stdlib.h>

static ListItem instruments[] = {
    { .name = "Instrument 1",  .item = (void*)17 },
    { .name = "Instrument 2",  .item = (void*)18 },
    { .name = "Instrument 3",  .item = (void*)19 },
    { .name = "Instrument 4",  .item = (void*)20 },
    { .name = "Instrument 5",  .item = (void*)21 },
    { .name = "Instrument 6",  .item = (void*)22 },
    { .name = "Instrument 7",  .item = (void*)23 },
    { .name = "Instrument 8",  .item = (void*)24 },
    { .name = "Instrument 9",  .item = (void*)25 },
    { .name = "Instrument 10", .item = (void*)26 },
    { .name = "Instrument 11", .item = (void*)27 },
    { .name = "Instrument 12", .item = (void*)28 },
};
static void* selected = (void*)1;
static int size = sizeof(instruments) / sizeof(*instruments);

static void* create_instrument() {
    return NULL;
}

void window_instruments(float w, float h) {
    ListItem* items = instruments;
    window_list(w, h, "Instruments", &items, &size, &selected, create_instrument);
}
