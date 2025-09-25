#ifndef NESYNTH
#define NESYNTH

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

typedef int16_t NESynthSample;
typedef struct NESynth NESynth;
typedef struct NESynthInstrument NESynthInstrument;
typedef struct NESynthSong NESynthSong;
typedef struct NESynthChannel NESynthChannel;
typedef struct NESynthPattern NESynthPattern;
typedef struct NESynthNote NESynthNote;
typedef struct NESynthNodeTable NESynthNodeTable;
typedef struct NESynthIter NESynthIter;
typedef void(*NESynthCmdFunc)();

typedef enum {
    NESynthChannelType_Square,
    NESynthChannelType_Triangle,
    NESynthChannelType_Noise,
    NESynthChannelType_Waveform,
} NESynthChannelType;

typedef enum {
    NESynthNoteType_Melodic,
    NESynthNoteType_Volume,
    NESynthNoteType_Pitch,
} NESynthNoteType;

typedef enum {
    NESynthTimescale_Seconds,
    NESynthTimescale_Beats,
} NESynthTimescale;

#define NESYNTH_NO_LOOP NAN
#define nesynth_is_looping isnan

#define NESYNTH_EDO                12
#define NESYNTH_CONCERT_PITCH_FREQ 440
#define NESYNTH_CONCERT_PITCH      NESYNTH_NOTE(A, 4)
#define NESYNTH_NOISE_RESOLUTION   4

#define NESYNTH_NOTE_C  0
#define NESYNTH_NOTE_CS 1
#define NESYNTH_NOTE_D  2
#define NESYNTH_NOTE_DS 3
#define NESYNTH_NOTE_E  4
#define NESYNTH_NOTE_F  5
#define NESYNTH_NOTE_FS 6
#define NESYNTH_NOTE_G  7
#define NESYNTH_NOTE_GS 8
#define NESYNTH_NOTE_A  9
#define NESYNTH_NOTE_AS 10
#define NESYNTH_NOTE_B  11

#define NESYNTH_FREQ(freq) nesynth_freq2midi(freq, NULL)
#define NESYNTH_MIDI(note, octave) ((note) + (octave) * (NESYNTH_EDO) + (NESYNTH_EDO)) // midi 0 is C-1, so we add EDO
#define NESYNTH_NOTE(note, octave) NESYNTH_MIDI(NESYNTH_NOTE_##note, octave)

typedef float(*NESynthFreqMod)(float freq);

float nesynth_midi2freq(float midi, NESynthFreqMod freq_mod);
float nesynth_freq2midi(float freq, NESynthFreqMod freq_mod);

NESynth* nesynth_create(int sample_rate);
void nesynth_destroy(NESynth* synth);
void nesynth_select_song(NESynth* synth, int song);
void nesynth_seek(NESynth* synth, float sec);
float nesynth_tell(NESynth* synth);
float nesynth_get_length(NESynth* synth);
void nesynth_get_samples(NESynth* synth, NESynthSample* samples, int num_samples, float volume);

int nesynth_num_instruments(NESynth* synth);
NESynthInstrument* nesynth_add_instrument(NESynth* synth);
NESynthInstrument* nesynth_get_instrument(NESynth* synth, int index);
void nesynth_delete_instrument(NESynthInstrument* instrument);
void nesynth_instrument_upload_samples(NESynthInstrument* instrument, NESynthSample* samples, int num_samples);
float* nesynth_instrument_tune_samples(NESynthInstrument* instrument);
bool* nesynth_instrument_loop_samples(NESynthInstrument* instrument);
NESynthNodeTable* nesynth_instrument_volume(NESynthInstrument* instrument);
NESynthNodeTable* nesynth_instrument_pitch(NESynthInstrument* instrument);

int nesynth_num_songs(NESynth* synth);
NESynthSong* nesynth_add_song(NESynth* synth);
NESynthSong* nesynth_get_song(NESynth* synth, int index);
void nesynth_delete_song(NESynthSong* song);
void nesynth_song_set_length(NESynthSong* song, int length);
int nesynth_song_get_length(NESynthSong* song);
float* nesynth_song_loop_point(NESynthSong* song);
float* nesynth_song_bpm(NESynthSong* song);

