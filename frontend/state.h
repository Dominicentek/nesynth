#ifndef STATE_H
#define STATE_H

#include "nesynth.h"
#include "windows/list.h"

extern struct {
    NESynth* synth;
    NESynthInstrument* instrument;
    NESynthSong* song;
    NESynthChannel* channel;
    NESynthNoteType note_type;
} state;

extern List songs;
extern List instruments;

void state_init();
void state_add_instrument();
void state_add_song();
void state_delete(List* list, void* item);

#endif
