#include "nesynth.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// == DEFINITIONS ==

typedef struct NESynthLinkedList {
    struct NESynthLinkedList *next, *prev;
    void* item;
} NESynthLinkedList;

typedef struct {
    float pos;
    float base;
    float slide;
} NESynthNode;

struct NESynthNodeTable {
    float loop_point;
    NESynthTimescale timescale;
    NESynthLinkedList* nodes;
};

struct NESynth {
    NESynthLinkedList* songs;
    NESynthLinkedList* instruments;
    NESynthSong* curr_song;
    float position;
    int sample_rate;
    bool fade, prev_fade;
    float fade_timer;
};

struct NESynthInstrument {
    NESynth* parent;
    NESynthNodeTable* volume;
    NESynthNodeTable* pitch;
    float* samples;
    float tune;
    int num_samples;
    bool loop_samples;
};

struct NESynthSong {
    NESynth* parent;
    NESynthLinkedList* channels;
    int length;
    float loop_point;
    float bpm;
};

struct NESynthChannel {
    NESynthSong* parent;
    NESynthLinkedList* patterns;
    NESynthPattern** pattern_layout;
    NESynthChannelType type;
    struct {
        uint32_t synth_state;
        float phase;
        float attack;
        NESynthNote* prev_note;
        struct {
            float value;
            float prev_pos;
            int curr_pattern;
            NESynthLinkedList* cursor;
            NESynthNote* curr_note;
        } notes[3];
    } state;
};

struct NESynthPattern {
    NESynthChannel* parent;
    NESynthLinkedList* notes[3];
};

struct NESynthNote {
    NESynthPattern* parent;
    NESynthInstrument* instrument;
    NESynthNoteType type;
    float start;
    float length;
    float base_note;
    float slide_note;
    bool attack;
};

struct NESynthIter {
    int index;
    NESynthLinkedList* element;
    NESynthLinkedList* first;
};

typedef bool(*LinkedListQuery)(void* item);

static NESynthLinkedList* linkedlist_create();
static bool linkedlist_empty(NESynthLinkedList* list);
static void linkedlist_add(NESynthLinkedList* list, void* value);
static void linkedlist_del(NESynthLinkedList* list, void* value);
static bool linkedlist_contains(NESynthLinkedList* list, void* value);
static int linkedlist_length(NESynthLinkedList* list);
static int linkedlist_indexof(NESynthLinkedList* list, void* value);
static void* linkedlist_index(NESynthLinkedList* list, int index);
static void linkedlist_clear(NESynthLinkedList* list);
static void linkedlist_destroy(NESynthLinkedList* list);

#define linkedlist_foreach(list, ...) ({ \
    NESynthLinkedList* __frst = list; \
    NESynthLinkedList* __curr = __frst; \
    if (!linkedlist_empty(list)) do { \
        NESynthLinkedList* __next = __curr->next; \
        void* curr = __curr->item; \
        __VA_ARGS__; \
        __curr = __next; \
    } \
    while (__frst != __curr); \
})

#define linkedlist_last() (__next == __frst)

#define linkedlist_delete() ({ \
    if (__curr->next == __curr) { \
        __curr->prev = __curr->next = __curr->item = NULL; \
        break; \
    } \
    else if (__curr == __frst) { \
        __curr->item = __curr->next->item; \
        __curr->next->next->prev = __curr; \
        __curr->next = __curr->next->next; \
        free(__next); \
        __next = __curr->next; \
    } \
    else { \
        __curr->next->prev = __curr->prev; \
        __curr->prev->next = __curr->next; \
        free(__curr); \
    } \
    linkedlist_continue(); \
})

#define linkedlist_continue() ({ \
    __curr = __next; \
    continue; \
})

static NESynthNodeTable* nesynth_create_nodetable(float default_value);

static void nesynth_destroy_instrument(NESynthInstrument* instrument);
static void nesynth_destroy_song(NESynthSong* song);
static void nesynth_destroy_channel(NESynthChannel* channel);
static void nesynth_destroy_pattern(NESynthPattern* pattern);
static void nesynth_destroy_note(NESynthNote* note);
static void nesynth_destroy_nodetable(NESynthNodeTable* note);

// == LINKED LIST ==

static NESynthLinkedList* linkedlist_create() {
    return calloc(sizeof(NESynthLinkedList), 1);
}

static bool linkedlist_empty(NESynthLinkedList* list) {
    return !list->prev;
}

