#include "state.h"
#include "windows/list.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typeof(state) state;

List songs;
List instruments;

static void state_list_add(List* list, void* item, const char* default_name) {
    list->num_items++;
    list->items = realloc(list->items, sizeof(ListItem) * list->num_items);
    ListItem* new = &list->items[list->num_items - 1];
    int length = snprintf(NULL, 0, default_name, list->num_items);
    new->item = item;
    new->name = malloc(length + 1);
    sprintf(new->name, default_name, list->num_items);
}

void state_init() {
    state.synth = nesynth_create(48000);
    state_add_instrument();
    state_add_song();
}

void state_add_instrument() {
    state.instrument = nesynth_add_instrument(state.synth);
    state_list_add(&instruments, state.instrument, "Instrument %d");
}

void state_add_song() {
    state.song = nesynth_add_song(state.synth);
    state.channel = nesynth_add_channel(state.song, NESynthChannelType_Square);
    *nesynth_song_bpm(state.song) = 120;
    state_list_add(&songs, state.song, "Song %d");
}

void state_delete(List* list, void* item) {
    int index = -1;
    for (int i = 0; i < list->num_items; i++) {
        if (list->items[i].item == item) {
            index = i;
            break;
        }
    }
    if (index == -1) return;
    free(list->items[index].name);
    list->num_items--;
    memmove(list->items + index, list->items + (index + 1), sizeof(ListItem) * (list->num_items - index));
    list->items = realloc(list->items, sizeof(ListItem) * list->num_items);
}