int nesynth_num_channels(NESynthSong* song);
NESynthChannel* nesynth_add_channel(NESynthSong* song, NESynthChannelType type);
NESynthChannel* nesynth_get_channel(NESynthSong* song, int index);
NESynthChannelType nesynth_get_channel_type(NESynthChannel* channel);
void nesynth_delete_channel(NESynthChannel* channel);
void nesynth_assign_pattern(NESynthChannel* channel, int pattern_id, int position);
void nesynth_remove_pattern_at(NESynthChannel* channel, int position);
bool nesynth_any_pattern_at(NESynthChannel* channel, int position);
int nesynth_num_unique_patterns(NESynthChannel* channel);

NESynthPattern* nesynth_get_pattern_at(NESynthChannel* channel, int position);
int nesynth_get_pattern_id(NESynthPattern* pattern);

int nesynth_num_notes(NESynthPattern* pattern, NESynthNoteType type);
NESynthNote* nesynth_get_note(NESynthPattern* pattern, NESynthNoteType type, int index);
NESynthNote* nesynth_insert_note(NESynthPattern* pattern, NESynthNoteType type, NESynthInstrument* instrument, float base_note, float start, float length);
void nesynth_delete_note(NESynthNote* note);
float* nesynth_base_note(NESynthNote* note);
float* nesynth_slide_note(NESynthNote* note);
bool* nesynth_attack_note(NESynthNote* note);
float* nesynth_note_length(NESynthNote* note);
float nesynth_get_note_start(NESynthNote* note);
void nesynth_set_note_start(NESynthNote* note, float start);
NESynthInstrument** nesynth_note_instrument(NESynthNote* note);

int nesynth_nodetable_num_nodes(NESynthNodeTable* node_table);
int nesynth_nodetable_insert(NESynthNodeTable* node_table, float position, float value);
void nesynth_nodetable_remove(NESynthNodeTable* node_table, int index);
float* nesynth_nodetable_loop_point(NESynthNodeTable* node_table);
float* nesynth_nodetable_value(NESynthNodeTable* node_table, int index);
float* nesynth_nodetable_slide(NESynthNodeTable* node_table, int index);
float nesynth_nodetable_pos(NESynthNodeTable* node_table, int index);
NESynthTimescale* nesynth_nodetable_timescale(NESynthNodeTable* node_table);

NESynthIter* nesynth_iter_instruments(NESynth* synth);
NESynthIter* nesynth_iter_songs(NESynth* synth);
NESynthIter* nesynth_iter_channels(NESynthSong* song);
NESynthIter* nesynth_iter_patterns(NESynthChannel* channel);
NESynthIter* nesynth_iter_notes(NESynthPattern* pattern, NESynthNoteType type);
bool nesynth_iter_next(NESynthIter* iter);
void* nesynth_iter_get(NESynthIter* iter);

NESynthSample* nesynthcmd_render(NESynthCmdFunc func, int sample_rate, int* num_samples, int* loop_point, float volume);

int  nesynthcmd_instrument();
void nesynthcmd_channel(NESynthChannelType type);
void nesynthcmd_bpm(float bpm);
void nesynthcmd_loop_song_to(float pos);
void nesynthcmd_volume();
void nesynthcmd_pitch();
void nesynthcmd_push_envelope(float length, float base);
void nesynthcmd_push_and_slide_envelope(float length, float base);
void nesynthcmd_slide_envelope(float dest);
void nesynthcmd_loop_nodetable_to(float pos);
void nesynthcmd_upload_samples(NESynthSample* samples, int num_samples);
void nesynthcmd_tune(float note);
void nesynthcmd_loop_samples();
void nesynthcmd_switch_instrument(int instrument);
void nesynthcmd_switch_note_type(NESynthNoteType note_type);
void nesynthcmd_sync_cursors();
void nesynthcmd_push_silence(float length);
void nesynthcmd_push_note(float length, float base);
void nesynthcmd_extend_note(float length, float base);
void nesynthcmd_slide_and_push_note(float length, float base);
void nesynthcmd_slide_and_extend_note(float length, float base);
void nesynthcmd_slide_note(float dest);

#endif