static void linkedlist_add(NESynthLinkedList* list, void* value) {
    if (linkedlist_empty(list)) {
        list->prev = list;
        list->next = list;
        list->item = value;
        return;
    }
    NESynthLinkedList* item = linkedlist_create();
    item->item = value;
    item->prev = list->prev;
    item->next = list;
    list->prev->next = item;
    list->prev = item;
}

static int linkedlist_add_sorted(NESynthLinkedList* list, void* value, int(*comp)(void*, void*)) {
    int index = 0;
    linkedlist_foreach(list, {
        if (comp(value, curr) < 0) {
            NESynthLinkedList* item = linkedlist_create();
            if (index == 0) {
                item->item = __curr->item;
                item->next = __curr->next;
                item->prev = __curr;
                __curr->next->prev = item;
                __curr->next = item;
                __curr->item = value;
            }
            else {
                item->item = value;
                item->prev = __curr->prev;
                item->next = __curr;
                __curr->prev->next = item;
                __curr->prev = item;
            }
            return index;
        }
        index++;
    });
    linkedlist_add(list, value);
    return index;
}

static void linkedlist_del(NESynthLinkedList* list, void* value) {
    if (linkedlist_empty(list)) return;
    linkedlist_foreach(list, {
        if (curr == value) linkedlist_delete();
    });
}

static bool linkedlist_contains(NESynthLinkedList* list, void* value) {
    return linkedlist_indexof(list, value) != -1;
}

static int linkedlist_length(NESynthLinkedList* list) {
    if (linkedlist_empty(list)) return 0;
    int length = 0;
    linkedlist_foreach(list, length++);
    return length;
}

static int linkedlist_indexof(NESynthLinkedList* list, void* value) {
    if (linkedlist_empty(list)) return -1;
    int index = 0;
    linkedlist_foreach(list, {
        if (curr == value) return index;
        index++;
    });
    return -1;
}

static void* linkedlist_index(NESynthLinkedList* list, int index) {
    if (linkedlist_empty(list)) return NULL;
    linkedlist_foreach(list,
        if (index == 0) return curr;
        index--;
    );
    return NULL;
}

static void* linkedlist_search(NESynthLinkedList* list, LinkedListQuery func) {
    if (linkedlist_empty(list)) return NULL;
    linkedlist_foreach(list, {
        if (func(curr)) return curr;
    });
    return NULL;
}

static void linkedlist_clear(NESynthLinkedList* list) {
    if (linkedlist_empty(list)) return;
    NESynthLinkedList* frst = list;
    NESynthLinkedList* curr = list;
    do {
        NESynthLinkedList* next = curr->next;
        if (curr != frst) free(curr);
        curr = next;
    }
    while (curr != frst);
    list->prev = list->next = NULL;
}

static void linkedlist_destroy(NESynthLinkedList* list) {
    linkedlist_clear(list);
    free(list);
}

// == MIDI TO FREQUENCY ==

static float nesynth_default_freq_mod(float freq) {
    return freq;
}

float nesynth_midi2freq(float midi, NESynthFreqMod freq_mod) {
    if (!freq_mod) freq_mod = nesynth_default_freq_mod;
    return freq_mod(NESYNTH_CONCERT_PITCH_FREQ * pow(2, (midi - NESYNTH_CONCERT_PITCH) / NESYNTH_EDO));
}

float nesynth_freq2midi(float freq, NESynthFreqMod freq_mod) {
    if (!freq_mod) freq_mod = nesynth_default_freq_mod;
    return log2f(freq_mod(freq) / NESYNTH_CONCERT_PITCH_FREQ) * NESYNTH_EDO + NESYNTH_CONCERT_PITCH;
}

// == MANAGEMENT FUNCTIONS ==

static NESynthNodeTable* nesynth_create_nodetable(float default_value) {
    NESynthNodeTable* nodetable = calloc(sizeof(NESynthNodeTable), 1);
    nodetable->loop_point = NESYNTH_NO_LOOP;
    nodetable->nodes = linkedlist_create();
    nesynth_nodetable_insert(nodetable, 0, default_value);
    return nodetable;
}

NESynth* nesynth_create(int sample_rate) {
    NESynth* synth = calloc(sizeof(NESynth), 1);
    synth->sample_rate = sample_rate;
    synth->songs = linkedlist_create();
    synth->instruments = linkedlist_create();
    return synth;
}

void nesynth_destroy(NESynth* synth) {
    linkedlist_foreach(synth->songs, nesynth_destroy_song(curr));
    linkedlist_foreach(synth->instruments, nesynth_destroy_instrument(curr));
    linkedlist_destroy(synth->songs);
    linkedlist_destroy(synth->instruments);
    free(synth);
}

