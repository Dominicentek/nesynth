#include "state.h"
#include "windows/list.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typeof(state) state;

List songs;
List instruments;

static int state_list_add(List* list, void* item, const char* default_name) {
    list->num_items++;
    list->items = realloc(list->items, sizeof(ListItem) * list->num_items);
    ListItem* new = &list->items[list->num_items - 1];
    int length = default_name ? snprintf(NULL, 0, default_name, list->num_items) : 0;
    memset(new, 0, sizeof(ListItem));
    new->item = item;
    new->name = default_name ? malloc(length + 1) : NULL;
    if (new->name) sprintf(new->name, default_name, list->num_items);
    return list->num_items - 1;
}

void state_init() {
    state.synth = nesynth_create(48000);
    state_add_instrument();
    state_add_song();
}

void state_add_instrument() {
    state.instrument = state_list_add(&instruments, nesynth_add_instrument(state.synth), "Instrument %d");
}

void state_add_song() {
    NESynthSong* song = nesynth_add_song(state.synth);
    state.song = state_list_add(&songs, song, "Song %d");
    state.channel = state_list_add(state_list_channels(), nesynth_add_channel(song, NESynthChannelType_Square), NULL);
    *nesynth_song_bpm(song) = 120;
}

void state_add_channel(NESynthChannelType type) {
    state.channel = state_list_add(state_list_channels(), nesynth_add_channel(state_song(), type), (char*[]){
        "Square", "Triangle", "Noise", "Waveform"
    }[type]);
}

static void state_delete(List* list, int* curr, void* item) {
    if (list->num_items <= 1 && curr) return;
    int index = -1;
    for (int i = 0; i < list->num_items; i++) {
        if (list->items[i].item == item) {
            index = i;
            break;
        }
    }
    if (index == -1) return;
    if (curr && *curr == index) *curr = index == 0;
    free(list->items[index].name);
    list->num_items--;
    memmove(list->items + index, list->items + (index + 1), sizeof(ListItem) * (list->num_items - index));
    list->items = realloc(list->items, sizeof(ListItem) * list->num_items);
}

void state_delete_instrument(NESynthInstrument* instrument) {
    state_delete(&instruments, &state.instrument, instrument);
}

void state_delete_song(NESynthSong* song) {
    state_delete(&songs, &state.song, song);
}

void state_delete_channel(NESynthChannel* channel) {
    state_delete(state_list_channels(), &state.channel, channel);
}

void state_delete_pattern(NESynthPattern* pattern) {
    NESynthChannel* channel = state_channel();
    for (int i = 0; i < nesynth_song_get_length(state_song()); i++) {
        if (!nesynth_any_pattern_at(channel, i)) continue;
        if (nesynth_get_pattern_at(channel, i) == pattern) return;
    }
    state_delete(state_list_patterns(), NULL, pattern);
}

NESynthInstrument* state_instrument() {
    return instruments.items[state.instrument].item;
}

NESynthSong* state_song() {
    return songs.items[state.song].item;
}

NESynthChannel* state_channel() {
    return songs.items[state.song].nested_list.items[state.channel].item;
}

List* state_list_channels() {
    return &songs.items[state.song].nested_list;
}

List* state_list_patterns() {
    return &songs.items[state.song].nested_list.items[state.channel].nested_list;
}

static void select_item(List* list, void* item, int* index) {
    for (int i = 0; i < list->num_items; i++) {
        if (list->items[i].item == item) {
            *index = i;
            return;
        }
    }
}

static ListItem* get_item(List* list, void* item) {
    for (int i = 0; i < list->num_items; i++) {
        if (list->items[i].item == item) return &list->items[i];
    }
    return NULL;
}

void state_select_instrument(NESynthInstrument* instrument) {
    select_item(&instruments, instrument, &state.instrument);
}

void state_select_song(NESynthSong* song) {
    select_item(&songs, song, &state.song);
}

void state_select_channel(NESynthChannel* channel) {
    select_item(state_list_channels(), channel, &state.channel);
}

ListItem* state_instrument_item(NESynthInstrument* instrument) {
    return get_item(&instruments, instrument);
}

ListItem* state_song_item(NESynthSong* song) {
    return get_item(&songs, song);
}

ListItem* state_channel_item(NESynthChannel* channel) {
    return get_item(state_list_channels(), channel);
}

ListItem* state_pattern_item(NESynthPattern* pattern) {
    ListItem* item = get_item(state_list_patterns(), pattern);
    if (item) return item;
    int index = state_list_add(state_list_patterns(), pattern, "Pattern %d");
    return &state_list_patterns()->items[index];
}
