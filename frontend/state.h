#ifndef STATE_H
#define STATE_H

#include "nesynth.h"
#include "windows/list.h"

extern struct {
    NESynth* synth;
    NESynthNoteType note_type;
    int instrument;
    int song;
    int channel;
} state;

extern List songs;
extern List instruments;

void state_init();
void state_add_instrument();
void state_add_song();
void state_add_channel(NESynthChannelType type);
void state_delete_instrument(NESynthInstrument* instrument);
void state_delete_song(NESynthSong* song);
void state_delete_channel(NESynthChannel* channel);
void state_delete_pattern(NESynthPattern* pattern);
NESynthInstrument* state_instrument();
NESynthSong* state_song();
NESynthChannel* state_channel();
List* state_list_channels();
List* state_list_patterns();
void state_select_instrument(NESynthInstrument* instrument);
void state_select_song(NESynthSong* song);
void state_select_channel(NESynthChannel* channel);
ListItem* state_instrument_item(NESynthInstrument* instrument);
ListItem* state_song_item(NESynthSong* song);
ListItem* state_channel_item(NESynthChannel* channel);
ListItem* state_pattern_item(NESynthPattern* pattern);
void state_move(List* list, int from, int to);

#endif