void nesynth_select_song(NESynth* synth, int song) {
    synth->curr_song = linkedlist_index(synth->songs, song);
    synth->position = 0;
}

void nesynth_seek(NESynth* synth, float sec) {
    if (!synth->curr_song) return;
    synth->position = sec / 60 * synth->curr_song->bpm;
}

float nesynth_tell(NESynth* synth) {
    if (!synth->curr_song) return 0;
    return synth->position / (synth->curr_song->bpm / 60);
}

float nesynth_get_length(NESynth* synth) {
    if (!synth->curr_song) return 0;
    return synth->curr_song->length * 4 / (synth->curr_song->bpm / 60);
}

int nesynth_num_instruments(NESynth* synth) {
    return linkedlist_length(synth->instruments);
}

NESynthInstrument* nesynth_add_instrument(NESynth* synth) {
    NESynthInstrument* instrument = calloc(sizeof(NESynthInstrument), 1);
    instrument->parent = synth;
    instrument->tune = NESYNTH_CONCERT_PITCH;
    instrument->volume = nesynth_create_nodetable(1);
    instrument->pitch = nesynth_create_nodetable(0);
    linkedlist_add(synth->instruments, instrument);
    return instrument;
}

NESynthInstrument* nesynth_get_instrument(NESynth* synth, int index) {
    return linkedlist_index(synth->instruments, index);
}

void nesynth_delete_instrument(NESynthInstrument* instrument) {
    linkedlist_foreach(instrument->parent->songs, {
        linkedlist_foreach(((NESynthSong*)curr)->channels, {
            linkedlist_foreach(((NESynthPattern*)curr)->notes[NESynthNoteType_Instrument], {
                if (((NESynthNote*)curr)->instrument == instrument) {
                    nesynth_destroy_note(curr);
                    linkedlist_delete();
                }
            });
        });
    });
    linkedlist_del(instrument->parent->instruments, instrument);
    nesynth_destroy_instrument(instrument);
}

void nesynth_instrument_upload_samples(NESynthInstrument* instrument, NESynthSample* samples, int num_samples) {
    free(instrument->samples);
    instrument->samples = malloc(sizeof(float) * num_samples);
    instrument->num_samples = num_samples;
    for (int i = 0; i < instrument->num_samples; i++) {
        instrument->samples[i] = samples[i] / 32767.f;
    }
}

float* nesynth_instrument_tune_samples(NESynthInstrument* instrument) {
    return &instrument->tune;
}

bool* nesynth_instrument_loop_samples(NESynthInstrument* instrument) {
    return &instrument->loop_samples;
}

NESynthNodeTable* nesynth_instrument_volume(NESynthInstrument* instrument) {
    return instrument->volume;
}

NESynthNodeTable* nesynth_instrument_pitch(NESynthInstrument* instrument) {
    return instrument->pitch;
}

int nesynth_num_songs(NESynth* synth) {
    return linkedlist_length(synth->songs);
}

NESynthSong* nesynth_add_song(NESynth* synth) {
    NESynthSong* song = calloc(sizeof(NESynthSong), 1);
    song->parent = synth;
    song->channels = linkedlist_create();
    song->bpm = 120;
    song->length = 64;
    song->loop_point = 0;
    linkedlist_add(synth->songs, song);
    return song;
}

NESynthSong* nesynth_get_song(NESynth* synth, int index) {
    return linkedlist_index(synth->songs, index);
}

void nesynth_delete_song(NESynthSong* song) {
    if (song->parent->curr_song == song) song->parent->curr_song = NULL;
    linkedlist_del(song->parent->songs, song);
    nesynth_destroy_song(song);
}

int nesynth_song_get_length(NESynthSong* song) {
    return song->length;
}

void nesynth_song_set_length(NESynthSong* song, int length) {
    if (length == song->length) return;
    linkedlist_foreach(song->channels, {
        NESynthChannel* channel = curr;
        channel->pattern_layout = realloc(channel->pattern_layout, sizeof(void*) * length);
        if (length > song->length) memset(channel->pattern_layout + song->length, 0, sizeof(void*) * (length - song->length));
        if (length < song->length) linkedlist_foreach(channel->patterns, {
            bool do_delete = true;
            for (int i = 0; i < length; i++) {
                if (channel->pattern_layout[i] == curr) {
                    do_delete = false;
                    break;
                }
            }
            if (do_delete) {
                nesynth_destroy_pattern(curr);
                linkedlist_delete();
            }
        });
    });
    song->length = length;
}

float* nesynth_song_loop_point(NESynthSong* song) {
    return &song->loop_point;
}

float* nesynth_song_bpm(NESynthSong* song) {
    return &song->bpm;
}

int nesynth_num_channels(NESynthSong* song) {
    return linkedlist_length(song->channels);
}

NESynthChannel* nesynth_add_channel(NESynthSong* song, NESynthChannelType type) {
    NESynthChannel* channel = calloc(sizeof(NESynthChannel), 1);
    channel->parent = song;
    channel->type = type;
    channel->patterns = linkedlist_create();
    channel->pattern_layout = calloc(sizeof(void*), song->length);
    channel->state.notes[NESynthNoteType_Volume].value = 1;
    linkedlist_add(song->channels, channel);
    return channel;
}

NESynthChannel* nesynth_get_channel(NESynthSong* song, int index) {
    return linkedlist_index(song->channels, index);
}

NESynthChannelType nesynth_get_channel_type(NESynthChannel* channel) {
    return channel->type;
}

void nesynth_delete_channel(NESynthChannel* channel) {
    linkedlist_del(channel->parent->channels, channel);
    nesynth_destroy_channel(channel);
}

void nesynth_assign_pattern(NESynthChannel* channel, int pattern_id, int position) {
    bool oob = position < 0 || position >= channel->parent->length;
    NESynthPattern* prev_pattern = oob ? NULL : channel->pattern_layout[position];
    channel->pattern_layout[position] = oob ? NULL : linkedlist_index(channel->patterns, pattern_id);
    if (!prev_pattern) return;
    for (int i = 0; i < channel->parent->length; i++) {
        if (channel->pattern_layout[i] == prev_pattern) return;
    }
    linkedlist_del(channel->patterns, prev_pattern);
    nesynth_destroy_pattern(prev_pattern);
}

void nesynth_remove_pattern_at(NESynthChannel* channel, int positon) {
    nesynth_assign_pattern(channel, -1, positon);
}

bool nesynth_any_pattern_at(NESynthChannel* channel, int position) {
    if (position < 0 || position >= channel->parent->length) return false;
    return channel->pattern_layout[position] != NULL;
}

NESynthPattern* nesynth_get_pattern_at(NESynthChannel* channel, int position) {
    if (position < 0 || position >= channel->parent->length) return NULL;
    if (!channel->pattern_layout[position]) {
        NESynthPattern* pattern = calloc(sizeof(NESynthPattern), 1);
        pattern->parent = channel;
        pattern->notes[0] = linkedlist_create();
        pattern->notes[1] = linkedlist_create();
        pattern->notes[2] = linkedlist_create();
        channel->pattern_layout[position] = pattern;
        linkedlist_add(channel->patterns, pattern);
    }
    return channel->pattern_layout[position];
}

int nesynth_num_unique_patterns(NESynthChannel* channel) {
    return linkedlist_length(channel->patterns);
}

int nesynth_get_pattern_id(NESynthPattern* pattern) {
    return linkedlist_indexof(pattern->parent->patterns, pattern);
}

int nesynth_num_notes(NESynthPattern* pattern, NESynthNoteType type) {
    return linkedlist_length(pattern->notes[type]);
}

NESynthNote* nesynth_get_note(NESynthPattern* pattern, NESynthNoteType type, int index) {
    return linkedlist_index(pattern->notes[type], index);
}

static int compare_notes(NESynthNote* a, NESynthNote* b) {
    return (a->start > b->start) - (a->start < b->start);
}

NESynthNote* nesynth_insert_note(NESynthPattern* pattern, NESynthNoteType type, NESynthInstrument* instrument, float base_note, float start, float length) {
    NESynthNote* note = calloc(sizeof(NESynthNote), 1);
    note->parent = pattern;
    note->start = start;
    note->length = length;
    note->instrument = instrument;
    note->base_note = base_note;
    note->slide_note = base_note;
    note->attack = true;
    note->type = type;
    linkedlist_add_sorted(pattern->notes[type], note, (void*)compare_notes);
    return note;
}

void nesynth_delete_note(NESynthNote* note) {
    linkedlist_del(note->parent->notes[note->type], note);
    nesynth_destroy_note(note);
}

float* nesynth_base_note(NESynthNote* note) {
    return &note->base_note;
}

float* nesynth_slide_note(NESynthNote* note) {
    return &note->slide_note;
}

bool* nesynth_attack_note(NESynthNote* note) {
    return &note->attack;
}

float nesynth_get_note_start(NESynthNote* note) {
    return note->start;
}

void nesynth_set_note_start(NESynthNote* note, float start) {
    note->start = start;
    linkedlist_del(note->parent->notes[note->type], note);
    linkedlist_add_sorted(note->parent->notes[note->type], note, (void*)compare_notes);
}

float* nesynth_note_length(NESynthNote* note) {
    return &note->length;
}

NESynthInstrument** nesynth_note_instrument(NESynthNote* note) {
    return &note->instrument;
}

static int nesynth_nodetable_compare(NESynthNode* a, NESynthNode* b) {
    return (a->pos > b->pos) - (a->pos < b->pos);
}

int nesynth_nodetable_num_nodes(NESynthNodeTable* node_table) {
    return linkedlist_length(node_table->nodes);
}

int nesynth_nodetable_insert(NESynthNodeTable* node_table, float position, float value) {
    NESynthNode* node = calloc(sizeof(NESynthNode), 1);
    node->base = node->slide = value;
    node->pos = position;
    return linkedlist_add_sorted(node_table->nodes, node, (void*)nesynth_nodetable_compare);
}

void nesynth_nodetable_remove(NESynthNodeTable* node_table, int index) {
    if (index <= 0) return;
    linkedlist_foreach(node_table->nodes, {
        if (index-- == 0) linkedlist_delete();
    });
}

float* nesynth_nodetable_loop_point(NESynthNodeTable* node_table) {
    return &node_table->loop_point;
}

float* nesynth_nodetable_value(NESynthNodeTable* node_table, int index) {
    NESynthNode* node = linkedlist_index(node_table->nodes, index);
    if (node) return &node->base;
    return NULL;
}

float* nesynth_nodetable_slide(NESynthNodeTable* node_table, int index) {
    NESynthNode* node = linkedlist_index(node_table->nodes, index);
    if (node) return &node->slide;
    return NULL;
}

NESynthTimescale* nesynth_nodetable_timescale(NESynthNodeTable* node_table) {
    return &node_table->timescale;
}

static void nesynth_destroy_instrument(NESynthInstrument* instrument) {
    nesynth_destroy_nodetable(instrument->volume);
    nesynth_destroy_nodetable(instrument->pitch);
    free(instrument->samples);
    free(instrument);
}

static void nesynth_destroy_song(NESynthSong* song) {
    linkedlist_foreach(song->channels, nesynth_destroy_channel(curr));
    linkedlist_destroy(song->channels);
    free(song);
}

static void nesynth_destroy_channel(NESynthChannel* channel) {
    linkedlist_foreach(channel->patterns, nesynth_destroy_pattern(curr));
    linkedlist_destroy(channel->patterns);
    free(channel->pattern_layout);
    free(channel);
}

static void nesynth_destroy_pattern(NESynthPattern* pattern) {
    for (int i = 0; i < 3; i++) {
        linkedlist_foreach(pattern->notes[i], nesynth_destroy_note(curr));
        linkedlist_destroy(pattern->notes[i]);
    }
    free(pattern);
}

static void nesynth_destroy_note(NESynthNote* note) {
    free(note);
}

static void nesynth_destroy_nodetable(NESynthNodeTable* node_table) {
    linkedlist_foreach(node_table->nodes, free(curr));
    linkedlist_destroy(node_table->nodes);
    free(node_table);
}

static NESynthIter* nesynth_iter(NESynthLinkedList* list) {
    NESynthIter* iter = malloc(sizeof(NESynthIter));
    iter->index = -1;
    iter->element = list->prev ? list->prev : NULL;
    iter->first = list;
    return iter;
}

NESynthIter* nesynth_iter_instruments(NESynth* synth) { return nesynth_iter(synth->instruments); }
NESynthIter* nesynth_iter_songs(NESynth* synth) { return nesynth_iter(synth->instruments); }
NESynthIter* nesynth_iter_channels(NESynthSong* song) { return nesynth_iter(song->channels); }
NESynthIter* nesynth_iter_patterns(NESynthChannel* channel) { return nesynth_iter(channel->patterns); }
NESynthIter* nesynth_iter_notes(NESynthPattern* pattern, NESynthNoteType type) { return nesynth_iter(pattern->notes[type]); }
NESynthIter* nesynth_iter_nodes(NESynthNodeTable* node_table) { return nesynth_iter(node_table->nodes); }

bool nesynth_iter_next(NESynthIter* iter) {
    if (!iter->element || (iter->element->next == iter->first && iter->index != -1)) {
        free(iter);
        return false;
    }
    iter->index++;
    iter->element = iter->element->next;
    return true;
}

void* nesynth_iter_get(NESynthIter* iter) {
    return iter->index == -1 ? NULL : iter->element ? iter->element->item : NULL;
}

// == RENDERER ==

static struct {
    NESynthSong* song;
    NESynthInstrument* instrument;
    NESynthNodeTable* nodetable;
    NESynthChannel* channel;
    NESynthNote* note;
    int curr_cursor, curr_env_cursor;
    int curr_instrument;
    union {
        struct {
            float instrument;
            float volume;
            float pitch;
            float env_volume;
            float env_pitch;
            float max;
        };
        float arr[5];
    } cursors;
} state;

#define CURSOR state.cursors.arr[state.curr_cursor]
#define CURSOR_ENV state.cursors.arr[state.curr_env_cursor]

int nesynthcmd_instrument() {
    NESynth* synth = state.song->parent;
    int index = nesynth_num_instruments(synth);
    state.instrument = nesynth_add_instrument(synth);
    state.cursors.env_volume = 0;
    state.cursors.env_pitch = 0;
    state.nodetable = NULL;
    return index;
}

void nesynthcmd_channel(NESynthChannelType type) {
    state.channel = nesynth_add_channel(state.song, type);
    state.cursors.instrument = 0;
    state.cursors.volume = 0;
    state.cursors.pitch = 0;
    state.curr_cursor = 0;
    state.note = NULL;
}

void nesynthcmd_bpm(float bpm) {
    state.song->bpm = bpm;
}

void nesynthcmd_loop_song_to(float pos) {
    state.song->loop_point = pos;
}

void nesynthcmd_volume() {
    if (!state.instrument) return;
    state.nodetable = state.instrument->volume;
    state.curr_env_cursor = 3;
}

void nesynthcmd_pitch() {
    if (!state.instrument) return;
    state.nodetable = state.instrument->pitch;
    state.curr_env_cursor = 4;
}

void nesynthcmd_push_envelope(float length, float base) {
    if (!state.nodetable) return;
    if (CURSOR_ENV == 0) *nesynth_nodetable_value(state.nodetable, 0) = base;
    else nesynth_nodetable_insert(state.nodetable, CURSOR_ENV, base);
    CURSOR_ENV += length;
}

void nesynthcmd_slide_envelope(float dest) {
    if (!state.nodetable) return;
    ((NESynthNode*)state.nodetable->nodes->prev->item)->slide = dest;
}

void nesynthcmd_push_and_slide_envelope(float length, float base) {
    nesynthcmd_slide_envelope(base);
    nesynthcmd_push_envelope(length, base);
}

void nesynthcmd_loop_nodetable_to(float pos) {
    if (!state.nodetable) return;
    state.nodetable->loop_point = pos;
}

void nesynthcmd_upload_samples(NESynthSample* samples, int num_samples) {
    if (!state.instrument) return;
    nesynth_instrument_upload_samples(state.instrument, samples, num_samples);
}

void nesynthcmd_tune(float note) {
    if (!state.instrument) return;
    state.instrument->tune = note;
}

void nesynthcmd_loop_samples() {
    if (!state.instrument) return;
    state.instrument->loop_samples = true;
}

void nesynthcmd_switch_instrument(int instrument) {
    if (instrument < 0 || instrument >= nesynth_num_instruments(state.song->parent)) return;
    state.curr_instrument = instrument;
}

void nesynthcmd_switch_note_type(NESynthNoteType note_type) {
    if (note_type < 0 || note_type >= 3) return;
    state.curr_cursor = note_type;
}

void nesynthcmd_sync_cursors() {
    float max = 0;
    for (int i = 0; i < 3; i++) {
        if (max < state.cursors.arr[i]) max = state.cursors.arr[i];
    }
    for (int i = 0; i < 3; i++) {
        state.cursors.arr[i] = max;
    }
}

void nesynthcmd_push_silence(float length) {
    if (!state.channel) return;
    CURSOR += length;
    if (state.cursors.max < CURSOR) state.cursors.max = CURSOR;
    if (floorf(state.cursors.max / 4) >= state.song->length) nesynth_song_set_length(state.song, state.song->length + 64);
}

void nesynthcmd_push_note(float length, float base) {
    if (!state.channel || state.curr_cursor < 0 || state.curr_cursor >= 3) return;
    int index = floor(CURSOR / 4);
    NESynthPattern* pattern = nesynth_get_pattern_at(state.channel, index);
    state.note = nesynth_insert_note(pattern, state.curr_cursor, nesynth_get_instrument(state.song->parent, state.curr_instrument), base, CURSOR - index * 4, length);
    nesynthcmd_push_silence(length);
}

void nesynthcmd_extend_note(float length, float base) {
    if (!state.note || state.curr_cursor != 0) return;
    nesynthcmd_push_note(length, base);
    state.note->attack = false;
}

void nesynthcmd_slide_and_push_note(float length, float base) {
    nesynthcmd_slide_note(base);
    nesynthcmd_push_note(length, base);
}

void nesynthcmd_slide_and_extend_note(float length, float base) {
    nesynthcmd_slide_note(base);
    nesynthcmd_extend_note(length, base);
}

void nesynthcmd_slide_note(float dest) {
    if (!state.note) return;
    state.note->slide_note = dest;
}

#undef CURSOR
#undef CURSOR_ENV

NESynthSample* nesynthcmd_render(NESynthCmdFunc func, int sample_rate, int* num_samples, int* loop_point, float volume) {
    memset(&state, 0, sizeof(state));
    NESynth* synth = nesynth_create(sample_rate);
    state.song = nesynth_add_song(synth);
    func();
    nesynth_song_set_length(state.song, ceil(state.cursors.max / 4));
    if (num_samples) *num_samples = state.song->length * 4 / (state.song->bpm / 60) * sample_rate;
    if (loop_point)  *loop_point  = state.song->loop_point / (state.song->bpm / 60) * sample_rate;
    NESynthSample* samples = malloc(sizeof(NESynthSample) * *num_samples);
    nesynth_select_song(synth, 0);
    nesynth_get_samples(synth, samples, *num_samples, volume);
    nesynth_destroy(synth);
    return samples;
}

// == SYNTHESIZER ==

static inline float nesynth_apply_noise_correction(float midi) {
    float midi_octave = floorf(midi / NESYNTH_EDO);
    float octave = midi_octave - 1;
    float tone = midi - midi_octave * NESYNTH_EDO;
    octave = (octave + 2) * 1.25;
    return tone + (octave + 1) * NESYNTH_EDO;
}

static inline NESynthNote* nesynth_get_note_value(NESynthChannel* channel, NESynthPattern* pattern, int pattern_index, NESynthNoteType type, float pos, float* out, bool noise_correct) {
    *out = channel->state.notes[type].value;
    if (linkedlist_empty(pattern->notes[type])) return NULL;
    if (pos < channel->state.notes[type].prev_pos) channel->state.notes[type].cursor = NULL;
    bool cursor_valid = channel->state.notes[type].cursor;
    NESynthLinkedList* curr = cursor_valid ? channel->state.notes[type].cursor : pattern->notes[type];
    while (true) {
        if (cursor_valid) {
            NESynthNote* next = curr->next->item;
            if (pos < next->start || curr->next == pattern->notes[type]) break;
            curr = curr->next;
        }
        channel->state.notes[type].curr_pattern = pattern_index;
        channel->state.notes[type].curr_note = curr->item;
        channel->state.notes[type].cursor = curr;
        cursor_valid = true;
    }
    NESynthNote* note = channel->state.notes[type].curr_note;
    if (note) {
        float start = note->start + channel->state.notes[type].curr_pattern * 4;
        float frame = pos + pattern_index * 4;
        if (frame >= start && frame < start + note->length) {
            float base  = noise_correct ? nesynth_apply_noise_correction(note->base_note)  : note->base_note;
            float slide = noise_correct ? nesynth_apply_noise_correction(note->slide_note) : note->slide_note;
            *out = (slide - base) * ((frame - start) / note->length) + base;
        }
        else note = NULL;
    }
    channel->state.notes[type].value = *out;
    channel->state.notes[type].prev_pos = pos;
    return note;
}

static inline float nesynth_compute_nodetable(NESynthNodeTable* node_table, float pos) {
    NESynthNodeTable* curr = node_table;
    float loop_pos = node_table->loop_point;
    float from = 0, to = 0, x = 0;
    int counter = 0;
    linkedlist_foreach(node_table->nodes, {
        NESynthNode* node = curr;
        NESynthNode* next = __next->item;
        if (linkedlist_last()) {
            if (nesynth_is_looping(loop_pos)) return node->base;
            else return nesynth_compute_nodetable(node_table, fmodf(pos, node->pos - loop_pos) + loop_pos);
        }
        else {
            if (pos >= node->pos && pos < next->pos) return (node->slide - node->base) * ((pos - node->pos) / (next->pos - node->pos)) + node->base;
            counter++;
        }
    });
    assert(false && "what the fuck");
}

static inline float nesynth_get_sample_from_instrument(NESynthInstrument* instrument, int sample) {
    if (!instrument->samples || instrument->num_samples == 0) return 0;
    if (sample >= instrument->num_samples) {
        if (instrument->loop_samples) sample %= instrument->num_samples;
        else sample = instrument->num_samples - 1;
    }
    return instrument->samples[sample];
}

void nesynth_get_samples(NESynth* synth, NESynthSample* samples, int num_samples, float volume) {
    if (!synth->curr_song) return;
    for (int i = 0; i < num_samples; i++) {
        float sample = 0;
        if (synth->position >= synth->curr_song->length * 4 && nesynth_is_looping(synth->curr_song->loop_point))
            synth->position = fmodf(
                synth->position              - synth->curr_song->loop_point,
                synth->curr_song->length * 4 - synth->curr_song->loop_point
            ) + synth->curr_song->loop_point;
        int pattern_index = (int)synth->position / 4;
        float pos = synth->position - pattern_index * 4;
        if (pattern_index < synth->curr_song->length) linkedlist_foreach(synth->curr_song->channels, {
            NESynthChannel* channel = curr;
            NESynthPattern* pattern = channel->pattern_layout[pattern_index];
            if (!pattern) linkedlist_continue();
            float pitch = 0, mod_volume = 1, mod_pitch = 0;
            NESynthNote* note = nesynth_get_note_value(channel, pattern, pattern_index, NESynthNoteType_Instrument, pos, &pitch, channel->type == NESynthChannelType_Noise);
            if (!note) linkedlist_continue();
            if (channel->state.prev_note != note && note->attack) {
                channel->state.prev_note = note;
                channel->state.attack = note->start + pattern_index * 4;
                if (channel->type == NESynthChannelType_Waveform) channel->state.phase = 0;
            }
            nesynth_get_note_value(channel, pattern, pattern_index, NESynthNoteType_Volume, pos, &mod_volume, false);
            nesynth_get_note_value(channel, pattern, pattern_index, NESynthNoteType_Pitch,  pos, &mod_pitch,  false);
            float seconds = fmaxf((synth->position - channel->state.attack) / synth->curr_song->bpm * 60, 0);
            float instrument_volume = nesynth_compute_nodetable(note->instrument->volume, seconds);
            float instrument_pitch  = nesynth_compute_nodetable(note->instrument->pitch,  seconds);
            mod_volume *= instrument_volume;
            mod_pitch  += instrument_pitch;
            float freq = nesynth_midi2freq(pitch + mod_pitch, NULL);
            float phase = channel->state.phase + freq / synth->sample_rate;
            float value = 0;
            int integral = (int)phase;
            phase -= integral;
            switch (channel->type) {
                case NESynthChannelType_Square: {
                    value = phase < 0.5 ? 1 : -1;
                    value *= .5f; // square is pretty loud
                    channel->state.phase = phase;
                } break;
                case NESynthChannelType_Triangle: {
                    value = -4 * fabsf(phase - .5f) + 1;
                    channel->state.phase = phase;
                } break;
                case NESynthChannelType_Noise: {
                    int noise_mask = (1 << NESYNTH_NOISE_RESOLUTION) - 1;
                    if (integral >= 1) channel->state.synth_state = rand() & noise_mask;
                    value = (2 * (channel->state.synth_state / (float)noise_mask) - 1);
                    value *= .5f; // noise too
                    channel->state.phase = phase;
                } break;
                case NESynthChannelType_Waveform: {
                    float multiplier = freq / nesynth_midi2freq(note->instrument->tune, NULL);
                    float from = nesynth_get_sample_from_instrument(note->instrument, floorf(channel->state.phase));
                    float to   = nesynth_get_sample_from_instrument(note->instrument,  ceilf(channel->state.phase));
                    value = (to - from) * (sample - floorf(sample)) + from;
                    channel->state.phase += multiplier;
                } break;
            }
            value *= mod_volume;
            sample += value;
        });
        sample *= volume;
        if (sample < -1) sample = -1;
        if (sample >  1) sample =  1;
        samples[i] = sample * 32767;
        synth->position += 1 / (60 / synth->curr_song->bpm * synth->sample_rate);
    }
}
