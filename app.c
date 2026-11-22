/* SPDX-License-Identifier: MPL-2.0 */
#include <stddef.h>
#include "fastmath.h"
#include "bass_samples.h"
#include "guitar_samples.h"
#include "lm1_samples.h"
#include "piano_samples.h"
#include "startup_splash.h"
#include "jc_face.h"
#include "knob_images.h"
#include "dsp/escape_sequence.h"
#include "dsp/halloween_sequence.h"
#include "dsp/fog_sequence.h"
#include "dsp/builtin_steal_sequences.h"
#include "dsp/escape_rebuild_sequence.h"

#define SETTINGS_MAGIC 0x504C4953u
#define SEQUENCE_MAGIC 0x53455131u
#define SEQUENCE_VERSION 4u
#define SEQUENCE_DRUM_TRACK_VERSION 3u
#define SEQUENCE_TEMPO_VERSION 2u
#define SEQUENCE_LEGACY_VERSION 1u
#define SEQUENCE_MELODIC_TRACKS 4u
#define SEQUENCE_DRUM_TRACK 4u
#define SEQUENCE_TRACKS 5u
#define SEQUENCE_VOICES 5u
#define BUILTIN_STEAL_VOICES 5u
#define ESCAPE_EXPERIMENT_VOICES 5u
#define SEQUENCE_DRUM_NATURAL 7u
#define SEQUENCE_DRUM_LEVEL 0.42f
#define SEQUENCE_MAX_TEMPO 140u
#define SEQUENCE_PLAY_RESET_MS 500u
#define SEQUENCE_USER_SLOTS 16u
#define SEQUENCE_BUILTIN_COUNT 3u
#define SEQUENCE_SLOTS (SEQUENCE_USER_SLOTS + SEQUENCE_BUILTIN_COUNT)
#define SEQUENCE_NAME_LENGTH 12u
#define SEQUENCE_MAX_STEPS 4096u
#define TRACKS 7
#define INSTRUMENTS (TRACKS + 1u)
#define SEQUENCE_NAME_ALPHABET "abcdefghijklmnopqrstuvwxyz0123456789 "
#define SEQUENCE_SOUND_COUNT 41
#define BASS_ELECTRIC_VARIANT 7u
#define BASS_ACOUSTIC_VARIANT 8u
#define BASS_TIMBRE_COUNT 5u
#define BASS_MODE_COUNT 10u
#define SAMPLE_RATE 44100.0f
#define PIANO_TABLE 1024u
#define PIANO_PARTIALS 4u
#define TANH_TABLE 256u

typedef struct {
    float phase[PIANO_PARTIALS + 2u];
    float inc[PIANO_PARTIALS + 2u];
    float amp[PIANO_PARTIALS + 2u];
    float decay[PIANO_PARTIALS + 2u];
    float punch;
    float thump;
    float level;
    const uint8_t *sample_data;
    uint32_t sample_length;
    uint32_t sample_code_position;
    uint32_t sample_cache_index;
    float sample_position;
    float sample_rate;
    float sample_filter;
    float sample_brightness;
    float sample_envelope;
    float sample_decay;
    int32_t sample_predictor;
    int16_t sample_a;
    int16_t sample_b;
    uint32_t age;
    uint32_t duration;
    int8_t sample_step_index;
    uint8_t active;
    uint8_t key;
    uint8_t sampled;
} piano_t;

static uint32_t trigger_piano(int32_t note, uint32_t duration, uint32_t velocity);
static void stop_audio_voices(void);

typedef struct {
    float phase;
    float increment;
    float envelope;
    int32_t note;
    uint32_t age;
    uint32_t duration;
    float phase2;
    float phase3;
    float envelope2;
    float lp1;
    float lp2;
    float lp3;
    float lp4;
    float ladder_g;
    float ladder_den;
    float velocity_gain;
    uint8_t active;
    uint8_t key;
} voice_t;

typedef struct {
    float phase[2];
    float increment[2];
    float filter_ic1[2];
    float filter_ic2[2];
    float filter_envelope;
    float velocity_gain;
    uint32_t age;
    uint32_t duration;
    uint8_t patch;
    uint8_t active;
    uint8_t key;
} cs80_voice_t;

typedef struct {
    const char *name;
    int8_t octave[2];
    int8_t detune_cents;
    uint8_t shape[2];
    uint8_t pulse_width[2];
    uint8_t cutoff;
    uint8_t resonance;
    uint8_t filter_env;
    uint8_t filter_decay;
    uint8_t attack_ms;
    uint16_t release_ms;
    uint8_t sustain;
    uint8_t controls[4];
    float output_trim;
} cs80_patch_t;

typedef struct {
    const int8_t *data;
    const uint8_t *adpcm_data;
    uint32_t length;
    uint32_t age;
    uint32_t max_samples;
    uint32_t adpcm_code_position;
    uint32_t adpcm_cache_index;
    int32_t adpcm_predictor;
    int16_t adpcm_a;
    int16_t adpcm_b;
    float position;
    float rate;
    float gain;
    int8_t adpcm_step_index;
    uint8_t loops;   /* extra passes over the data, each quieter than the last */
} sample_voice_t;

typedef struct {
    uint32_t tick;
    uint16_t duration;
    uint8_t part;
    uint8_t note;
    uint8_t velocity;
} song_event_t;

typedef struct {
    uint32_t magic;
    uint16_t count;
    uint8_t version;
    uint8_t meter;
    uint8_t bars;
    uint8_t instrument[SEQUENCE_MELODIC_TRACKS];
    uint8_t variant[SEQUENCE_MELODIC_TRACKS];
    char name[SEQUENCE_NAME_LENGTH];
    uint8_t tempo;
    uint8_t drum_variant;
} sequence_header_t;

/* Versions 2 and 3 use bytes of the original header's trailing padding. */
typedef char sequence_header_layout_check[
    sizeof(sequence_header_t) == 32u && offsetof(sequence_header_t, tempo) == 29u
        && offsetof(sequence_header_t, drum_variant) == 30u ? 1 : -1];

#define SEQUENCE_EVENT_MAX ((PLAT_STORE_MAX - sizeof(sequence_header_t)) / sizeof(uint32_t))
typedef char escape_sequence_fits[
    ESCAPE_SEQUENCE_EVENT_COUNT <= SEQUENCE_EVENT_MAX ? 1 : -1];
typedef char halloween_sequence_fits[
    HALLOWEEN_SEQUENCE_EVENT_COUNT <= SEQUENCE_EVENT_MAX ? 1 : -1];
typedef char fog_sequence_fits[
    FOG_SEQUENCE_EVENT_COUNT <= SEQUENCE_EVENT_MAX ? 1 : -1];
typedef char builtin_steal_sequences_fit[
    HALLOWEEN_STEAL_EVENT_COUNT <= SEQUENCE_EVENT_MAX
        && FOG_STEAL_EVENT_COUNT <= SEQUENCE_EVENT_MAX ? 1 : -1];
typedef struct {
    sequence_header_t header;
    uint32_t events[SEQUENCE_EVENT_MAX];
} sequence_t;

typedef struct {
    uint8_t used;
    char name[SEQUENCE_NAME_LENGTH];
} sequence_slot_t;

enum { SCREEN_MAIN, SCREEN_SEQUENCE, SCREEN_HELP, SCREEN_NAME, SCREEN_DELETE, SCREEN_PLAYBACK };
enum { METER_4_4, METER_8_4, METER_8_8, METER_16_16, METER_5_4, METER_COUNT };

typedef struct {
    const char *display_name;
    const char *stored_name;
    uint8_t meter;
    uint8_t bars;
    uint8_t tempo;
    uint8_t instrument[SEQUENCE_MELODIC_TRACKS];
    uint32_t count;
    const uint32_t *events;
    uint32_t steal_count;
    const uint32_t *steal_events;
} sequence_builtin_t;

static const sequence_builtin_t sequence_builtins[SEQUENCE_BUILTIN_COUNT] = {
    {
        "Escape From Firmware", "escape from", METER_4_4,
        ESCAPE_SEQUENCE_BARS, ESCAPE_SEQUENCE_BPM, {0u, 1u, 2u, 6u},
        ESCAPE_SEQUENCE_EVENT_COUNT, escape_sequence_events,
        ESCAPE_REBUILD_EVENT_COUNT, escape_rebuild_events
    },
    {
        "Halloween", "halloween", HALLOWEEN_SEQUENCE_METER,
        HALLOWEEN_SEQUENCE_BARS, HALLOWEEN_SEQUENCE_BPM, {0u, 0u, 4u, 0u},
        HALLOWEEN_SEQUENCE_EVENT_COUNT, halloween_sequence_events,
        HALLOWEEN_STEAL_EVENT_COUNT, halloween_steal_events
    },
    {
        "The FMog", "the fmog", METER_4_4,
        FOG_SEQUENCE_BARS, FOG_SEQUENCE_BPM, {1u, 0u, 4u, 6u},
        FOG_SEQUENCE_EVENT_COUNT, fog_sequence_events,
        FOG_STEAL_EVENT_COUNT, fog_steal_events
    },
};

/* Song mode and the PLAY backing loop are switched off until their mix is fixed (distortion). */
#define BACKING_ENABLED 0
#if BACKING_ENABLED && defined(__has_include)
#if __has_include("dsp/song.h")
#include "dsp/song.h"
#define HAVE_SONG 1
#endif
#endif
#ifdef HAVE_SONG
#define DEFAULT_TEMPO SONG_BPM
#else
#define DEFAULT_TEMPO 80u
#endif

#define JUNO_VOICES 8u
#define PIANO_VOICES 6u
#define PRO_ONE_VOICES 6u
#define GUITAR_VOICES 3u
#define CS50_VOICES 4u
#define CS80_VOICES 4u
#define CS80_PATCHES 10u
#define MONO_VOICES 2u

typedef struct {
    uint32_t magic;
    uint8_t tempo;
    uint8_t reserved[3];
    uint16_t seq_mask[3];   /* drum sequencer: kick, snare, hat as 16-step bit masks */
    uint8_t seq_perc[16];   /* fourth track: drum sample + 1 per step, 0 = none */
} settings_t;

static settings_t settings;
static volatile int32_t requested_tempo = DEFAULT_TEMPO;
static volatile uint8_t requested_play;
static volatile uint8_t requested_escape_play;
static volatile uint8_t all_notes_off_requested;
static volatile uint8_t restart_requested;
static volatile uint8_t requested_seq;
static volatile uint8_t sequence_playing;
static volatile uint8_t sequence_stop_requested;
static volatile uint8_t sequence_playback_tempo;
static uint8_t sequence_screen;
static uint8_t sequence_tempo_initialized;
static uint8_t sequence_selected_slot;
static uint8_t sequence_editing_saved;
static uint8_t sequence_editing_slot;
static uint8_t sequence_builtin_seed_failed;
static uint8_t sequence_cursor_track;
static uint16_t sequence_cursor_step;
static uint8_t sequence_follow_playback;
static uint32_t sequence_last_play_press;
static uint8_t sequence_last_play_press_valid;
static uint8_t sequence_duration_index;
static uint8_t sequence_selected_sound;
static uint8_t sequence_warning;
static uint32_t sequence_warning_until;
static volatile uint32_t sequence_audio_step;
static uint32_t sequence_audio_start;
static volatile uint32_t sequence_audio_length;
static float sequence_audio_samples;
static uint8_t sequence_audio_started;
static uint8_t sequence_audio_from_saved;
static uint8_t sequence_audio_stealing;
static uint8_t sequence_audio_steal_limit;
static const uint32_t *sequence_audio_events;
static uint32_t sequence_audio_event_count, sequence_audio_event_index;
static uint32_t sequence_escape_cpu_peak, sequence_escape_xruns_start;
#ifdef PLAT_HOST
static uint32_t sequence_steals, sequence_steal_drops, sequence_steal_peak;
static float sequence_mix_peak;
static uint32_t sequence_mix_over_one;
static uint32_t sequence_mix_peak_step;
#endif
static uint8_t sequence_name_cursor;
static uint8_t sequence_name_value;
static uint8_t sequence_name_length;
static uint8_t sequence_save_slot;
static uint8_t sequence_slot_count;
static uint8_t sequence_last_note;
static uint8_t sequence_last_note_valid;
static uint8_t sequence_help_page;
static uint32_t sequence_help_master;
static uint8_t sequence_help_return_screen;
enum { SEQUENCE_NOTICE_NONE, SEQUENCE_NOTICE_NOTE, SEQUENCE_NOTICE_OCTAVE, SEQUENCE_NOTICE_FX };
static uint8_t sequence_notice, sequence_notice_note, sequence_notice_instrument, sequence_notice_variant;
static uint8_t sequence_notice_knob, sequence_notice_value;
static int8_t sequence_notice_octave;
static char sequence_notice_fx_label[4];
static uint32_t sequence_notice_until;
static sequence_t sequence;
static sequence_t sequence_playback;
static sequence_slot_t sequence_slots[SEQUENCE_SLOTS];
/* Backing-part mutes set by ENV (chords), LFO (bass) or the GLO layer. */
static volatile uint8_t backing_mute;
#define MUTE_CHORDS 1u
#define MUTE_BASS 2u
#define MUTE_DRUMS 4u
static uint8_t arp_on;
static uint32_t arp_held;
static int32_t arp_key = -1;
static uint32_t arp_next_ms;
static volatile uint32_t hits[TRACKS];
static volatile uint32_t rendered_step;
static volatile uint32_t stereo_diff_samples;
static volatile uint8_t audio_playing;
static uint8_t audio_escape_play;
static uint32_t previous_buttons;
static uint32_t previous_keys;
static uint32_t startup_splash_until;
#define SPLASH_SWEEP_MS 900u
#define SPLASH_SWEEP_STEP_MS 40u
#define STARTUP_SCREEN_HEIGHT 240u
#define STARTUP_SPLASH_Y ((STARTUP_SCREEN_HEIGHT - STARTUP_SPLASH_HEIGHT) / 2u)
static uint8_t splash_booted;
static uint32_t splash_sweep_frame = 0xFFFFu;
static int8_t live_key_voice[NKEYS];
static uint8_t live_key_instrument[NKEYS];
static uint8_t live_key_chord_voice[NKEYS];
static uint8_t selected_instrument;
static volatile uint8_t piano_sampled = 1u;
static uint8_t chord_voice;
static volatile uint8_t chord_layout;
static uint8_t ui_chord_layout;
static int8_t instrument_octave[INSTRUMENTS];

/* Small live-performance phrase recorder: one free-running loop of key events. */
#define PHRASE_EVENTS 128u
#define PHRASE_MAX_MS 16000u
#define PHRASE_MIN_MS 250u
#define PHRASE_UP 0x80u
enum { PHRASE_IDLE, PHRASE_REC, PHRASE_PLAY };
typedef struct {
    uint16_t at;
    uint8_t key;
    uint8_t velocity;
    uint8_t instrument;
    int8_t octave;
} phrase_event_t;
static phrase_event_t phrase[PHRASE_EVENTS];
static uint8_t phrase_count, phrase_state, phrase_next, ui_phrase_state;
static uint16_t phrase_length;
static uint32_t phrase_start, phrase_down, phrase_held;
static volatile uint8_t drum_bass_boost;
static volatile uint8_t bass_sound, bass_one_shot, guitar_sound, guitar_one_shot;
static volatile uint8_t drum_chorus, drum_reverb, drum_compression;
static float drum_bass_low, drum_bass_gain;
static PLAT_POOL float drum_chorus_buffer[1024], drum_reverb_buffer[8192];
static uint32_t drum_chorus_pos, drum_reverb_pos, drum_chorus_age, drum_reverb_age, drum_effect_tail;
static uint8_t drum_chorus_mode, drum_reverb_mode;
static float drum_chorus_phase, drum_compressor_envelope;
typedef struct {
    float mod[1024], delay[8192];
    float phase, allpass;
    uint32_t mod_pos, delay_pos, mod_age, delay_age, tail;
    uint8_t mod_mode, delay_mode;
} string_fx_t;
static PLAT_POOL string_fx_t string_fx[3];
static volatile uint8_t string_controls[2][4];
static uint8_t ui_string_controls[2][4];
static volatile uint8_t voice_controls[TRACKS][4];
static uint8_t ui_voice_controls[TRACKS][4];
static volatile uint8_t chord_controls[4];
static uint8_t ui_chord_controls[4];
static volatile uint8_t cs80_preset;
static volatile uint8_t cs80_controls[4];
static uint8_t ui_cs80_preset;
static uint8_t ui_cs80_controls[4];
static uint8_t live_key_cs80[NKEYS];
#ifdef PLAT_HOST
static float last_live_increment;
#endif
static int32_t ui_tempo;
static uint8_t ui_instrument;
static uint8_t ui_chord_voice;
static int8_t ui_octave;
static uint32_t ui_hits[TRACKS];
static uint8_t ui_seq, ui_playing, ui_escape_play, ui_save_failed, ui_drawn, ui_drum_bass_boost;
static uint8_t ui_bass_mode, ui_guitar_mode;
static uint8_t knob_image_visible, knob_image_frame, knob_image_color;
static uint8_t ui_knob_image_visible, ui_knob_image_frame, ui_knob_image_color;
static uint8_t portrait_color, portrait_color_visible;
static uint8_t ui_portrait_color, ui_portrait_color_visible;
static uint32_t portrait_master_last;
static int32_t portrait_master_accumulator;
static uint8_t ui_drum_chorus, ui_drum_reverb, ui_drum_compression;
static uint32_t ui_drum_sample_event, ui_drum_sample_until;
static uint8_t ui_drum_sample, ui_drum_sample_visible;
static uint32_t samples_into_step;
static uint32_t step;
static float samples_per_step;
static float master_gain;
static float chorus_delay[1024];
static uint32_t chorus_position;
static uint32_t chorus_tail;
static float chorus_phase;
static voice_t juno[JUNO_VOICES], pro_one[PRO_ONE_VOICES], guitar[GUITAR_VOICES];
static piano_t piano[PIANO_VOICES];
static voice_t bass[MONO_VOICES], cs50[CS50_VOICES];
static cs80_voice_t cs80[CS80_VOICES];
static float piano_table[PIANO_TABLE];
static float tanh_table[TANH_TABLE + 1u];
static uint32_t noise_state = 1u;
#ifdef HAVE_SONG
static uint32_t song_tick;
static uint32_t song_index;
static float samples_into_tick;
#endif
static sample_voice_t drum_samples[ROSSFLM1_SAMPLE_COUNT];
static volatile uint8_t last_drum_sample;
static volatile uint32_t drum_sample_event;
static sample_voice_t bass_sample_voices[MONO_VOICES];
static sample_voice_t guitar_sample_voices[GUITAR_VOICES];
static uint32_t guitar_sample_next;
static uint32_t bass_sample_next;
static uint8_t live_key_bass_sample[NKEYS];
enum {
    ROSSFLM1_KICK,
    ROSSFLM1_SNARE,
    ROSSFLM1_HAT_CLOSED,
    ROSSFLM1_HAT_OPEN,
    ROSSFLM1_TOM_HI,
    ROSSFLM1_TOM_LO,
    ROSSFLM1_CLAP,
    ROSSFLM1_COWBELL,
    ROSSFLM1_TAMBOURINE,
    ROSSFLM1_CABASA,
    ROSSFLM1_CLAVE,
    ROSSFLM1_CONGA_HI,
    ROSSFLM1_CONGA_LO
};
static const int8_t *const drum_sample_data[ROSSFLM1_SAMPLE_COUNT] = {
    rossflm1_kick, rossflm1_snare, rossflm1_hat_closed, rossflm1_hat_open,
    rossflm1_tom_hi, rossflm1_tom_lo, rossflm1_clap, rossflm1_cowbell,
    rossflm1_tambourine, rossflm1_cabasa, rossflm1_clave, rossflm1_conga_hi,
    rossflm1_conga_lo
};
static const uint32_t drum_sample_lengths[ROSSFLM1_SAMPLE_COUNT] = {
    ROSSFLM1_KICK_LENGTH, ROSSFLM1_SNARE_LENGTH, ROSSFLM1_HAT_CLOSED_LENGTH,
    ROSSFLM1_HAT_OPEN_LENGTH, ROSSFLM1_TOM_HI_LENGTH, ROSSFLM1_TOM_LO_LENGTH,
    ROSSFLM1_CLAP_LENGTH, ROSSFLM1_COWBELL_LENGTH, ROSSFLM1_TAMBOURINE_LENGTH,
    ROSSFLM1_CABASA_LENGTH, ROSSFLM1_CLAVE_LENGTH, ROSSFLM1_CONGA_HI_LENGTH,
    ROSSFLM1_CONGA_LO_LENGTH
};
static const float drum_sample_levels[ROSSFLM1_SAMPLE_COUNT] = {
    0.62f, 0.40f, 0.22f, 0.18f, 0.30f, 0.30f, 0.30f, 0.25f, 0.20f,
    0.18f, 0.25f, 0.30f, 0.30f
};
static const char *const drum_sample_labels[ROSSFLM1_SAMPLE_COUNT] = {
    "KICK", "SNARE", "CLOSED HAT", "OPEN HAT", "HIGH TOM", "LOW TOM", "CLAP",
    "COWBELL", "TAMBOURINE", "CABASA", "CLAVE", "HIGH CONGA", "LOW CONGA"
};
static uint8_t save_failed;
static volatile uint32_t solo_mask = 0x7Fu;
static volatile uint8_t startup_requested;
typedef struct {
    uint32_t offset;
    uint32_t duration;
    uint8_t note;
    uint8_t velocity;
    uint8_t pro;
} startup_event_t;

/* Opening: four seconds of escape_from_ny.mid from 21.0 s on the Pro-1. The repeated
   low E2/E3 pulse is represented by E4 to keep the startup clear on the FM-1.
   One beat is 33075 samples (80 BPM). */
#define STARTUP_MS 3000u
#define SP(t, dur_q, n, v) {(t), (dur_q), (n), (v), 1u}
static const startup_event_t startup_events[] = {
    SP(0u, 132300u, 76u, 29u),
    SP(0u, 17912u, 64u, 127u), SP(0u, 115400u, 59u, 84u),
    SP(33075u, 22050u, 64u, 127u),
    SP(66150u, 9648u, 64u, 127u),
    SP(82688u, 40307u, 64u, 127u),
    SP(115763u, 7925u, 62u, 84u), SP(124031u, 7925u, 64u, 84u)
};
#define STARTUP_EVENTS (sizeof(startup_events) / sizeof(startup_events[0]))
static const uint32_t startup_end = 132300u;
static uint8_t startup_active;
static uint8_t startup_event;
static uint32_t startup_sample;
static const char *const track_names[TRACKS] = {
    "JUNO", "PIANO", "PRO-1", "GUITAR", "BASS", "LM-1", "CS-50/CS-80"
};
static const char *const instrument_names[INSTRUMENTS] = {
    "JUNO", "PIANO", "PRO-1", "GUITAR", "BASS", "LM-1", "CS-50/CS-80", "CHORD"
};
static const char *const chord_instrument_names[5] = {
    "CHORD PIANO", "CHORD JUNO", "CHORD CS-50", "CHORD PRO-1", "CHORD GUITAR"
};
static const uint8_t chord_instruments[5] = {1u, 0u, 6u, 2u, 3u};
static const uint32_t live_release[TRACKS] = {3000u, 3000u, 600u, 900u, 700u, 0u, 1400u};
/* Original, compact patch recipes; the names identify broad timbral inspirations. */
static const cs80_patch_t cs80_patches[CS80_PATCHES] = {
    {"CS80 TYRELL",      {-1, 0},  3, {2, 1}, {44, 50}, 76, 28, 72, 254, 18, 180, 96, {72, 28, 62, 92}, 0.80f},
    {"CS80 RAINDROPS",   { 0, 1}, -4, {1, 2}, {32, 54}, 92, 12, 24, 252,  4, 240, 82, {88, 14, 54, 88}, 0.85f},
    {"CS80 AFRICA",      {-1, 0},  6, {2, 0}, {46, 50}, 68, 34, 64, 250, 22, 160, 90, {62, 34, 72, 96}, 1.40f},
    {"CS80 BR BLUES",    {-1, 0}, -2, {0, 2}, {50, 42}, 62, 42, 48, 253, 12, 220, 92, {56, 42, 64, 94}, 1.20f},
    {"CS80 BR STRINGS",  { 0, 0},  8, {2, 1}, {42, 58}, 78, 20, 54, 254, 70, 260, 88, {74, 20, 58, 90}, 1.25f},
    {"CS80 ROSANNA",     {-1, 0},  5, {0, 1}, {50, 38}, 70, 38, 60, 251,  8, 180, 96, {66, 38, 58, 92}, 0.67f},
    {"CS80 BILLIE JEAN", {-1, 0}, -3, {1, 0}, {34, 50}, 58, 24, 36, 254,  5, 120, 94, {54, 24, 46, 96}, 0.57f},
    {"CS80 END TITLES",  {-1, 0},  4, {2, 2}, {44, 56}, 72, 18, 42, 255, 95, 360, 84, {68, 18, 64, 90}, 2.40f},
    {"CS80 BLADE BRASS", {-1, 0}, -6, {0, 2}, {50, 34}, 84, 52, 92, 248,  6, 200, 90, {82, 52, 72, 94}, 1.27f},
    {"CS80 CHARIOTS",    {-1, 0},  2, {2, 1}, {42, 52}, 80, 32, 82, 252, 38, 300, 88, {78, 32, 62, 92}, 0.86f}
};
static const int8_t pro_pattern[16] = {0, -1, 7, -1, 3, -1, 10, -1, 0, -1, 7, -1, 5, -1, 3, -1};
static const int8_t bass_pattern[16] = {0, -1, -1, 0, -1, -1, 7, -1, 0, -1, -1, 3, -1, -1, 7, -1};
static const int8_t cs_pattern[4] = {0, 7, 12, 7};
static const int8_t chord_roots[4] = {48, 44, 46, 43};
static const int8_t chord_intervals[4] = {0, 3, 7, 10};
static const int8_t piano_roots[4] = {60, 56, 58, 55};
static const int8_t piano_melody[4] = {7, 3, 7, 3};

static float note_increment(int32_t note)
{
    return 440.0f * fm_exp2f((float)(note - 69) * (1.0f / 12.0f)) / SAMPLE_RATE;
}

static float velocity_gain(uint32_t velocity)
{
    if (velocity > 127u)
        velocity = 127u;
    return 0.2f + 0.8f * ((float)velocity * (1.0f / 127.0f));
}

static void trigger_voice(voice_t *voice, int32_t note, uint32_t duration)
{
    voice->note = note;
    voice->increment = note_increment(note);
    voice->phase = 0.0f;
    voice->phase2 = 0.37f;
    voice->phase3 = 0.0f;
    voice->lp1 = 0.0f;
    voice->lp2 = 0.0f;
    voice->lp3 = 0.0f;
    voice->lp4 = 0.0f;
    voice->envelope = 1.0f;
    voice->envelope2 = 1.0f;
    voice->age = 0u;
    voice->duration = duration;
    voice->velocity_gain = 1.0f;
    voice->active = 1;
    voice->key = 0u;
}

static uint32_t trigger_pooled(voice_t *pool, uint32_t count, int32_t note, uint32_t duration)
{
    uint32_t i, pick = 0u;
    for (i = 0; i < count; i++) {
        if (!pool[i].active) {
            pick = i;
            break;
        }
        if (pool[i].age > pool[pick].age)
            pick = i;
    }
    trigger_voice(&pool[pick], note, duration);
    return pick;
}

static uint32_t trigger_cs80(int32_t note, uint32_t duration, uint8_t patch_index)
{
    uint32_t i, pick = 0u;
    const cs80_patch_t *patch = &cs80_patches[patch_index - 1u];
    for (i = 0; i < CS80_VOICES; i++) {
        if (!cs80[i].active) {
            pick = i;
            break;
        }
        if (cs80[i].age > cs80[pick].age)
            pick = i;
    }
    cs80[pick].phase[0] = 0.0f;
    cs80[pick].phase[1] = 0.37f;
    cs80[pick].increment[0] = note_increment(note + 12 * patch->octave[0]);
    cs80[pick].increment[1] = note_increment(note + 12 * patch->octave[1])
        * (1.0f + 0.0005776f * (float)patch->detune_cents);
    cs80[pick].filter_ic1[0] = cs80[pick].filter_ic1[1] = 0.0f;
    cs80[pick].filter_ic2[0] = cs80[pick].filter_ic2[1] = 0.0f;
    cs80[pick].filter_envelope = 1.0f;
    cs80[pick].velocity_gain = 1.0f;
    cs80[pick].age = 0u;
    cs80[pick].duration = duration;
    cs80[pick].patch = patch_index;
    cs80[pick].active = 1u;
    cs80[pick].key = 0u;
    return pick;
}

/* A monophonic part: the previous note fades out over `release` samples instead of being cut. */
static uint32_t trigger_mono(voice_t *pool, uint32_t count, int32_t note, uint32_t duration, uint32_t release)
{
    uint32_t i;
    for (i = 0; i < count; i++)
        if (pool[i].active && pool[i].duration > pool[i].age + release)
            pool[i].duration = pool[i].age + release;
    return trigger_pooled(pool, count, note, duration);
}

static float noise(void)
{
    noise_state = noise_state * 1664525u + 1013904223u;
    return (float)(int32_t)noise_state * (1.0f / 2147483648.0f);
}

static float poly_blep(float phase, float increment)
{
    float t;
    if (increment <= 0.0f || increment >= 0.5f)
        return 0.0f;
    if (phase < increment) {
        t = phase / increment;
        return t + t - t * t - 1.0f;
    }
    if (phase > 1.0f - increment) {
        t = (phase - 1.0f) / increment;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

static float bandlimited_saw(float phase, float increment)
{
    return 2.0f * phase - 1.0f - poly_blep(phase, increment);
}

static float bandlimited_pulse(float phase, float increment, float duty)
{
    float edge = phase - duty;
    if (edge < 0.0f)
        edge += 1.0f;
    return (phase < duty ? 1.0f : -1.0f)
         + poly_blep(phase, increment) - poly_blep(edge, increment);
}

/* Four-pole low-pass built from trapezoidal one-pole stages with the resonance feedback solved within the
   same sample (a standard zero-delay-feedback ladder), so the cutoff stays accurate and stable near the top
   of the range. The tan() pre-warp is refreshed only every 16 samples. `cutoff` is the one-pole coefficient
   (about 2*pi*fc/fs) used elsewhere in this file. */
static float ladder_tanh(float x)
{
    if (x > 3.0f)
        return 1.0f;
    if (x < -3.0f)
        return -1.0f;
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

static void ladder_update(voice_t *voice, float cutoff, float k)
{
    float g = fm_tanf(fm_minf(cutoff, 2.4f) * 0.5f);
    float big_g = g / (1.0f + g);
    float g2 = big_g * big_g;
    voice->ladder_g = big_g;
    voice->ladder_den = 1.0f / (1.0f + k * g2 * g2);
}

static float ladder_tick(voice_t *voice, float x, float k)
{
    float g = voice->ladder_g, v, y, u;
    u = x * 0.5f;
    if (k != 0.0f) {
        float feedback = (1.0f - g) * (g * g * g * voice->lp1 + g * g * voice->lp2 + g * voice->lp3 + voice->lp4);
        u = (u - k * feedback) * voice->ladder_den;
    }
    u = ladder_tanh(u);
    v = (u - voice->lp1) * g;
    y = v + voice->lp1;
    voice->lp1 = fm_flush(y + v);
    v = (y - voice->lp2) * g;
    u = v + voice->lp2;
    voice->lp2 = fm_flush(u + v);
    v = (u - voice->lp3) * g;
    y = v + voice->lp3;
    voice->lp3 = fm_flush(y + v);
    v = (y - voice->lp4) * g;
    u = v + voice->lp4;
    voice->lp4 = fm_flush(u + v);
    return u * 2.0f * (1.0f + 0.2f * k);
}

static void trigger_sample(sample_voice_t *voice, const int8_t *data, uint32_t length)
{
    voice->data = data;
    voice->adpcm_data = 0;
    voice->length = length;
    voice->age = 0u;
    voice->max_samples = 0u;
    voice->position = 0.0f;
    voice->rate = 1.0f;
    voice->gain = 1.0f;
    voice->loops = 0u;
}

static void trigger_drum_sample(uint32_t sample)
{
    sample %= ROSSFLM1_SAMPLE_COUNT;
    trigger_sample(&drum_samples[sample], drum_sample_data[sample], drum_sample_lengths[sample]);
    last_drum_sample = (uint8_t)sample;
    drum_sample_event++;
}

static const int16_t ima_step_table[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};
static const int8_t ima_index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8
};

static int16_t ima_adpcm_decode_next(const uint8_t *data, uint32_t *code_position,
                                     int32_t *predictor, int8_t *step_index)
{
    uint32_t nibble = (*code_position)++;
    uint8_t packed = data[2u + nibble / 2u];
    uint8_t code = (uint8_t)((packed >> ((nibble & 1u) * 4u)) & 0x0Fu);
    int32_t step = ima_step_table[(uint8_t)*step_index];
    int32_t delta = step >> 3;
    if (code & 1u)
        delta += step >> 2;
    if (code & 2u)
        delta += step >> 1;
    if (code & 4u)
        delta += step;
    *predictor += code & 8u ? -delta : delta;
    if (*predictor > 32767)
        *predictor = 32767;
    else if (*predictor < -32768)
        *predictor = -32768;
    *step_index += ima_index_table[code];
    if (*step_index < 0)
        *step_index = 0;
    else if (*step_index > 88)
        *step_index = 88;
    return (int16_t)*predictor;
}

static void trigger_adpcm_sample(sample_voice_t *voice, const uint8_t *data, uint32_t length)
{
    voice->data = 0;
    voice->adpcm_data = data;
    voice->length = length;
    voice->age = 0u;
    voice->max_samples = 0u;
    voice->adpcm_code_position = 0u;
    voice->adpcm_cache_index = 0u;
    voice->adpcm_predictor = (int16_t)(data[0] | ((uint16_t)data[1] << 8));
    voice->adpcm_step_index = 0;
    voice->adpcm_a = (int16_t)voice->adpcm_predictor;
    voice->adpcm_b = length > 1u
        ? ima_adpcm_decode_next(data, &voice->adpcm_code_position,
                                &voice->adpcm_predictor, &voice->adpcm_step_index)
        : voice->adpcm_a;
    voice->position = 0.0f;
    voice->loops = 0u;
}

static float sample_voice(sample_voice_t *voice, float level)
{
    uint32_t position;
    float sample, envelope = 1.0f;
    if (!voice->data && !voice->adpcm_data)
        return 0.0f;
    if (voice->max_samples && voice->age >= voice->max_samples) {
        voice->data = 0;
        voice->adpcm_data = 0;
        return 0.0f;
    }
    if (voice->max_samples && voice->max_samples - voice->age < 2205u)
        envelope = (float)(voice->max_samples - voice->age) * (1.0f / 2205.0f);
    if (voice->position >= voice->length) {
        if (voice->loops) {
            voice->loops--;
            voice->position -= (float)voice->length;
            voice->gain *= 0.6f;
        } else {
            voice->data = 0;
            voice->adpcm_data = 0;
            return 0.0f;
        }
    }
    position = (uint32_t)voice->position;
    if (voice->adpcm_data) {
        while (voice->adpcm_cache_index < position) {
            voice->adpcm_a = voice->adpcm_b;
            voice->adpcm_cache_index++;
            if (voice->adpcm_cache_index + 1u < voice->length)
                voice->adpcm_b = ima_adpcm_decode_next(voice->adpcm_data,
                    &voice->adpcm_code_position, &voice->adpcm_predictor, &voice->adpcm_step_index);
            else
                voice->adpcm_b = voice->adpcm_a;
        }
        sample = (float)voice->adpcm_a * (1.0f / 256.0f);
        if (voice->rate != 1.0f && position + 1u < voice->length) {
            float fraction = voice->position - (float)position;
            sample += (float)(voice->adpcm_b - voice->adpcm_a) * (fraction / 256.0f);
        }
    } else {
        sample = (float)voice->data[position];
        if (voice->rate != 1.0f && position + 1u < voice->length) {
            float fraction = voice->position - (float)position;
            sample += ((float)voice->data[position + 1u] - sample) * fraction;
        }
    }
    voice->position += voice->rate;
    voice->age++;
    return sample * (level * voice->gain * envelope / 128.0f);
}

static int16_t piano_sample_decode_next(piano_t *voice)
{
    return ima_adpcm_decode_next(voice->sample_data, &voice->sample_code_position,
                                 &voice->sample_predictor, &voice->sample_step_index);
}

static uint32_t trigger_bass_sample_variant(int32_t note, uint32_t velocity, uint8_t variant)
{
    static const int32_t sample_octave_offset = -24;
    static const int8_t *const pcm_data[9] = {
        0, bass_nomore, bass_808, 0, 0, 0, 0, 0, 0
    };
    static const uint8_t *const adpcm_data[9] = {
        0, 0, 0, 0, 0, 0, 0, bass_electric_a_sharp3, bass_acoustic_c2
    };
    static const uint32_t lengths[9] = {
        0u, BASS_NOMORE_LENGTH, BASS_808_LENGTH, 0u, 0u, 0u, 0u,
        BASS_ELECTRIC_LENGTH, BASS_ACOUSTIC_LENGTH
    };
    uint32_t index = bass_sample_next++ % MONO_VOICES;
    sample_voice_t *voice = &bass_sample_voices[index];
    if (adpcm_data[variant]) {
        int8_t root = variant == BASS_ELECTRIC_VARIANT ? bass_cc0_roots[0] : bass_cc0_roots[1];
        trigger_adpcm_sample(voice, adpcm_data[variant], lengths[variant]);
        voice->rate = 0.5f * fm_exp2f(
            (float)(note + sample_octave_offset - root) * (1.0f / 12.0f));
    } else {
        trigger_sample(voice, pcm_data[variant], lengths[variant]);
        voice->rate = fm_exp2f((float)(note - 36 + sample_octave_offset) * (1.0f / 12.0f));
    }
    voice->gain = velocity_gain(velocity) * (variant == 2u ? 2.0f : 1.0f);
    return index;
}

static uint32_t trigger_bass_sample(int32_t note, uint32_t velocity)
{
    return trigger_bass_sample_variant(note, velocity, bass_sound);
}

static uint32_t trigger_guitar_sample_variant(int32_t note, uint32_t velocity, uint8_t variant)
{
    static const uint8_t *const data[] = {
        guitar_electric_g2, guitar_nylon_g3, guitar_acoustic_e2
    };
    static const uint32_t lengths[] = {
        GUITAR_ELECTRIC_G2_LENGTH, GUITAR_NYLON_G3_LENGTH, GUITAR_ACOUSTIC_E2_LENGTH
    };
    uint32_t index = guitar_sample_next++ % GUITAR_VOICES;
    sample_voice_t *voice = &guitar_sample_voices[index];
    uint32_t sample = (uint32_t)variant - 1u;
    trigger_adpcm_sample(voice, data[sample], lengths[sample]);
    voice->rate = 0.5f * fm_exp2f((float)(note - guitar_sample_roots[sample]) * (1.0f / 12.0f));
    voice->gain = velocity_gain(velocity);
    return index;
}

static uint32_t trigger_guitar_sample(int32_t note, uint32_t velocity)
{
    return trigger_guitar_sample_variant(note, velocity, guitar_sound);
}

static uint32_t trigger_bass_note(int32_t note, uint32_t duration, uint32_t velocity)
{
    if (bass_sound)
        return trigger_bass_sample(note, velocity);
    {
        uint32_t index = trigger_mono(bass, MONO_VOICES, note, duration, 700u);
        bass[index].velocity_gain = velocity_gain(velocity);
        return index;
    }
}

static float drum_effects(float input, uint8_t chorus, uint8_t reverb, uint8_t compression)
{
    float x = input;
    if (chorus != drum_chorus_mode) {
        drum_chorus_mode = chorus;
        drum_chorus_age = 0u;
    }
    if (reverb != drum_reverb_mode) {
        drum_reverb_mode = reverb;
        drum_reverb_age = 0u;
    }
    if (input != 0.0f && (chorus || reverb))
        drum_effect_tail = reverb ? 88200u : 1024u;
    else if (drum_effect_tail)
        drum_effect_tail--;
    if (!chorus && !reverb)
        drum_effect_tail = 0u;

    if (chorus) {
        float wave, delayed = 0.0f;
        uint32_t delay;
        drum_chorus_phase += (0.25f + 0.15f * (float)chorus) / SAMPLE_RATE;
        if (drum_chorus_phase >= 1.0f)
            drum_chorus_phase -= 1.0f;
        wave = drum_chorus_phase < 0.5f ? drum_chorus_phase * 2.0f : 2.0f - drum_chorus_phase * 2.0f;
        delay = 300u + (uint32_t)(wave * (float)(100u + 50u * chorus));
        if (drum_chorus_age >= delay)
            delayed = drum_chorus_buffer[(drum_chorus_pos - delay) & 1023u];
        drum_chorus_buffer[drum_chorus_pos] = x;
        drum_chorus_pos = (drum_chorus_pos + 1u) & 1023u;
        if (drum_chorus_age < 1024u)
            drum_chorus_age++;
        x += delayed * (0.12f + 0.06f * (float)chorus);
    }
    if (reverb) {
        static const uint32_t delays[3] = {2205u, 4093u, 7001u};
        uint32_t delay = delays[reverb - 1u];
        uint32_t early = delay / 3u, middle = delay * 2u / 3u;
        float tap1 = drum_reverb_age >= early
            ? drum_reverb_buffer[(drum_reverb_pos - early) & 8191u] : 0.0f;
        float tap2 = drum_reverb_age >= middle
            ? drum_reverb_buffer[(drum_reverb_pos - middle) & 8191u] : 0.0f;
        float delayed = drum_reverb_age >= delay
            ? drum_reverb_buffer[(drum_reverb_pos - delay) & 8191u] : 0.0f;
        drum_reverb_buffer[drum_reverb_pos] = x + delayed * 0.48f;
        drum_reverb_pos = (drum_reverb_pos + 1u) & 8191u;
        if (drum_reverb_age < 8192u)
            drum_reverb_age++;
        x += (tap1 + tap2 + delayed) * 0.16f;
    }
    if (compression) {
        float level = x < 0.0f ? -x : x;
        float threshold = 0.28f - 0.05f * (float)compression;
        drum_compressor_envelope += (level > drum_compressor_envelope ? 0.03f : 0.001f)
                                  * (level - drum_compressor_envelope);
        if (drum_compressor_envelope > threshold)
            x *= threshold / (threshold + (drum_compressor_envelope - threshold)
                                      * (0.4f + 0.2f * (float)compression));
    } else {
        drum_compressor_envelope = 0.0f;
    }
    return x;
}

static float chorus_tap(float delay)
{
    float position = (float)((chorus_position - 1u) & 1023u) - delay;
    uint32_t index;
    float fraction;
    while (position < 0.0f)
        position += 1024.0f;
    index = (uint32_t)position & 1023u;
    fraction = position - (float)(uint32_t)position;
    return chorus_delay[index] + (chorus_delay[(index + 1u) & 1023u] - chorus_delay[index]) * fraction;
}

#ifdef HAVE_SONG
static void trigger_song_event(const song_event_t *e, float samples_per_tick)
{
    uint32_t duration = (uint32_t)((float)e->duration * samples_per_tick);
    uint32_t index;
    if (duration < 1500u)
        duration = 1500u;
    hits[e->part]++;
    switch (e->part) {
    case 0:
        index = trigger_pooled(juno, JUNO_VOICES, e->note, duration);
        juno[index].velocity_gain = velocity_gain(e->velocity);
        break;
    case 1:
        trigger_piano(e->note, duration, e->velocity);
        break;
    case 2:
        index = trigger_mono(pro_one, PRO_ONE_VOICES, e->note, duration, 600u);
        pro_one[index].velocity_gain = velocity_gain(e->velocity);
        break;
    case 3:
        index = trigger_pooled(guitar, GUITAR_VOICES, e->note, duration);
        guitar[index].velocity_gain = velocity_gain(e->velocity);
        break;
    case 4:
        trigger_bass_note(e->note, duration, e->velocity);
        break;
    case 6:
        if (cs80_preset) {
            index = trigger_cs80(e->note, duration, cs80_preset);
            cs80[index].velocity_gain = velocity_gain(e->velocity);
        } else {
            index = trigger_pooled(cs50, CS50_VOICES, e->note, duration);
            cs50[index].velocity_gain = velocity_gain(e->velocity);
        }
        break;
    default:
        trigger_drum_sample(e->note);
        break;
    }
}

static void song_advance(float samples_per_tick)
{
    if (samples_into_tick >= samples_per_tick) {
        samples_into_tick -= samples_per_tick;
        song_tick++;
        if (song_tick >= SONG_TICKS) {
            song_tick = 0u;
            song_index = 0u;
        }
    }
    samples_into_tick += 1.0f;
    while (song_index < SONG_EVENTS && song_events[song_index].tick <= song_tick)
        trigger_song_event(&song_events[song_index++], samples_per_tick);
    rendered_step = song_tick / (SONG_PPQ / 4u);
}
#endif

static void trigger_step(uint32_t s)
{
    uint32_t i;
    int32_t root = chord_roots[(s >> 2) & 3u];
    uint8_t mute = backing_mute;

    if ((s & 3u) == 0u && !(mute & MUTE_CHORDS)) {
        for (i = 0; i < 4u; i++) {
            trigger_voice(&juno[i], root + chord_intervals[i], 19845u);
            juno[i].velocity_gain = velocity_gain(90u);
        }
        hits[0]++;
        if (s == 0u || s == 8u) {
            for (i = 0; i < 3u; i++)
                trigger_piano(piano_roots[(s >> 2) & 3u] + (i == 0u ? 0 : i == 1u ? 3 : 7), 39690u, 90u);
            hits[1]++;
        }
        if (s == 0u || s == 8u) {
            trigger_voice(&guitar[0], root - 12, 19845u);
            trigger_voice(&guitar[1], root - 5, 19845u);
            guitar[0].velocity_gain = guitar[1].velocity_gain = velocity_gain(90u);
            hits[3]++;
        }
    }

    if ((s == 2u || s == 6u || s == 10u || s == 14u) && !(mute & MUTE_CHORDS)) {
        trigger_piano(piano_roots[(s >> 2) & 3u] + 12 + piano_melody[(s >> 2) & 3u], 12000u, 100u);
        hits[1]++;
    }
    if (pro_pattern[s] >= 0) {
        uint32_t index = trigger_mono(pro_one, PRO_ONE_VOICES, root + 12 + pro_pattern[s], 12000u, 600u);
        pro_one[index].velocity_gain = velocity_gain(90u);
        hits[2]++;
    }
    if (bass_pattern[s] >= 0 && !(mute & MUTE_BASS)) {
        trigger_bass_note(root - 24 + bass_pattern[s], 9000u, 90u);
        hits[4]++;
    }
    if (!(mute & MUTE_CHORDS)) {
        if (cs80_preset) {
            i = trigger_cs80(root + 12 + cs_pattern[s & 3u], 7000u, cs80_preset);
            cs80[i].velocity_gain = velocity_gain(90u);
        } else {
            i = trigger_pooled(cs50, CS50_VOICES, root + 12 + cs_pattern[s & 3u], 7000u);
            cs50[i].velocity_gain = velocity_gain(90u);
        }
        hits[6]++;
    }
    if (!(mute & MUTE_DRUMS)) {
        if (s == 0u || s == 8u) {
            trigger_drum_sample(ROSSFLM1_KICK);
            hits[5]++;
        }
        if (s == 4u || s == 12u) {
            trigger_drum_sample(ROSSFLM1_SNARE);
            hits[5]++;
        }
        if ((s & 1u) == 0u) {
            trigger_drum_sample(ROSSFLM1_HAT_CLOSED);
            hits[5]++;
        }
    }
    rendered_step = s;
}

/* Four-track drum step sequencer (kick, snare, hat, one free percussion sound), 16 steps on the backing clock. */
static void seq_trigger(uint32_t s)
{
    if (!(backing_mute & MUTE_DRUMS)) {
        if (settings.seq_mask[0] & (1u << s))
            trigger_drum_sample(ROSSFLM1_KICK);
        if (settings.seq_mask[1] & (1u << s))
            trigger_drum_sample(ROSSFLM1_SNARE);
        if (settings.seq_mask[2] & (1u << s))
            trigger_drum_sample(ROSSFLM1_HAT_CLOSED);
        if (settings.seq_perc[s])
            trigger_drum_sample((uint32_t)settings.seq_perc[s] - 1u);
    }
    rendered_step = s;
}

static void seq_clear(void)
{
    for (uint32_t i = 0; i < 3u; i++)
        settings.seq_mask[i] = 0u;
    for (uint32_t i = 0; i < 16u; i++)
        settings.seq_perc[i] = 0u;
}

static const uint8_t sequence_steps_per_bar[METER_COUNT] = {16u, 32u, 16u, 16u, 20u};
static const uint8_t sequence_note_steps[7] = {1u, 2u, 4u, 8u, 16u, 32u, 64u};
static const char *const sequence_note_names[7] = {"1/16", "1/8", "1/4", "1/2", "1", "2", "4"};
static const char *const sequence_meter_names[METER_COUNT] = {"4/4", "8/4", "8/8", "16/16", "5/4"};

static uint32_t sequence_slot_object(uint32_t slot)
{
    return slot >= SEQUENCE_USER_SLOTS
        ? (uint32_t)OBJ_SEQ_BUILTIN + slot - SEQUENCE_USER_SLOTS
        : (uint32_t)OBJ_SEQ0 + slot;
}

static const char *sequence_slot_name(uint32_t slot)
{
    return slot >= SEQUENCE_USER_SLOTS
        ? sequence_builtins[slot - SEQUENCE_USER_SLOTS].display_name
        : sequence_slots[slot].name;
}

static uint32_t sequence_event_step(uint32_t event)
{
    return event & 0xFFFu;
}

static uint8_t sequence_event_note(uint32_t event)
{
    return (uint8_t)((event >> 12) & 0x7Fu);
}

static uint8_t sequence_event_duration(uint32_t event)
{
    return (uint8_t)((event >> 19) & 7u);
}

static uint8_t sequence_event_track(uint32_t event)
{
    return (event & 0x80000000u) ? SEQUENCE_DRUM_TRACK : (uint8_t)((event >> 22) & 3u);
}

static uint8_t sequence_event_instrument(uint32_t event)
{
    return (uint8_t)((event >> 24) & 7u);
}

static uint8_t sequence_event_variant(uint32_t event)
{
    return (uint8_t)((event >> 27) & 15u);
}

static uint32_t sequence_event_make(uint32_t step_index, uint8_t note, uint8_t duration,
                                    uint8_t track, uint8_t instrument, uint8_t variant)
{
    return (step_index & 0xFFFu) | ((uint32_t)(note & 0x7Fu) << 12)
        | ((uint32_t)(duration & 7u) << 19) | ((uint32_t)(track & 3u) << 22)
        | ((uint32_t)(instrument & 7u) << 24) | ((uint32_t)(variant & 15u) << 27)
        | (track == SEQUENCE_DRUM_TRACK ? 0x80000000u : 0u);
}

static uint32_t sequence_event_length_steps(uint32_t event, uint32_t tempo)
{
    uint8_t duration = sequence_event_duration(event);
    if (sequence_event_track(event) == SEQUENCE_DRUM_TRACK) {
        uint32_t samples = drum_sample_lengths[sequence_event_note(event)];
        uint32_t steps = (samples * tempo + 661499u) / 661500u;
        if (duration != SEQUENCE_DRUM_NATURAL && steps > sequence_note_steps[duration])
            steps = sequence_note_steps[duration];
        return steps;
    }
    return sequence_note_steps[duration];
}

static float sequence_event_length_visual(uint32_t event, uint32_t tempo)
{
    uint8_t duration = sequence_event_duration(event);
    if (sequence_event_track(event) == SEQUENCE_DRUM_TRACK) {
        float steps = (float)drum_sample_lengths[sequence_event_note(event)] * (float)tempo / 661500.0f;
        if (duration != SEQUENCE_DRUM_NATURAL && steps > (float)sequence_note_steps[duration])
            steps = (float)sequence_note_steps[duration];
        return steps;
    }
    return (float)sequence_note_steps[duration];
}

static uint32_t sequence_total_steps(void)
{
    return (uint32_t)sequence.header.bars * sequence_steps_per_bar[sequence.header.meter];
}

static int sequence_variant_valid(uint8_t instrument, uint8_t variant)
{
    if (instrument < 3u)
        return variant == 0u;
    if (instrument == 3u)
        return variant < 4u;
    if (instrument == 4u)
        return variant == 0u || variant == 1u || variant == 2u
            || variant == BASS_ELECTRIC_VARIANT || variant == BASS_ACOUSTIC_VARIANT;
    if (instrument == 5u)
        return variant < ROSSFLM1_SAMPLE_COUNT;
    if (instrument == 6u)
        return variant <= CS80_PATCHES;
    return variant < 5u;
}

static uint32_t sequence_sound_cost(uint8_t instrument)
{
    return instrument == 7u && chord_controls[0] >= 4u && chord_controls[0] <= 6u ? 4u
        : instrument == 7u ? 3u : 1u;
}

static void sequence_reset(void)
{
    sequence.header.magic = SEQUENCE_MAGIC;
    sequence.header.count = 0u;
    sequence.header.version = SEQUENCE_VERSION;
    sequence.header.meter = METER_4_4;
    sequence.header.bars = 1u;
    sequence.header.tempo = (uint8_t)requested_tempo;
    sequence.header.drum_variant = 0u;
    sequence_tempo_initialized = 0u;
    for (uint32_t i = 0; i < SEQUENCE_MELODIC_TRACKS; i++) {
        sequence.header.instrument[i] = (uint8_t)i;
        sequence.header.variant[i] = 0u;
    }
    for (uint32_t i = 0; i < SEQUENCE_NAME_LENGTH; i++)
        sequence.header.name[i] = 0;
}

static int sequence_header_valid(const sequence_header_t *header, int size)
{
    uint32_t expected;
    if (header->magic != SEQUENCE_MAGIC
        || (header->version != SEQUENCE_VERSION && header->version != SEQUENCE_DRUM_TRACK_VERSION
            && header->version != SEQUENCE_TEMPO_VERSION
            && header->version != SEQUENCE_LEGACY_VERSION)
        || (header->version >= SEQUENCE_TEMPO_VERSION
            && (header->tempo < 70u || header->tempo > SEQUENCE_MAX_TEMPO))
        || (header->version >= SEQUENCE_DRUM_TRACK_VERSION && header->drum_variant >= ROSSFLM1_SAMPLE_COUNT)
        || header->meter >= METER_COUNT || !header->bars || header->bars > 128u
        || header->count > SEQUENCE_EVENT_MAX)
        return 0;
    expected = (uint32_t)sizeof *header + (uint32_t)header->count * sizeof(uint32_t);
    if ((uint32_t)size != expected)
        return 0;
    for (uint32_t i = 0; i < SEQUENCE_MELODIC_TRACKS; i++)
        if (header->instrument[i] >= INSTRUMENTS
            || !sequence_variant_valid(header->instrument[i], header->variant[i]))
            return 0;
    return 1;
}

static int sequence_contents_valid(const sequence_t *data)
{
    uint32_t steps_per_bar = sequence_steps_per_bar[data->header.meter];
    uint32_t total_steps = (uint32_t)data->header.bars * steps_per_bar;
    for (uint32_t i = 0; i < data->header.count; i++) {
        uint32_t event = data->events[i];
        uint32_t start = sequence_event_step(event);
        uint8_t duration = sequence_event_duration(event);
        uint8_t instrument = sequence_event_instrument(event);
        uint8_t variant = sequence_event_variant(event);
        if (start >= total_steps
            || (duration >= 7u && !(duration == SEQUENCE_DRUM_NATURAL
                && data->header.version == SEQUENCE_VERSION
                && sequence_event_track(event) == SEQUENCE_DRUM_TRACK))
            || sequence_event_track(event) >= SEQUENCE_TRACKS
            || instrument >= INSTRUMENTS || !sequence_variant_valid(instrument, variant)
            || (data->header.version < SEQUENCE_DRUM_TRACK_VERSION && (event & 0x80000000u))
            || (sequence_event_track(event) == SEQUENCE_DRUM_TRACK && instrument != 5u)
            || (instrument == 5u && (sequence_event_note(event) >= ROSSFLM1_SAMPLE_COUNT
                || (sequence_event_track(event) == SEQUENCE_DRUM_TRACK
                    && sequence_event_note(event) != variant))))
            return 0;
        if (start + sequence_event_length_steps(event, data->header.tempo) > total_steps)
            return 0;
    }
    for (uint32_t i = 0; i < data->header.count; i++) {
        uint32_t event = data->events[i];
        uint32_t start = sequence_event_step(event);
        uint32_t active = 0u;
        for (uint32_t j = 0; j < data->header.count; j++) {
            uint32_t other = data->events[j];
            uint32_t other_start = sequence_event_step(other);
            uint32_t other_end = other_start + sequence_event_length_steps(other, SEQUENCE_MAX_TEMPO);
            if (start >= other_start && start < other_end)
                active += sequence_sound_cost(sequence_event_instrument(other));
            if (active > SEQUENCE_VOICES)
                return 0;
        }
    }
    return 1;
}

static int sequence_load_slot(uint32_t slot, sequence_t *destination)
{
    uint32_t object, expected;
    int size, loaded;
    if (slot >= SEQUENCE_SLOTS)
        return 0;
    object = sequence_slot_object(slot);
    size = plat_store_size(object);
    if (size < (int)sizeof sequence.header || size > (int)PLAT_STORE_MAX)
        return 0;
    expected = (uint32_t)size;
    loaded = plat_store_load(object, destination, sizeof *destination);
    if (loaded != size || !sequence_header_valid(&destination->header, size)
        || !sequence_contents_valid(destination))
        return 0;
    if (destination->header.version == SEQUENCE_LEGACY_VERSION)
        destination->header.tempo = (uint8_t)requested_tempo;
    if (destination->header.version < SEQUENCE_DRUM_TRACK_VERSION)
        destination->header.drum_variant = 0u;
    return expected == (uint32_t)sizeof sequence.header
        + (uint32_t)destination->header.count * sizeof(uint32_t);
}

static void sequence_scan_slots(void)
{
    sequence_header_t header;
    sequence_slot_count = 0u;
    for (uint32_t slot = 0; slot < SEQUENCE_SLOTS; slot++) {
        int size = plat_store_size(sequence_slot_object(slot));
        int loaded = size >= (int)sizeof header && size <= (int)PLAT_STORE_MAX
            ? plat_store_head(sequence_slot_object(slot), &header, sizeof header) : -1;
        sequence_slots[slot].used = 0u;
        sequence_slots[slot].name[0] = 0;
        if (loaded == (int)sizeof header && sequence_header_valid(&header, size)) {
            sequence_slots[slot].used = 1u;
            for (uint32_t i = 0; i < SEQUENCE_NAME_LENGTH - 1u; i++) {
                char ch = header.name[i];
                sequence_slots[slot].name[i] = ch >= ' ' && ch <= '~' ? ch : 0;
                if (!ch)
                    break;
            }
            sequence_slots[slot].name[SEQUENCE_NAME_LENGTH - 1u] = 0;
            if (sequence_slot_count < SEQUENCE_SLOTS)
                sequence_slot_count++;
        }
    }
}

static int32_t sequence_find_event(uint8_t track, uint32_t step_index)
{
    for (uint32_t i = 0; i < sequence.header.count; i++) {
        uint32_t event = sequence.events[i];
        if (sequence_event_track(event) == track && sequence_event_step(event) == step_index
            && (track != SEQUENCE_DRUM_TRACK
                || sequence_event_variant(event) == sequence.header.drum_variant))
            return (int32_t)i;
    }
    return -1;
}

static int sequence_polyphony_ok(uint32_t start, uint32_t length, int32_t skip, uint8_t instrument)
{
    /* Reserve drum tails at the fastest tempo, so later tempo changes remain safe. */
    for (uint32_t step_index = start; step_index < start + length; step_index++) {
        uint32_t active = sequence_sound_cost(instrument);
        for (uint32_t i = 0; i < sequence.header.count; i++) {
            uint32_t event, event_start, event_end;
            if ((int32_t)i == skip)
                continue;
            event = sequence.events[i];
            event_start = sequence_event_step(event);
            event_end = event_start + sequence_event_length_steps(event, SEQUENCE_MAX_TEMPO);
            if (step_index >= event_start && step_index < event_end) {
                active += sequence_sound_cost(sequence_event_instrument(event));
                if (active > SEQUENCE_VOICES)
                    return 0;
            }
        }
    }
    return 1;
}

static void sequence_extend_to(uint32_t end_step)
{
    uint32_t steps_per_bar = sequence_steps_per_bar[sequence.header.meter];
    uint32_t bars = (end_step + steps_per_bar - 1u) / steps_per_bar;
    if (!bars)
        bars = 1u;
    if (bars > 128u)
        bars = 128u;
    if (bars > sequence.header.bars) {
        uint32_t lock = plat_lock();
        sequence.header.bars = (uint8_t)bars;
        if (sequence_playing && !sequence_audio_from_saved)
            sequence_audio_length = bars * steps_per_bar;
        plat_unlock(lock);
    }
}

static int sequence_store_slot(uint32_t slot)
{
    uint32_t bytes;
    if (slot >= SEQUENCE_SLOTS || sequence.header.count > SEQUENCE_EVENT_MAX)
        return -1;
    sequence.header.magic = SEQUENCE_MAGIC;
    sequence.header.version = SEQUENCE_VERSION;
    bytes = (uint32_t)sizeof sequence.header + (uint32_t)sequence.header.count * sizeof(uint32_t);
    return plat_store_save(sequence_slot_object(slot), &sequence, bytes);
}

static int sequence_builtin_legacy_header_matches(uint32_t index, const sequence_header_t *header)
{
    const sequence_builtin_t *builtin = &sequence_builtins[index];
    const char *name = index == 2u ? "the fog" : builtin->stored_name;
    uint32_t i = 0u;
    if (header->version != SEQUENCE_VERSION || header->count != builtin->count
        || header->meter != builtin->meter || header->bars != builtin->bars
        || header->tempo != builtin->tempo || header->drum_variant != 0u)
        return 0;
    for (i = 0u; i < SEQUENCE_MELODIC_TRACKS; i++)
        if (header->instrument[i] != builtin->instrument[i] || header->variant[i] != 0u)
            return 0;
    for (i = 0u; i < SEQUENCE_NAME_LENGTH && name[i]; i++)
        if (header->name[i] != name[i])
            return 0;
    for (; i < SEQUENCE_NAME_LENGTH; i++)
        if (header->name[i] != 0)
            return 0;
    return 1;
}

static uint32_t sequence_arrange_builtin_event(uint32_t index, uint32_t event)
{
    uint8_t piano_track = index == 1u ? 1u : 3u;
    uint8_t track = sequence_event_track(event);
    uint8_t instrument = sequence_event_instrument(event);
    uint8_t note = sequence_event_note(event);
    uint8_t variant = sequence_event_variant(event);
    if (track == piano_track && instrument == (index == 1u ? 0u : 6u))
        instrument = 1u;
    else if (index != 0u && track == 2u && instrument == 4u && variant == 0u) {
        note += 24u;
        variant = BASS_ELECTRIC_VARIANT;
    }
    return sequence_event_make(sequence_event_step(event), note, sequence_event_duration(event),
                               track, instrument, variant);
}

static uint32_t sequence_builtin_leading_steps(uint32_t index)
{
    return index == 0u || index == 2u ? 16u : 0u;
}

static uint32_t sequence_trim_builtin_event(uint32_t index, uint32_t event)
{
    return event - sequence_builtin_leading_steps(index);
}

static void sequence_arrange_builtin(uint32_t index)
{
    uint8_t piano_track = index == 1u ? 1u : 3u;
    sequence.header.instrument[piano_track] = 1u;
    if (index != 0u)
        sequence.header.variant[2] = BASS_ELECTRIC_VARIANT;
    for (uint32_t i = 0u; i < sequence.header.count; i++)
        sequence.events[i] = sequence_arrange_builtin_event(index, sequence.events[i]);
}

static int sequence_builtin_factory_matches(uint32_t index, const sequence_t *data, int trimmed)
{
    const sequence_builtin_t *builtin = &sequence_builtins[index];
    const sequence_header_t *header = &data->header;
    uint8_t piano_track = index == 1u ? 1u : 3u;
    uint32_t i;
    if (header->version != SEQUENCE_VERSION || header->count != builtin->count
        || header->meter != builtin->meter
        || header->bars != builtin->bars - (trimmed && sequence_builtin_leading_steps(index) ? 1u : 0u)
        || header->tempo != builtin->tempo || header->drum_variant != 0u)
        return 0;
    for (i = 0u; i < SEQUENCE_MELODIC_TRACKS; i++) {
        uint8_t instrument = i == piano_track ? 1u : builtin->instrument[i];
        uint8_t variant = index != 0u && i == 2u ? BASS_ELECTRIC_VARIANT : 0u;
        if (header->instrument[i] != instrument || header->variant[i] != variant)
            return 0;
    }
    for (i = 0u; i < SEQUENCE_NAME_LENGTH && builtin->stored_name[i]; i++)
        if (header->name[i] != builtin->stored_name[i])
            return 0;
    for (; i < SEQUENCE_NAME_LENGTH; i++)
        if (header->name[i] != 0)
            return 0;
    for (i = 0u; i < builtin->count; i++) {
        uint32_t event = sequence_arrange_builtin_event(index, builtin->events[i]);
        if (trimmed)
            event = sequence_trim_builtin_event(index, event);
        if (data->events[i] != event)
            return 0;
    }
    return 1;
}

static void sequence_prepare_builtin(uint32_t index, int legacy, int trimmed)
{
    const sequence_builtin_t *builtin = &sequence_builtins[index];
    const char *name = legacy && index == 2u ? "the fog" : builtin->stored_name;
    sequence_reset();
    sequence.header.count = (uint16_t)builtin->count;
    sequence.header.meter = builtin->meter;
    sequence.header.bars = builtin->bars;
    sequence.header.tempo = builtin->tempo;
    for (uint32_t i = 0; i < SEQUENCE_MELODIC_TRACKS; i++)
        sequence.header.instrument[i] = builtin->instrument[i];
    for (uint32_t i = 0; i < SEQUENCE_NAME_LENGTH - 1u && name[i]; i++)
        sequence.header.name[i] = name[i];
    for (uint32_t i = 0; i < builtin->count; i++)
        sequence.events[i] = builtin->events[i];
    if (!legacy) {
        sequence_arrange_builtin(index);
        if (trimmed && sequence_builtin_leading_steps(index)) {
            sequence.header.bars--;
            for (uint32_t i = 0u; i < sequence.header.count; i++)
                sequence.events[i] = sequence_trim_builtin_event(index, sequence.events[i]);
        }
    }
}

static void sequence_seed_builtin(uint32_t index)
{
    sequence_header_t header;
    const sequence_builtin_t *builtin = &sequence_builtins[index];
    uint32_t slot = SEQUENCE_USER_SLOTS + index;
    uint32_t object = sequence_slot_object(slot);
    int size = plat_store_size(object);
    int loaded = size >= (int)sizeof header && size <= (int)PLAT_STORE_MAX
        ? plat_store_head(object, &header, sizeof header) : -1;
    if (loaded == (int)sizeof header && sequence_header_valid(&header, size)) {
        int legacy = sequence_builtin_legacy_header_matches(index, &header);
        if (!legacy && (!sequence_builtin_leading_steps(index) || header.bars != builtin->bars))
            return;
        if (plat_store_load(object, &sequence, sizeof sequence) != size) {
            sequence_builtin_seed_failed = 1u;
            return;
        }
        if (legacy) {
            for (uint32_t i = 0u; i < builtin->count; i++)
                if (sequence.events[i] != builtin->events[i]) {
                    sequence_reset();
                    return;
                }
        } else if (!sequence_builtin_factory_matches(index, &sequence, 0)) {
                sequence_reset();
                return;
        }
    } else if (size >= (int)sizeof header && size <= (int)PLAT_STORE_MAX
               && loaded != (int)sizeof header) {
        sequence_builtin_seed_failed = 1u;
        return;
    }

    sequence_prepare_builtin(index, 0, 1);
    if (sequence_store_slot(slot) != 0)
        sequence_builtin_seed_failed = 1u;
    sequence_reset();
}

static void sequence_seed_builtins(void)
{
    for (uint32_t i = 0; i < SEQUENCE_BUILTIN_COUNT; i++)
        sequence_seed_builtin(i);
}

/* Live record: a drum key pressed while the sequencer runs lands on the nearest 16th step. */
static void seq_record(uint32_t key)
{
    uint32_t target;
    if (!requested_seq || selected_instrument != 5u)
        return;
    key %= ROSSFLM1_SAMPLE_COUNT;
    target = ((float)samples_into_step * 2.0f > samples_per_step) ? (step + 1u) & 15u : step & 15u;
    if (key < 3u)
        settings.seq_mask[key] |= (uint16_t)(1u << target);
    else
        settings.seq_perc[target] = (uint8_t)(key + 1u);
}

static void build_piano_table(void)
{
    uint32_t i;
    for (i = 0; i < PIANO_TABLE; i++)
        piano_table[i] = fm_sinf((float)i * (FM_TWO_PI / (float)PIANO_TABLE));
}

static float piano_lookup(float phase)
{
    float position = phase * (float)PIANO_TABLE;
    uint32_t index = (uint32_t)position & (PIANO_TABLE - 1u);
    float fraction = position - (float)(uint32_t)position;
    return piano_table[index] + (piano_table[(index + 1u) & (PIANO_TABLE - 1u)] - piano_table[index]) * fraction;
}

static void build_tanh_table(void)
{
    uint32_t i;
    for (i = 0u; i <= TANH_TABLE; i++)
        tanh_table[i] = fm_tanhf((float)i * (8.0f / (float)TANH_TABLE));
}

static float app_tanh(float x)
{
    float position, fraction, value;
    uint32_t index;
    if (x < 0.0f) {
        if (x <= -8.0f)
            return -1.0f;
        x = -x;
        value = -1.0f;
    } else {
        if (x >= 8.0f)
            return 1.0f;
        value = 1.0f;
    }
    position = x * ((float)TANH_TABLE / 8.0f);
    index = (uint32_t)position;
    fraction = position - (float)index;
    return value * (tanh_table[index] + (tanh_table[index + 1u] - tanh_table[index]) * fraction);
}

static float string_effects(uint32_t part, float input, const uint8_t controls[4], int reverb)
{
    static const uint32_t delay_lengths[3] = {2205u, 4410u, 7717u};
    string_fx_t *s = &string_fx[part];
    float x = input, delayed = 0.0f;
    uint8_t modulation = (uint8_t)(controls[1] | (controls[2] << 2));
    if (controls[0]) {
        float drive = 1.0f + (float)controls[0] * 1.5f;
        x = app_tanh(x * drive) / drive;
    }
    if (s->mod_mode != modulation) {
        s->mod_mode = modulation;
        s->mod_age = 0u;
        s->allpass = 0.0f;
    }
    if (s->delay_mode != controls[3]) {
        s->delay_mode = controls[3];
        s->delay_age = 0u;
    }
    if (input != 0.0f && (modulation || controls[3]))
        s->tail = controls[3] ? 88200u : 1024u;
    else if (s->tail)
        s->tail--;
    if (!modulation && !controls[3])
        s->tail = 0u;
    if (modulation) {
        float wave;
        uint32_t offset, flanger_offset;
        float flanger;
        s->phase += 0.6f / SAMPLE_RATE;
        if (s->phase >= 1.0f)
            s->phase -= 1.0f;
        wave = s->phase < 0.5f ? s->phase * 2.0f : 2.0f - s->phase * 2.0f;
        offset = controls[1] ? 240u + (uint32_t)(wave * 180.0f) : 20u + (uint32_t)(wave * 120.0f);
        flanger_offset = 20u + (uint32_t)(wave * 120.0f);
        flanger = s->mod_age >= flanger_offset ? s->mod[(s->mod_pos - flanger_offset) & 1023u] : 0.0f;
        if (s->mod_age >= offset)
            delayed = s->mod[(s->mod_pos - offset) & 1023u];
        s->mod[s->mod_pos] = x;
        s->mod_pos = (s->mod_pos + 1u) & 1023u;
        if (s->mod_age < 1024u)
            s->mod_age++;
        if (controls[1])
            x += delayed * (0.12f * (float)controls[1]);
        if (controls[2] == 1u || controls[2] == 2u)
            x += flanger * (controls[2] == 1u ? 0.35f : -0.35f);
        else if (controls[2] == 3u) {
            float a = 0.2f + 0.6f * wave;
            float y = s->allpass - a * x;
            s->allpass = x + a * y;
            x = (x + y * 0.55f) * 0.75f;
        }
    }
    if (controls[3]) {
        uint32_t offset = delay_lengths[controls[3] - 1u];
        delayed = s->delay_age >= offset ? s->delay[(s->delay_pos - offset) & 8191u] : 0.0f;
        s->delay[s->delay_pos] = x + delayed * 0.45f;
        s->delay_pos = (s->delay_pos + 1u) & 8191u;
        if (s->delay_age < 8192u)
            s->delay_age++;
        if (reverb) {
            uint32_t early = offset / 3u, middle = offset * 2u / 3u;
            float tap1 = s->delay_age > early ? s->delay[(s->delay_pos - 1u - early) & 8191u] : 0.0f;
            float tap2 = s->delay_age > middle ? s->delay[(s->delay_pos - 1u - middle) & 8191u] : 0.0f;
            x += (tap1 + tap2 + delayed) * 0.16f;
        } else {
            x += delayed * 0.4f;
        }
    }
    return x;
}

/* Additive piano: stretched partials with their own decay, a second detuned string on the lowest two,
   velocity-dependent brightness, a hammer thump and a damper fade at note end. */
static uint32_t trigger_piano(int32_t note, uint32_t duration, uint32_t velocity)
{
    static const float hammer[PIANO_PARTIALS] = {1.0f, 0.95f, 0.8f, 0.55f};
    static const float sample_decay[4] = {0.999992f, 0.999994f, 0.999996f, 0.999998f};
    piano_t *voice = &piano[0];
    uint32_t i, k;
    float base = note_increment(note);
    float v = (float)velocity * (1.0f / 127.0f);
    float brightness = 0.35f + 0.6f * v;
    float stretch = 0.00012f * fm_exp2f((float)(note - 60) * 0.06f);
    float t1 = 4.0f * fm_exp2f(-(float)(note - 36) * 0.0375f);
    float scale = 1.0f;
    for (i = 0; i < PIANO_VOICES; i++) {
        if (!piano[i].active) {
            voice = &piano[i];
            break;
        }
        if (piano[i].age > voice->age)
            voice = &piano[i];
    }
    voice->sampled = piano_sampled;
    voice->sample_data = 0;
    voice->sample_length = 0u;
    voice->sample_position = 0.0f;
    voice->sample_rate = 0.0f;
    voice->sample_filter = 0.0f;
    voice->sample_envelope = 1.0f;
    voice->age = 0u;
    voice->duration = duration;
    voice->active = 1u;
    voice->key = 0u;
    if (voice->sampled) {
        uint32_t zone = 0u;
        int32_t nearest = note - piano_sample_roots[0];
        if (nearest < 0)
            nearest = -nearest;
        for (i = 1u; i < PIANO_SAMPLE_COUNT; i++) {
            int32_t distance = note - piano_sample_roots[i];
            if (distance < 0)
                distance = -distance;
            if (distance < nearest) {
                zone = i;
                nearest = distance;
            }
        }
        voice->sample_data = piano_sample_data[zone];
        voice->sample_length = piano_sample_lengths[zone];
        voice->sample_predictor = (int16_t)((uint16_t)voice->sample_data[0]
            | ((uint16_t)voice->sample_data[1] << 8));
        voice->sample_step_index = 0;
        voice->sample_code_position = 0u;
        voice->sample_cache_index = 0u;
        voice->sample_a = (int16_t)voice->sample_predictor;
        voice->sample_b = piano_sample_decode_next(voice);
        voice->sample_rate = 0.5f * fm_exp2f(
            (float)(note - piano_sample_roots[zone]) * (1.0f / 12.0f));
        voice->sample_brightness = 0.38f + 0.52f * v;
        voice->sample_decay = sample_decay[voice_controls[1][1]];
        voice->level = 0.35f + 0.65f * v;
        voice->inc[0] = base;
        return (uint32_t)(voice - piano);
    }
    for (k = 0; k < PIANO_PARTIALS; k++) {
        float n = (float)(k + 1u);
        float t = t1 / (1.0f + 0.9f * (n - 1.0f));
        float f = base * n * (1.0f + stretch * n * n * 0.5f);
        voice->inc[k] = f;
        voice->phase[k] = 0.0f;
        voice->amp[k] = hammer[k] * scale * (0.3f + 0.7f * v);
        voice->decay[k] = 1.0f - 1.0f / (t * SAMPLE_RATE);
        scale *= brightness;
    }
    for (k = 0; k < 2u; k++) {
        voice->inc[PIANO_PARTIALS + k] = voice->inc[k] * 1.0014f;
        voice->phase[PIANO_PARTIALS + k] = 0.5f;
        voice->amp[PIANO_PARTIALS + k] = voice->amp[k] * 0.8f;
        voice->decay[PIANO_PARTIALS + k] = voice->decay[k] * 0.99999f;
    }
    voice->punch = 1.0f;
    voice->thump = 0.0f;
    voice->level = (0.35f + 0.65f * v);
    return (uint32_t)(voice - piano);
}

#define CHORD_TYPES 8u
#define CHORD_SHAPE_KEY 12u

static uint32_t trigger_chord_voice(int32_t root, uint32_t duration, uint32_t velocity,
                                    uint32_t key, uint8_t voice)
{
    /* major, minor, sus2, sus4, dominant 7, major 7, minor 7, diminished; 0 = no fourth note */
    static const int8_t intervals[8][3] = {
        {0, 4, 7}, {0, 3, 7}, {0, 2, 7}, {0, 5, 7}, {0, 4, 7}, {0, 4, 7}, {0, 3, 7}, {0, 3, 6}
    };
    static const int8_t sevenths[8] = {0, 0, 0, 0, 10, 11, 10, 0};
    int32_t notes[4];
    uint32_t i, count = 3u, inversion = chord_controls[2];
    uint32_t first = 0u;
    for (i = 0; i < 3u; i++)
        notes[i] = root + intervals[chord_controls[0]][i];
    if (sevenths[chord_controls[0]])
        notes[count++] = root + sevenths[chord_controls[0]];
    if (inversion < 3u) {
        for (i = 0; i < inversion; i++) {
            int32_t note = notes[0];
            notes[0] = notes[1];
            notes[1] = notes[2];
            notes[2] = note + 12;
        }
    } else {
        notes[2] += 12;
    }
    if (chord_controls[1] == 1u)
        notes[1] += 12;
    else if (chord_controls[1] == 2u) {
        notes[0] -= 12;
        notes[2] += 12;
    } else if (chord_controls[1] == 3u) {
        notes[2] += 12;
    }
    notes[2] += 12 * (int32_t)chord_controls[3];
    for (i = 0; i < count; i++) {
        uint32_t index;
        uint8_t instrument = chord_instruments[voice % 5u];
        if (instrument == 1u) {
            index = trigger_piano(notes[i], duration, velocity * 56u / 100u);
            piano[index].key = (uint8_t)(key + 1u);
        } else if (instrument == 0u) {
            index = trigger_pooled(juno, JUNO_VOICES, notes[i], duration);
            juno[index].key = (uint8_t)(key + 1u);
            juno[index].velocity_gain = velocity_gain(velocity) * 0.467f;
        } else if (instrument == 6u) {
            index = trigger_pooled(cs50, CS50_VOICES, notes[i], duration);
            cs50[index].key = (uint8_t)(key + 1u);
            cs50[index].velocity_gain = velocity_gain(velocity) * 0.56f;
        } else if (instrument == 2u) {
            index = trigger_pooled(pro_one, PRO_ONE_VOICES, notes[i], duration);
            pro_one[index].key = (uint8_t)(key + 1u);
            pro_one[index].velocity_gain = velocity_gain(velocity) * 0.56f;
        } else {
            index = trigger_pooled(guitar, GUITAR_VOICES, notes[i], duration);
            guitar[index].key = (uint8_t)(key + 1u);
            guitar[index].velocity_gain = velocity_gain(velocity) * 0.56f;
        }
        if (i == 0u)
            first = index;
    }
    if (key < NKEYS)
        live_key_chord_voice[key] = chord_instruments[voice % 5u];
    return first;
}

static uint32_t trigger_chord(int32_t root, uint32_t duration, uint32_t velocity, uint32_t key)
{
    return trigger_chord_voice(root, duration, velocity, key, chord_voice);
}

static void sequence_trigger_event(uint32_t event)
{
    uint8_t instrument = sequence_event_instrument(event);
    uint8_t variant = sequence_event_variant(event);
    uint8_t note = sequence_event_note(event);
    uint8_t duration_index = sequence_event_duration(event);
    uint32_t duration, index;
    if (duration_index >= 7u && !(duration_index == SEQUENCE_DRUM_NATURAL
                                  && sequence_event_track(event) == SEQUENCE_DRUM_TRACK))
        return;
    duration = duration_index == SEQUENCE_DRUM_NATURAL ? 0u
        : (uint32_t)((float)sequence_note_steps[duration_index] * samples_per_step);
    if (duration < 1500u)
        duration = 1500u;
    if (instrument < TRACKS)
        hits[instrument]++;
    switch (instrument) {
    case 0u:
        index = trigger_pooled(juno, JUNO_VOICES, note, duration);
        juno[index].velocity_gain = velocity_gain(sequence_audio_events ? 65u : 100u);
        break;
    case 1u:
        trigger_piano(note, duration, sequence_audio_events
                      ? (sequence_event_track(event) == 3u ? 40u : 70u) : 100u);
        break;
    case 2u:
        index = trigger_mono(pro_one, PRO_ONE_VOICES, note, duration, 600u);
        pro_one[index].velocity_gain = velocity_gain(sequence_audio_events ? 55u : 100u);
        break;
    case 3u:
        if (variant) {
            index = trigger_guitar_sample_variant(note, 100u, variant);
            if (!guitar_one_shot)
                guitar_sample_voices[index].max_samples = duration;
        }
        else {
            index = trigger_pooled(guitar, GUITAR_VOICES, note, guitar_one_shot ? 132300u : duration);
            guitar[index].velocity_gain = velocity_gain(100u);
        }
        break;
    case 4u:
        if (variant) {
            index = trigger_bass_sample_variant(note, 100u, variant);
            if (!bass_one_shot)
                bass_sample_voices[index].max_samples = duration;
        } else
            trigger_bass_note(note, duration, 100u);
        break;
    case 5u:
        trigger_drum_sample(note);
        drum_samples[note].gain = SEQUENCE_DRUM_LEVEL;
        if (duration_index != SEQUENCE_DRUM_NATURAL)
            drum_samples[note].max_samples = duration;
        break;
    case 6u:
        if (variant) {
            index = trigger_cs80(note, duration, variant);
            cs80[index].velocity_gain = velocity_gain(100u);
        } else {
            index = trigger_pooled(cs50, CS50_VOICES, note, duration);
            cs50[index].velocity_gain = velocity_gain(100u);
        }
        break;
    default:
        trigger_chord_voice(note, duration, 100u, NKEYS,
                            variant < 5u ? variant : 0u);
        break;
    }
}

typedef struct {
    uint8_t *active;
    sample_voice_t *sample;
    uint32_t age;
    uint32_t count;
    uint8_t priority;
} sequence_steal_candidate_t;

static void sequence_consider_steal_voice(sequence_steal_candidate_t *candidate,
                                           uint8_t *active, sample_voice_t *sample,
                                           uint32_t age, uint8_t priority)
{
    if (active ? !*active : !sample->data && !sample->adpcm_data)
        return;
    candidate->count++;
    if (priority < candidate->priority || (priority == candidate->priority && age > candidate->age)) {
        candidate->active = active;
        candidate->sample = sample;
        candidate->age = age;
        candidate->priority = priority;
    }
}

static void sequence_trigger_stealing(uint32_t event)
{
    uint8_t instrument = sequence_event_instrument(event);
    uint8_t note = sequence_event_note(event);
    uint8_t priority = instrument == 5u ? 3u : instrument == 0u ? 1u : 2u;
    sequence_steal_candidate_t candidate = {0, 0, 0u, 0u, 4u};
    for (uint32_t i = 0u; i < JUNO_VOICES; i++)
        sequence_consider_steal_voice(&candidate, &juno[i].active, 0, juno[i].age, 1u);
    for (uint32_t i = 0u; i < PIANO_VOICES; i++)
        sequence_consider_steal_voice(&candidate, &piano[i].active, 0, piano[i].age, 2u);
    for (uint32_t i = 0u; i < PRO_ONE_VOICES; i++)
        sequence_consider_steal_voice(&candidate, &pro_one[i].active, 0, pro_one[i].age, 2u);
    for (uint32_t i = 0u; i < GUITAR_VOICES; i++) {
        sequence_consider_steal_voice(&candidate, &guitar[i].active, 0, guitar[i].age, 2u);
        if (instrument != 3u || i != guitar_sample_next % GUITAR_VOICES)
            sequence_consider_steal_voice(&candidate, 0, &guitar_sample_voices[i],
                                           guitar_sample_voices[i].age, 2u);
    }
    for (uint32_t i = 0u; i < MONO_VOICES; i++) {
        sequence_consider_steal_voice(&candidate, &bass[i].active, 0, bass[i].age, 2u);
        if (instrument != 4u || i != bass_sample_next % MONO_VOICES)
            sequence_consider_steal_voice(&candidate, 0, &bass_sample_voices[i],
                                           bass_sample_voices[i].age, 2u);
    }
    for (uint32_t i = 0u; i < ROSSFLM1_SAMPLE_COUNT; i++)
        if (instrument != 5u || note != i)
            sequence_consider_steal_voice(&candidate, 0, &drum_samples[i], drum_samples[i].age, 3u);
    if (candidate.count > sequence_audio_steal_limit
        || (candidate.count == sequence_audio_steal_limit && candidate.priority > priority)) {
#ifdef PLAT_HOST
        sequence_steal_drops++;
#endif
        return;
    }
    if (candidate.count == sequence_audio_steal_limit) {
        if (candidate.active)
            *candidate.active = 0u;
        else {
            candidate.sample->data = 0;
            candidate.sample->adpcm_data = 0;
        }
        candidate.count--;
#ifdef PLAT_HOST
        sequence_steals++;
#endif
    }
    sequence_trigger_event(event);
#ifdef PLAT_HOST
    if (candidate.count + 1u > sequence_steal_peak)
        sequence_steal_peak = candidate.count + 1u;
#endif
}

static void sequence_trigger_audio_step(const sequence_t *source)
{
    if (sequence_audio_events) {
        while (sequence_audio_event_index < sequence_audio_event_count
               && sequence_event_step(sequence_audio_events[sequence_audio_event_index])
                    - sequence_builtin_leading_steps(0u) == sequence_audio_step)
            sequence_trigger_stealing(sequence_audio_events[sequence_audio_event_index++]);
        return;
    }
    for (uint32_t i = 0; i < source->header.count; i++) {
        if (sequence_event_step(source->events[i]) == sequence_audio_step) {
            if (sequence_audio_stealing)
                sequence_trigger_stealing(source->events[i]);
            else
                sequence_trigger_event(source->events[i]);
        }
    }
}

static void sequence_audio_advance(void)
{
    const sequence_t *source = sequence_audio_from_saved ? &sequence_playback : &sequence;
    if (sequence_stop_requested) {
        sequence_stop_requested = 0u;
        sequence_audio_started = 0u;
        stop_audio_voices();
    }
    if (!sequence_playing)
        return;
    if (!sequence_audio_started) {
        sequence_audio_started = 1u;
#ifdef PLAT_HOST
        sequence_steals = sequence_steal_drops = sequence_steal_peak = 0u;
        sequence_mix_peak = 0.0f;
        sequence_mix_over_one = 0u;
        sequence_mix_peak_step = 0u;
#endif
        sequence_audio_step = sequence_audio_start;
        sequence_audio_samples = 0.0f;
        sequence_audio_event_index = 0u;
        stop_audio_voices();
        sequence_trigger_audio_step(source);
        return;
    }
    sequence_audio_samples += 1.0f;
    if (sequence_audio_samples < samples_per_step)
        return;
    sequence_audio_samples -= samples_per_step;
    sequence_audio_step++;
    if (sequence_audio_step >= sequence_audio_length) {
        sequence_playing = 0u;
        sequence_audio_started = 0u;
        return;
    }
    sequence_trigger_audio_step(source);
}

static void release_chord_key(uint32_t key)
{
    uint8_t instrument = live_key_chord_voice[key];
    uint8_t key_id = (uint8_t)(key + 1u);
    if (instrument == 1u) {
        for (uint32_t i = 0; i < PIANO_VOICES; i++)
            if (piano[i].key == key_id) {
                if (piano[i].active)
                    piano[i].duration = piano[i].age + live_release[1];
                piano[i].key = 0u;
            }
    } else {
        voice_t *voices;
        uint32_t count;
        if (instrument == 0u) {
            voices = juno;
            count = JUNO_VOICES;
        } else if (instrument == 6u) {
            voices = cs50;
            count = CS50_VOICES;
        } else if (instrument == 3u) {
            voices = guitar;
            count = GUITAR_VOICES;
        } else {
            voices = pro_one;
            count = PRO_ONE_VOICES;
        }
        for (uint32_t i = 0; i < count; i++)
            if (voices[i].key == key_id) {
                voices[i].duration = voices[i].age + live_release[instrument];
                voices[i].key = 0u;
            }
    }
    live_key_chord_voice[key] = 0u;
}

static void release_live_key(uint32_t key)
{
    uint32_t instrument = live_key_instrument[key];
    int32_t index = live_key_voice[key];
    voice_t *voice = 0;
    if (instrument == 3u && live_key_bass_sample[key]) {
        if (index >= 0 && (uint32_t)index < GUITAR_VOICES) {
            sample_voice_t *sample_voice = &guitar_sample_voices[index];
            if (!guitar_one_shot && sample_voice->adpcm_data)
                sample_voice->max_samples = sample_voice->age + 2205u;
        }
        live_key_bass_sample[key] = 0u;
        live_key_voice[key] = -1;
        return;
    }
    if (instrument == 4u && live_key_bass_sample[key]) {
        if (!bass_one_shot && index >= 0 && (uint32_t)index < MONO_VOICES) {
            sample_voice_t *sample_voice = &bass_sample_voices[index];
            if (sample_voice->data || sample_voice->adpcm_data)
                sample_voice->max_samples = sample_voice->age + 2205u;
        }
        live_key_bass_sample[key] = 0u;
        live_key_voice[key] = -1;
        return;
    }
    if (index < 0)
        return;
    switch (instrument) {
    case 0u: voice = &juno[index]; break;
    case 1u:
        if (piano[index].key == (uint8_t)(key + 1u)) {
            if (piano[index].active)
                piano[index].duration = piano[index].age + live_release[instrument];
            piano[index].key = 0u;
        }
        break;
    case 7u:
        release_chord_key(key);
        break;
    case 2u: voice = &pro_one[index]; break;
    case 3u:
        if (guitar_one_shot) {
            if (guitar[index].key == (uint8_t)(key + 1u))
                guitar[index].key = 0u;
        }
        else
            voice = &guitar[index];
        break;
    case 4u:
        if (bass_one_shot) {
            if (bass[index].key == (uint8_t)(key + 1u))
                bass[index].key = 0u;
        } else {
            voice = &bass[index];
        }
        break;
    case 6u:
        if (live_key_cs80[key]) {
            if (cs80[index].key == (uint8_t)(key + 1u)) {
                if (cs80[index].active) {
                    uint32_t release = (uint32_t)cs80_patches[cs80[index].patch - 1u].release_ms * 44u;
                    cs80[index].duration = cs80[index].age + release;
                }
                cs80[index].key = 0u;
            }
            live_key_cs80[key] = 0u;
            live_key_voice[key] = -1;
            return;
        }
        voice = &cs50[index];
        break;
    default: break;
    }
    if (voice && voice->key == (uint8_t)(key + 1u)) {
        voice->duration = voice->age + live_release[instrument];
        voice->key = 0u;
    }
    live_key_voice[key] = -1;
}

static void trigger_live_key(uint32_t key, uint32_t velocity)
{
    uint32_t index;
    int32_t note = 53 + (int32_t)key + 12 * instrument_octave[selected_instrument];
    live_key_instrument[key] = selected_instrument;
    live_key_bass_sample[key] = 0u;
    live_key_cs80[key] = 0u;
    switch (selected_instrument) {
    case 0u:
        index = trigger_pooled(juno, JUNO_VOICES, note, 132300u);
        juno[index].key = (uint8_t)(key + 1u);
        juno[index].velocity_gain = velocity_gain(velocity);
        break;
    case 1u:
        index = trigger_piano(note, 132300u, velocity);
        piano[index].key = (uint8_t)(key + 1u);
        break;
    case 2u:
        index = trigger_mono(pro_one, PRO_ONE_VOICES, note, 132300u, live_release[2]);
        pro_one[index].key = (uint8_t)(key + 1u);
        pro_one[index].velocity_gain = velocity_gain(velocity);
        break;
    case 3u:
        if (guitar_sound) {
            index = trigger_guitar_sample(note, velocity);
            live_key_bass_sample[key] = 1u;
            live_key_voice[key] = (int8_t)index;
#ifdef PLAT_HOST
            last_live_increment = 0.0f;
#endif
            return;
        }
        index = trigger_pooled(guitar, GUITAR_VOICES, note, 132300u);
        guitar[index].key = (uint8_t)(key + 1u);
        guitar[index].velocity_gain = velocity_gain(velocity);
        break;
    case 4u:
        if (bass_sound) {
            index = trigger_bass_note(note, 132300u, velocity);
            live_key_bass_sample[key] = 1u;
        } else {
            index = trigger_bass_note(note, 132300u, velocity);
            bass[index].key = (uint8_t)(key + 1u);
        }
        break;
    case 7u:
        if (chord_layout && key >= CHORD_SHAPE_KEY && key - CHORD_SHAPE_KEY < CHORD_TYPES)
            chord_controls[0] = (uint8_t)(key - CHORD_SHAPE_KEY);
        index = trigger_chord(note, 132300u, velocity, key);
        break;
    case 5u:
        trigger_drum_sample(key);
#ifdef PLAT_HOST
        last_live_increment = 0.0f;
#endif
        live_key_voice[key] = -1;
        return;
    default:
        if (selected_instrument == 6u && cs80_preset) {
            index = trigger_cs80(note, 132300u, cs80_preset);
            cs80[index].key = (uint8_t)(key + 1u);
            cs80[index].velocity_gain = velocity_gain(velocity);
            live_key_cs80[key] = 1u;
        } else {
            index = trigger_pooled(cs50, CS50_VOICES, note, 132300u);
            cs50[index].key = (uint8_t)(key + 1u);
            cs50[index].velocity_gain = velocity_gain(velocity);
        }
        break;
    }
    live_key_voice[key] = (int8_t)index;
#ifdef PLAT_HOST
    switch (selected_instrument) {
    case 0u: last_live_increment = juno[index].increment; break;
    case 1u: last_live_increment = piano[index].inc[0]; break;
    case 2u: last_live_increment = pro_one[index].increment; break;
    case 3u: last_live_increment = guitar[index].increment; break;
    case 4u: last_live_increment = bass_sound ? 0.0f : bass[index].increment; break;
    case 7u:
        switch (live_key_chord_voice[key]) {
        case 0u: last_live_increment = juno[index].increment; break;
        case 1u: last_live_increment = piano[index].inc[0]; break;
        case 2u: last_live_increment = cs50[index].increment; break;
        case 3u: last_live_increment = pro_one[index].increment; break;
        default: last_live_increment = guitar[index].increment; break;
        }
        break;
    default:
        last_live_increment = selected_instrument == 6u && live_key_cs80[key]
            ? cs80[index].increment[0] : cs50[index].increment;
        break;
    }
#endif
}

static float piano_voice(piano_t *voice, float level, const uint8_t controls[4])
{
    static const float decay_scales[4] = {1.0f, 1.0f / 1.4f, 1.0f / 1.8f, 1.0f / 2.2f};
    float sample, coefficient, remaining, release;
    float sum = 0.0f, thump;
    float decay_scale = decay_scales[controls[1]];
    float brightness = 1.0f + 0.18f * (float)controls[0];
    uint32_t k;
    if (voice->sampled) {
        uint32_t position;
        if (voice->age >= voice->duration || !voice->sample_data
            || voice->sample_position >= (float)voice->sample_length) {
            voice->active = 0u;
            voice->key = 0u;
            voice->sample_data = 0;
            return 0.0f;
        }
        voice->age++;
        position = (uint32_t)voice->sample_position;
        while (position > voice->sample_cache_index) {
            voice->sample_a = voice->sample_b;
            voice->sample_cache_index++;
            if (voice->sample_cache_index + 1u < voice->sample_length)
                voice->sample_b = piano_sample_decode_next(voice);
            else
                voice->sample_b = voice->sample_a;
        }
        sample = (float)voice->sample_a;
        sample += ((float)voice->sample_b - sample)
            * (voice->sample_position - (float)voice->sample_cache_index);
        voice->sample_position += voice->sample_rate;
        coefficient = voice->sample_brightness + 0.025f * (float)controls[0];
        if (coefficient > 0.98f)
            coefficient = 0.98f;
        voice->sample_filter += coefficient * (sample * (1.0f / 32768.0f) - voice->sample_filter);
        voice->sample_envelope *= voice->sample_decay;
        remaining = (float)(voice->duration - voice->age);
        release = 1.0f;
        if (remaining < 3000.0f)
            release = remaining * (1.0f / 3000.0f);
        return voice->sample_filter * voice->sample_envelope * release * voice->level * level * 2.6f;
    }
    if (voice->age >= voice->duration) {
        voice->active = 0u;
        voice->key = 0u;
        return 0.0f;
    }
    voice->age++;
    for (k = 0; k < PIANO_PARTIALS + 2u; k++) {
        voice->phase[k] += voice->inc[k];
        if (voice->phase[k] >= 1.0f)
            voice->phase[k] -= 1.0f;
        voice->amp[k] *= controls[1] ? 1.0f - (1.0f - voice->decay[k]) * decay_scale : voice->decay[k];
        sum += piano_lookup(voice->phase[k]) * voice->amp[k]
             * (k == 0u || k == PIANO_PARTIALS ? 1.0f : brightness);
    }
    voice->punch *= 0.99995f;
    sum *= 0.55f + 0.45f * voice->punch;
    if (voice->age < 360u) {
        voice->thump += 0.15f * (noise() - voice->thump);
        thump = voice->thump * 1.6f * (1.0f - (float)voice->age * (1.0f / 360.0f)) * voice->level;
        sum += thump;
    }
    remaining = (float)(voice->duration - voice->age);
    if (remaining < 3000.0f)
        sum *= remaining * (1.0f / 3000.0f);
    return sum * level;
}

static float juno_pwm_phase;

/* Juno-style: saw + PWM pulse + sub, slow attack, darkening low-pass. */
static float juno_voice(voice_t *voice, float level, const uint8_t controls[4])
{
    float increment, duty, saw, pulse, sub, x, amp, cutoff, blep, edge;
    if (voice->age >= voice->duration) {
        voice->active = 0u;
        return 0.0f;
    }
    voice->age++;
    amp = (float)voice->age * (1.0f / 5000.0f);
    if (amp > 1.0f)
        amp = 1.0f;
    if (voice->duration - voice->age < 2200u)
        amp *= (float)(voice->duration - voice->age) * (1.0f / 2200.0f);
    increment = voice->increment * (1.0f + 0.004f * (float)controls[3] * juno_pwm_phase);
    voice->phase += increment;
    if (voice->phase >= 1.0f)
        voice->phase -= 1.0f;
    voice->phase2 += voice->increment * 0.5f;
    if (voice->phase2 >= 1.0f)
        voice->phase2 -= 1.0f;
    duty = 0.5f + 0.35f * juno_pwm_phase;
    blep = poly_blep(voice->phase, increment);
    saw = 2.0f * voice->phase - 1.0f - blep;
    edge = voice->phase - duty;
    if (edge < 0.0f)
        edge += 1.0f;
    pulse = (voice->phase < duty ? 1.0f : -1.0f) + blep - poly_blep(edge, increment);
    sub = bandlimited_pulse(voice->phase2, voice->increment * 0.5f, 0.5f);
    x = saw * 0.35f + pulse * 0.3f + sub * 0.4f;
    voice->envelope *= 0.99996f;
    cutoff = 0.026f + 0.09f * voice->envelope + 0.025f * (float)controls[0]
           + 0.02f * (float)controls[2] * voice->envelope;
    /* Zero-delay four-pole ladder like the Juno's VCF. */
    cutoff *= 1.5f;
    if ((voice->age & 15u) == 1u)
        ladder_update(voice, cutoff, 1.1f * (float)controls[1]);
    return ladder_tick(voice, x, 1.1f * (float)controls[1]) * amp * level * voice->velocity_gain;
}

/* Pro One-style: bright detuned saws, short filter and amplitude decay, late vibrato. */
static float pro_voice(voice_t *voice, float level, const uint8_t controls[4])
{
    float increment, sub_increment, a, b, sub, x, amp, cutoff, vibrato, lfo;
    if (voice->age >= voice->duration) {
        voice->active = 0u;
        return 0.0f;
    }
    voice->age++;
    amp = voice->age < 30u ? (float)voice->age * (1.0f / 30.0f) : 1.0f;
    if (voice->duration - voice->age < 600u)
        amp *= (float)(voice->duration - voice->age) * (1.0f / 600.0f);
    lfo = voice->age > 4000u || controls[3]
        ? fm_sinf((float)voice->age * (5.5f / SAMPLE_RATE) * FM_TWO_PI) : 0.0f;
    vibrato = voice->age > 4000u ? 1.0f + 0.0035f * lfo : 1.0f;
    if (controls[3])
        vibrato += 0.004f * (float)controls[3] * lfo;
    increment = voice->increment * vibrato;
    voice->phase += increment;
    if (voice->phase >= 1.0f)
        voice->phase -= 1.0f;
    voice->phase2 += voice->increment * 1.006f * vibrato;
    if (voice->phase2 >= 1.0f)
        voice->phase2 -= 1.0f;
    sub_increment = increment * 0.5f;
    voice->phase3 += sub_increment;
    if (voice->phase3 >= 1.0f)
        voice->phase3 -= 1.0f;
    a = bandlimited_saw(voice->phase, increment);
    b = bandlimited_saw(voice->phase2, increment * 1.006f);
    sub = bandlimited_pulse(voice->phase3, sub_increment, 0.5f);
    x = a * 0.5f + b * 0.5f + sub * 0.12f;
    voice->envelope *= 0.9996f;
    voice->envelope2 *= 0.99975f;
    /* Four-pole low-pass with a lower, narrower sweep: the earlier two-pole filter opened to several kHz and
       boosted its first stage, which made the detuned saws harsh. */
    cutoff = 0.09f + 0.25f * voice->envelope + 0.025f * (float)controls[0]
           + 0.03f * (float)controls[2] * voice->envelope;
    if (cutoff > 0.45f) cutoff = 0.45f;
    if ((voice->age & 15u) == 1u)
        ladder_update(voice, cutoff, 1.1f * (float)controls[1]);
    x = ladder_tick(voice, x, 1.1f * (float)controls[1]);
    amp *= 0.18f + 0.82f * voice->envelope2;
    return app_tanh(x * 1.6f) * amp * level * voice->velocity_gain;
}

/* Bass: dark saw/pulse through a steep four-pole low-pass (harmonics 1-4 even, then a sharp roll-off) with a
   soft attack and a small opening filter bump; no phase jumps between notes. */
static float bass_voice(voice_t *voice, float level)
{
    float x, amp, cutoff;
    if (voice->age >= voice->duration) {
        voice->active = 0u;
        return 0.0f;
    }
    voice->age++;
    amp = voice->age < 1500u ? (float)voice->age * (1.0f / 1500.0f) : 1.0f;
    if (voice->duration - voice->age < 900u)
        amp *= (float)(voice->duration - voice->age) * (1.0f / 900.0f);
    voice->phase += voice->increment;
    if (voice->phase >= 1.0f)
        voice->phase -= 1.0f;
    x = bandlimited_saw(voice->phase, voice->increment) * 0.55f
      + bandlimited_pulse(voice->phase, voice->increment, 0.3f) * 0.45f;
    voice->envelope *= 0.9998f;
    voice->envelope2 *= 0.99998f;
    cutoff = 0.05f + 0.05f * voice->envelope;
    voice->lp1 += cutoff * (x - voice->lp1);
    voice->lp2 += cutoff * (voice->lp1 - voice->lp2);
    voice->lp3 += cutoff * (voice->lp2 - voice->lp3);
    voice->lp4 += cutoff * (voice->lp3 - voice->lp4);
    amp *= 0.7f + 0.3f * voice->envelope2;
    return app_tanh(voice->lp4 * 2.4f) * amp * level * voice->velocity_gain;
}

static int audio_voices_active(void)
{
    uint32_t i;
    for (i = 0; i < JUNO_VOICES; i++)
        if (juno[i].active)
            return 1;
    for (i = 0; i < PIANO_VOICES; i++)
        if (piano[i].active)
            return 1;
    for (i = 0; i < PRO_ONE_VOICES; i++)
        if (pro_one[i].active)
            return 1;
    for (i = 0; i < MONO_VOICES; i++)
        if (bass[i].active)
            return 1;
    for (i = 0; i < GUITAR_VOICES; i++)
        if (guitar[i].active)
            return 1;
    for (i = 0; i < CS50_VOICES; i++)
        if (cs50[i].active)
            return 1;
    for (i = 0; i < CS80_VOICES; i++)
        if (cs80[i].active)
            return 1;
    for (i = 0; i < MONO_VOICES; i++)
        if (bass_sample_voices[i].data || bass_sample_voices[i].adpcm_data)
            return 1;
    for (i = 0; i < GUITAR_VOICES; i++)
        if (guitar_sample_voices[i].data || guitar_sample_voices[i].adpcm_data)
            return 1;
    for (uint32_t i = 0; i < ROSSFLM1_SAMPLE_COUNT; i++)
        if (drum_samples[i].data)
            return 1;
    return 0;
}

static void stop_audio_voices(void)
{
    uint32_t i;
    for (i = 0; i < JUNO_VOICES; i++)
        juno[i].active = 0u;
    for (i = 0; i < PIANO_VOICES; i++)
        piano[i].active = 0u;
    for (i = 0; i < PRO_ONE_VOICES; i++)
        pro_one[i].active = 0u;
    for (i = 0; i < MONO_VOICES; i++) {
        bass[i].active = 0u;
        bass_sample_voices[i].data = 0;
        bass_sample_voices[i].adpcm_data = 0;
    }
    for (i = 0; i < GUITAR_VOICES; i++) {
        guitar[i].active = 0u;
        guitar_sample_voices[i].data = 0;
        guitar_sample_voices[i].adpcm_data = 0;
    }
    for (i = 0; i < CS50_VOICES; i++)
        cs50[i].active = 0u;
    for (i = 0; i < CS80_VOICES; i++)
        cs80[i].active = 0u;
    for (i = 0; i < ROSSFLM1_SAMPLE_COUNT; i++)
        drum_samples[i].data = 0;
    startup_active = 0u;
    chorus_tail = 0u;
    drum_effect_tail = 0u;
    drum_chorus_age = drum_reverb_age = 0u;
    for (i = 0; i < 3u; i++) {
        string_fx[i].tail = 0u;
        string_fx[i].mod_age = string_fx[i].delay_age = 0u;
    }
}

/* Yamaha CS-50-style: saw plus narrow pulse, brassy filter swell that settles to a sustained level. */
static float cs50_voice(voice_t *voice, float level, const uint8_t controls[4])
{
    float increment, x, amp, cutoff;
    if (voice->age >= voice->duration) {
        voice->active = 0u;
        return 0.0f;
    }
    voice->age++;
    amp = voice->age < 700u ? (float)voice->age * (1.0f / 700.0f) : 1.0f;
    if (voice->duration - voice->age < 1400u)
        amp *= (float)(voice->duration - voice->age) * (1.0f / 1400.0f);
    increment = voice->increment;
    if (controls[3])
        increment *= 1.0f + 0.004f * (float)controls[3]
            * fm_sinf((float)voice->age * (5.0f / SAMPLE_RATE) * FM_TWO_PI);
    voice->phase += increment;
    if (voice->phase >= 1.0f)
        voice->phase -= 1.0f;
    voice->phase2 += voice->increment * 1.005f;
    if (voice->phase2 >= 1.0f)
        voice->phase2 -= 1.0f;
    x = bandlimited_saw(voice->phase, increment) * 0.55f
      + bandlimited_pulse(voice->phase2, voice->increment * 1.005f, 0.38f) * 0.4f;
    voice->envelope *= 0.99985f;
    voice->envelope2 *= 0.99992f;
    cutoff = 0.045f + 0.2f * voice->envelope + 0.04f * (float)controls[0]
           + 0.04f * (float)controls[2] * voice->envelope;
    voice->lp1 += cutoff * (x - voice->lp1 - 0.35f * (float)controls[1] * voice->lp2);
    voice->lp2 += cutoff * (voice->lp1 - voice->lp2);
    amp *= 0.55f + 0.45f * voice->envelope2;
    return voice->lp2 * amp * level * voice->velocity_gain;
}

static float cs80_oscillator(float phase, float increment, uint8_t shape, uint8_t pulse_width)
{
    float saw = bandlimited_saw(phase, increment);
    float pulse;
    if (shape == 0u)
        return saw;
    pulse = bandlimited_pulse(phase, increment, (float)pulse_width * (1.0f / 100.0f));
    return shape == 1u ? pulse : saw * 0.52f + pulse * 0.48f;
}

static float cs80_voice(cs80_voice_t *voice, float level, const uint8_t controls[4])
{
    const cs80_patch_t *patch = &cs80_patches[voice->patch - 1u];
    float channel[2], amp, cutoff, resonance, mix;
    uint32_t channel_index;
    uint32_t release_samples = (uint32_t)patch->release_ms * 44u;
    if (voice->age >= voice->duration) {
        voice->active = 0u;
        return 0.0f;
    }
    voice->age++;
    amp = (float)patch->sustain * (1.0f / 127.0f);
    if (patch->attack_ms && voice->age < (uint32_t)patch->attack_ms * 44u)
        amp *= (float)voice->age / ((float)patch->attack_ms * 44.0f);
    if (release_samples && voice->duration - voice->age < release_samples)
        amp *= (float)(voice->duration - voice->age) / (float)release_samples;

    cutoff = (float)patch->cutoff + (float)((int32_t)controls[0] - (int32_t)patch->controls[0]);
    cutoff += (float)patch->filter_env * (1.0f / 127.0f) * voice->filter_envelope * 48.0f;
    if (cutoff < 0.0f)
        cutoff = 0.0f;
    if (cutoff > 180.0f)
        cutoff = 180.0f;
    cutoff = 0.018f + cutoff * 0.004f;
    resonance = (float)patch->resonance + (float)((int32_t)controls[1] - (int32_t)patch->controls[1]);
    if (resonance < 0.0f)
        resonance = 0.0f;
    if (resonance > 127.0f)
        resonance = 127.0f;
    resonance = 2.0f - 1.65f * resonance * (1.0f / 127.0f);
    voice->filter_envelope *= 0.96f + 0.00015f * (float)patch->filter_decay;

    for (channel_index = 0u; channel_index < 2u; channel_index++) {
        float increment = voice->increment[channel_index];
        float input, g, a1, a2, a3, v3, v1, v2;
        voice->phase[channel_index] += increment;
        if (voice->phase[channel_index] >= 1.0f)
            voice->phase[channel_index] -= 1.0f;
        input = cs80_oscillator(voice->phase[channel_index], increment,
                                patch->shape[channel_index], patch->pulse_width[channel_index]);
        g = cutoff;
        a1 = 1.0f / (1.0f + g * (g + resonance));
        a2 = g * a1;
        a3 = g * a2;
        v3 = input - voice->filter_ic2[channel_index];
        v1 = a1 * voice->filter_ic1[channel_index] + a2 * v3;
        v2 = voice->filter_ic2[channel_index] + a2 * voice->filter_ic1[channel_index] + a3 * v3;
        voice->filter_ic1[channel_index] = 2.0f * v1 - voice->filter_ic1[channel_index];
        voice->filter_ic2[channel_index] = 2.0f * v2 - voice->filter_ic2[channel_index];
        channel[channel_index] = v2;
    }
    mix = (float)controls[2] * (1.0f / 127.0f);
    return (channel[0] * (1.0f - mix) + channel[1] * mix)
        * amp * level * ((float)controls[3] * (1.0f / 127.0f))
        * patch->output_trim * voice->velocity_gain;
}

#define KS_SIZE 1024u

typedef struct {
    float line[KS_SIZE];
    uint32_t write;
    float prev;
} ks_string_t;

static ks_string_t guitar_string[GUITAR_VOICES];

/* Guitar: Karplus-Strong plucked string (noise burst in a damped delay loop, the harmonics die away as on a
   real string) through a single-coil pickup comb, a mildly overdriven valve-style amp and a speaker low-pass.
   Plain public-domain textbook algorithm. */
static float guitar_voice(voice_t *voice, float level)
{
    ks_string_t *s = &guitar_string[voice - guitar];
    float period, delay, pos, frac, a, b, y, amp, x;
    uint32_t i, n, idx;
    if (voice->age >= voice->duration) {
        voice->active = 0u;
        return 0.0f;
    }
    period = 1.0f / voice->increment;
    if (period > (float)(KS_SIZE - 4u))
        period = (float)(KS_SIZE - 4u);
    if (voice->age == 0u) {
        float mean = 0.0f, smooth = 0.0f;
        n = (uint32_t)period + 2u;
        for (i = 0; i < KS_SIZE; i++)
            s->line[i] = 0.0f;
        for (i = 0; i < n; i++) {
            smooth += 0.45f * (noise() - smooth);
            s->line[i] = smooth;
            mean += smooth;
        }
        mean /= (float)n;
        for (i = 0; i < n; i++)
            s->line[i] = (s->line[i] - mean) * 1.6f;
        s->write = n;
        s->prev = 0.0f;
    }
    voice->age++;
    amp = voice->age < 40u ? (float)voice->age * (1.0f / 40.0f) : 1.0f;
    if (voice->duration - voice->age < 1500u)
        amp *= (float)(voice->duration - voice->age) * (1.0f / 1500.0f);
    delay = period - 0.5f;
    pos = (float)s->write - delay;
    while (pos < 0.0f)
        pos += (float)KS_SIZE;
    idx = (uint32_t)pos;
    frac = pos - (float)idx;
    a = s->line[idx % KS_SIZE];
    b = s->line[(idx + 1u) % KS_SIZE];
    y = a + frac * (b - a);
    x = 0.5f * (y + s->prev);
    s->prev = y;
    s->line[s->write % KS_SIZE] = x * 0.9991f;
    s->write = (s->write + 1u) % KS_SIZE;
    /* Single-coil pickup: comb from sensing the string a fifth of the way along it */
    pos = (float)s->line[(idx + (uint32_t)(period * 0.8f)) % KS_SIZE];
    x = (y - pos) * 0.9f;
    /* Amp: asymmetric valve-style soft clip, then high-pass, then speaker low-pass */
    x = app_tanh(x * 3.2f + 0.25f) - 0.245f;
    voice->lp3 += 0.012f * (x - voice->lp3);
    x -= voice->lp3;
    voice->lp1 += 0.4f * (x - voice->lp1);
    voice->lp2 += 0.4f * (voice->lp1 - voice->lp2);
    return voice->lp2 * amp * level * voice->velocity_gain * 0.9f;
}

void app_init(void)
{
    requested_play = 0u;
    requested_seq = 0u;
    requested_escape_play = 0u;
    audio_playing = 0u;
    audio_escape_play = 0u;
    settings.magic = SETTINGS_MAGIC;
    settings.tempo = DEFAULT_TEMPO;
    requested_tempo = DEFAULT_TEMPO;
    int loaded = plat_store_load(OBJ_SETTINGS, &settings, sizeof settings);
    if (loaded >= 8 && settings.magic == SETTINGS_MAGIC && settings.tempo >= 70u && settings.tempo <= 140u) {
        requested_tempo = settings.tempo;
        if (loaded < (int)sizeof settings)
            seq_clear();
    } else {
        settings.magic = SETTINGS_MAGIC;
        settings.tempo = DEFAULT_TEMPO;
        seq_clear();
    }
    samples_per_step = 661500.0f / (float)requested_tempo;
    build_piano_table();
    build_tanh_table();
}

void app_boot(void)
{
    startup_requested = 1u;
    startup_splash_until = plat_ms() + STARTUP_MS;
    splash_booted = 1u;
}

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t i, j;
    uint8_t strings[2][4];
    uint8_t voice_settings[TRACKS][4];
    uint8_t cs80_settings[4];
    uint8_t piano_effects[4];
    int32_t tempo = sequence_playing ? sequence_playback_tempo : requested_tempo;
    uint8_t playing = requested_play;
    uint8_t escape_play = requested_escape_play;
    float target_gain = (float)master_q12 * (8300000.0f / 4096.0f);
    float target_drum_bass_gain = (float)drum_bass_boost * 0.125f;
    uint8_t drum_chorus_setting = drum_chorus, drum_reverb_setting = drum_reverb;
    uint8_t drum_compression_setting = drum_compression;
    for (i = 0; i < 2u; i++)
        for (j = 0; j < 4u; j++)
            strings[i][j] = string_controls[i][j];
    for (i = 0; i < TRACKS; i++)
        for (j = 0; j < 4u; j++)
            voice_settings[i][j] = voice_controls[i][j];
    for (j = 0; j < 4u; j++)
        cs80_settings[j] = cs80_controls[j];
    piano_effects[0] = 0u;
    piano_effects[1] = voice_settings[1][2];
    piano_effects[2] = 0u;
    piano_effects[3] = voice_settings[1][3];

    if (startup_requested) {
        startup_requested = 0u;
        startup_active = 1u;
        startup_event = 0u;
        startup_sample = 0u;
    }

    if (playing || requested_seq) {
        if (!audio_playing) {
            audio_playing = 1u;
            step = 15u;
            samples_into_step = 0u;
        }
    } else {
        audio_playing = 0u;
    }
    if (restart_requested) {
        restart_requested = 0u;
        step = 15u;
        samples_into_step = 0u;
#ifdef HAVE_SONG
        if (escape_play) {
            song_tick = 0u;
            song_index = 0u;
            samples_into_tick = 0.0f;
        }
#endif
    }
    if (all_notes_off_requested) {
        all_notes_off_requested = 0u;
        stop_audio_voices();
    }
    if (escape_play != audio_escape_play) {
        audio_escape_play = escape_play;
        stop_audio_voices();
#ifdef HAVE_SONG
        if (escape_play) {
            song_tick = 0u;
            song_index = 0u;
            samples_into_tick = 0.0f;
        }
#endif
    }
    samples_per_step = 661500.0f / (float)tempo;
    if (!playing && !requested_seq && !sequence_playing && !sequence_stop_requested
        && !escape_play && !startup_active && !chorus_tail && !drum_effect_tail
        && !string_fx[0].tail && !string_fx[1].tail && !string_fx[2].tail && !audio_voices_active()) {
        for (i = 0; i < n; i++)
            lr[2u * i] = lr[2u * i + 1u] = 0;
        return;
    }

    for (i = 0; i < n; i++) {
        float mix = 0.0f;
        float juno_mix = 0.0f;
        float cs_mix = 0.0f;
        float guitar_mix = 0.0f, bass_mix = 0.0f, piano_mix = 0.0f;
        float chorus_l, chorus_r, chorus_mod;
        int32_t left, right;
        uint8_t voices_active = 0u;
        if (sequence_playing || sequence_stop_requested)
            sequence_audio_advance();
        if (startup_active) {
            while (startup_event < STARTUP_EVENTS
                   && startup_sample >= startup_events[startup_event].offset) {
                const startup_event_t *e = &startup_events[startup_event];
                if (e->pro) {
                    uint32_t index = trigger_pooled(pro_one, PRO_ONE_VOICES, e->note, e->duration);
                    pro_one[index].velocity_gain = velocity_gain(e->velocity);
                } else {
                    trigger_pooled(cs50, CS50_VOICES, e->note, e->duration);
                }
                startup_event++;
            }
            if (startup_event == STARTUP_EVENTS && startup_sample >= startup_end)
                startup_active = 0u;
            startup_sample++;
        }
#ifdef HAVE_SONG
        if (escape_play)
            song_advance(2646000.0f / ((float)tempo * (float)SONG_PPQ));
        else
#endif
        if (playing || requested_seq) {
            if ((float)samples_into_step >= samples_per_step) {
                samples_into_step = 0u;
                step = (step + 1u) & 15u;
                if (playing)
                    trigger_step(step);
                if (requested_seq)
                    seq_trigger(step);
            }
            samples_into_step++;
        }

        for (j = 0; j < JUNO_VOICES; j++)
            if (juno[j].active) {
                juno_pwm_phase = fm_sinf(chorus_phase * 9.0f * FM_TWO_PI);
                break;
            }
        if (j == JUNO_VOICES)
            juno_pwm_phase = 0.0f;
        for (j = 0; j < JUNO_VOICES; j++) {
            if (juno[j].active)
                juno_mix += juno_voice(&juno[j], 0.624f, voice_settings[0]);
            voices_active |= juno[j].active;
        }
        juno_mix *= (solo_mask & 1u) ? 1.0f : 0.0f;
        for (j = 0; j < PIANO_VOICES; j++) {
            if (piano[j].active)
                piano_mix += piano_voice(&piano[j], 0.3f, voice_settings[1]);
            voices_active |= piano[j].active;
        }
        if (solo_mask & 2u) {
            mix += string_effects(2u, piano_mix, piano_effects, 1);
            voices_active |= string_fx[2].tail != 0u;
        } else {
            string_fx[2].tail = string_fx[2].mod_age = string_fx[2].delay_age = 0u;
        }
        for (j = 0; j < PRO_ONE_VOICES; j++) {
            if (pro_one[j].active)
                mix += pro_voice(&pro_one[j], 0.9f, voice_settings[2]) * ((solo_mask & 4u) ? 1.0f : 0.0f);
            voices_active |= pro_one[j].active;
        }
        for (j = 0; j < GUITAR_VOICES; j++) {
            if (guitar[j].active)
                guitar_mix += guitar_voice(&guitar[j], 0.34f);
            voices_active |= guitar[j].active;
            if (guitar_sample_voices[j].data || guitar_sample_voices[j].adpcm_data)
                guitar_mix += sample_voice(&guitar_sample_voices[j], 0.7f);
            voices_active |= guitar_sample_voices[j].data != 0 || guitar_sample_voices[j].adpcm_data != 0;
        }
        for (j = 0; j < MONO_VOICES; j++) {
            if (bass[j].active)
                bass_mix += bass_voice(&bass[j], 0.7f);
            voices_active |= bass[j].active;
            if (bass_sample_voices[j].data || bass_sample_voices[j].adpcm_data)
                bass_mix += sample_voice(&bass_sample_voices[j], 0.7f);
            voices_active |= bass_sample_voices[j].data != 0 || bass_sample_voices[j].adpcm_data != 0;
        }
        if (solo_mask & 8u) {
            mix += string_effects(0u, guitar_mix, strings[0], 0);
            voices_active |= string_fx[0].tail != 0u;
        } else {
            string_fx[0].tail = string_fx[0].mod_age = string_fx[0].delay_age = 0u;
        }
        if (solo_mask & 16u) {
            mix += string_effects(1u, bass_mix, strings[1], 0);
            voices_active |= string_fx[1].tail != 0u;
        } else {
            string_fx[1].tail = string_fx[1].mod_age = string_fx[1].delay_age = 0u;
        }
        for (j = 0; j < CS50_VOICES; j++) {
            if (cs50[j].active)
                cs_mix += cs50_voice(&cs50[j], 0.3f, voice_settings[6]);
            voices_active |= cs50[j].active;
        }
        for (j = 0; j < CS80_VOICES; j++) {
            if (cs80[j].active)
                cs_mix += cs80_voice(&cs80[j], 0.3f, cs80_settings);
            voices_active |= cs80[j].active;
        }
        cs_mix *= (solo_mask & 64u) ? 1.0f : 0.0f;
        mix += cs_mix;

        if (solo_mask & 32u) {
            float drums = 0.0f;
            for (j = 0; j < ROSSFLM1_SAMPLE_COUNT; j++) {
                drums += sample_voice(&drum_samples[j], drum_sample_levels[j]);
                voices_active |= drum_samples[j].data != 0;
            }
            drum_bass_low += 0.0183f * (drums - drum_bass_low);
            drum_bass_gain += (target_drum_bass_gain - drum_bass_gain) * 0.004f;
            drums = drum_effects(drums, drum_chorus_setting, drum_reverb_setting, drum_compression_setting);
            mix += drums + drum_bass_low * drum_bass_gain;
            voices_active |= drum_effect_tail != 0;
        } else {
            for (j = 0; j < ROSSFLM1_SAMPLE_COUNT; j++)
                sample_voice(&drum_samples[j], 0.0f);
            drum_effect_tail = 0u;
            drum_chorus_age = drum_reverb_age = 0u;
        }
        if (juno_mix != 0.0f || cs_mix != 0.0f)
            chorus_tail = 1024u;
        chorus_l = chorus_r = 0.0f;
        if (chorus_tail) {
            chorus_delay[chorus_position] = juno_mix + cs_mix * 0.6f;
            chorus_mod = fm_sinf(chorus_phase * FM_TWO_PI);
            chorus_l = chorus_tap(300.0f + chorus_mod * 105.0f);
            chorus_r = chorus_tap(300.0f - chorus_mod * 105.0f);
            chorus_position = (chorus_position + 1u) & 1023u;
            chorus_phase += 0.18f / SAMPLE_RATE;
            if (chorus_phase >= 1.0f)
                chorus_phase -= 1.0f;
            chorus_tail--;
            voices_active = 1u;
        }
        master_gain += (target_gain - master_gain) * 0.004f;
        if (voices_active) {
#ifdef PLAT_HOST
            if (sequence_playing && sequence_audio_events) {
                float l = mix + juno_mix * 0.72f + chorus_l * 0.24f;
                float r = mix + juno_mix * 0.72f + chorus_r * 0.24f;
                float peak = l < 0.0f ? -l : l;
                float right_peak = r < 0.0f ? -r : r;
                if (right_peak > peak)
                    peak = right_peak;
                if (peak > sequence_mix_peak) {
                    sequence_mix_peak = peak;
                    sequence_mix_peak_step = sequence_audio_step;
                }
                if (peak >= 1.0f)
                    sequence_mix_over_one++;
            }
#endif
            left = (int32_t)(app_tanh(mix + juno_mix * 0.72f + chorus_l * 0.24f) * master_gain);
            right = (int32_t)(app_tanh(mix + juno_mix * 0.72f + chorus_r * 0.24f) * master_gain);
        } else {
            left = right = 0;
        }
        lr[2u * i] = left;
        lr[2u * i + 1u] = right;
        if (left != right)
            stereo_diff_samples++;
    }
}

void app_panic(void)
{
    uint32_t i;
    requested_play = 0u;
    audio_playing = 0u;
    startup_requested = 0u;
    startup_active = 0u;
    drum_bass_low = 0.0f;
    drum_bass_gain = 0.0f;
    drum_chorus_pos = drum_reverb_pos = drum_chorus_age = drum_reverb_age = drum_effect_tail = 0u;
    drum_chorus_mode = drum_reverb_mode = 0u;
    drum_chorus_phase = drum_compressor_envelope = 0.0f;
    for (i = 0; i < 3u; i++) {
        string_fx[i].mod_pos = string_fx[i].delay_pos = 0u;
        string_fx[i].mod_age = string_fx[i].delay_age = string_fx[i].tail = 0u;
        string_fx[i].mod_mode = string_fx[i].delay_mode = 0u;
        string_fx[i].phase = string_fx[i].allpass = 0.0f;
    }
    for (i = 0; i < JUNO_VOICES; i++)
        juno[i].active = 0u;
    for (i = 0; i < PIANO_VOICES; i++) {
        piano[i].active = 0u;
        piano[i].key = 0u;
    }
    for (i = 0; i < PRO_ONE_VOICES; i++)
        pro_one[i].active = 0u;
    for (i = 0; i < MONO_VOICES; i++) {
        bass[i].active = 0u;
        bass_sample_voices[i].data = 0;
        bass_sample_voices[i].adpcm_data = 0;
    }
    for (i = 0; i < GUITAR_VOICES; i++) {
        guitar[i].active = 0u;
        guitar_sample_voices[i].data = 0;
        guitar_sample_voices[i].adpcm_data = 0;
    }
    for (i = 0; i < CS50_VOICES; i++)
        cs50[i].active = 0u;
    for (i = 0; i < CS80_VOICES; i++) {
        cs80[i].active = 0u;
        cs80[i].key = 0u;
    }
    for (i = 0; i < ROSSFLM1_SAMPLE_COUNT; i++)
        drum_samples[i].data = 0;
    chorus_position = 0u;
    chorus_tail = 0u;
    chorus_phase = 0.0f;
    for (i = 0; i < 1024u; i++)
        chorus_delay[i] = 0.0f;
    master_gain = 0.0f;
}

void app_ui_init(void)
{
    uint32_t i;
#ifdef APP_TOUCH
    touch_on(1);
#endif
    previous_keys = 0u;
    drum_sample_event = 0u;
    ui_drum_sample_event = 0u;
    ui_drum_sample_visible = 0u;
    knob_image_visible = 0u;
    knob_image_frame = 0u;
    knob_image_color = 0u;
    portrait_color = 0u;
    portrait_color_visible = 0u;
    ui_portrait_color = 0u;
    ui_portrait_color_visible = 0u;
    portrait_master_last = plat_master();
    portrait_master_accumulator = 0;
    ui_knob_image_visible = 0u;
    ui_knob_image_frame = 0u;
    ui_knob_image_color = 0u;
    selected_instrument = 1u;
    chord_voice = 0u;
    chord_layout = 0u;
    ui_chord_layout = 0u;
    phrase_count = phrase_state = phrase_next = ui_phrase_state = 0u;
    phrase_length = 0u;
    phrase_down = phrase_held = 0u;
    cs80_preset = 0u;
    ui_cs80_preset = 0u;
    for (i = 0; i < 4u; i++)
        cs80_controls[i] = ui_cs80_controls[i] = 0u;
    drum_bass_boost = 0u;
    bass_sound = BASS_ELECTRIC_VARIANT;
    bass_one_shot = 0u;
    guitar_sound = 1u;
    guitar_one_shot = 0u;
    drum_chorus = drum_reverb = drum_compression = 0u;
    for (i = 0; i < 2u; i++)
        for (uint32_t k = 0; k < 4u; k++)
            string_controls[i][k] = 0u;
#ifdef PLAT_HOST
    last_live_increment = 0.0f;
#endif
    for (i = 0; i < TRACKS; i++) {
        instrument_octave[i] = 0;
        for (uint32_t k = 0; k < 4u; k++)
            voice_controls[i][k] = 0u;
    }
    for (i = 0; i < NKEYS; i++) {
        live_key_voice[i] = -1;
        live_key_instrument[i] = selected_instrument;
        live_key_chord_voice[i] = 0u;
        live_key_cs80[i] = 0u;
    }
    ui_escape_play = 0u;
    ui_drawn = 0u;
    sequence_screen = SCREEN_MAIN;
    sequence_playing = 0u;
    sequence_stop_requested = 0u;
    sequence_audio_started = 0u;
    sequence_audio_from_saved = 0u;
    sequence_audio_stealing = 0u;
    sequence_audio_steal_limit = SEQUENCE_VOICES;
    sequence_audio_events = 0;
    sequence_audio_event_count = sequence_audio_event_index = 0u;
    sequence_escape_cpu_peak = sequence_escape_xruns_start = 0u;
    sequence_audio_start = sequence_audio_step = sequence_audio_length = 0u;
    sequence_audio_samples = 0.0f;
    sequence_cursor_track = 0u;
    sequence_cursor_step = 0u;
    sequence_duration_index = 0u;
    sequence_selected_sound = 0u;
    sequence_warning = 0u;
    sequence_editing_saved = 0u;
    sequence_editing_slot = 0u;
    sequence_builtin_seed_failed = 0u;
    sequence_notice = SEQUENCE_NOTICE_NONE;
    sequence_help_page = 0u;
    sequence_last_note = 60u;
    sequence_last_note_valid = 0u;
    sequence_reset();
    sequence_seed_builtins();
    sequence_reset();
    sequence_scan_slots();
}

static void splash_decode(void)
{
    uint32_t i, out = 0u, run;

    cv_begin(STARTUP_SPLASH_WIDTH, STARTUP_SPLASH_HEIGHT, C_BLACK);
    for (i = 0u; i < STARTUP_SPLASH_RLE_SIZE; i += 2u) {
        uint16_t color = swap16(startup_splash_palette[startup_splash_rle[i + 1u]]);
        for (run = 0u; run < startup_splash_rle[i]; run++)
            cv_px[out++] = color;
    }
}

static void splash_blit(void)
{
    cv_blit(0, STARTUP_SPLASH_Y);
}

static void splash_clear_margins(void)
{
    lcd_fill(0, 0, STARTUP_SPLASH_WIDTH, STARTUP_SPLASH_Y, C_BLACK);
    lcd_fill(0, STARTUP_SPLASH_Y + STARTUP_SPLASH_HEIGHT, STARTUP_SPLASH_WIDTH,
             STARTUP_SCREEN_HEIGHT - STARTUP_SPLASH_Y - STARTUP_SPLASH_HEIGHT, C_BLACK);
}

void app_splash(void)
{
    if (!splash_booted) {
        /* the boot screen is drawn by the sweep in app_frame */
        lcd_fill(0, 0, 240u, STARTUP_SCREEN_HEIGHT, C_BLACK);
        return;
    }
    splash_decode();
    splash_clear_margins();
    splash_blit();
}

/* Scanline sweep: a cyan line reveals the logo top to bottom, with a few
   glitched rows trailing it. Returns after drawing; the last frame is clean. */
static void splash_sweep(uint32_t elapsed)
{
    uint32_t frame = elapsed >= SPLASH_SWEEP_MS ? 0xFFFEu : elapsed / SPLASH_SWEEP_STEP_MS;
    uint32_t y, row, x;

    if (frame == splash_sweep_frame)
        return;
    splash_sweep_frame = frame;
    if (frame == 0xFFFEu) {
        app_splash();
        return;
    }
    splash_decode();
    y = (STARTUP_SPLASH_HEIGHT * elapsed) / SPLASH_SWEEP_MS;
    for (row = (y > 30u ? y - 30u : 0u); row < y; row++) {
        uint32_t r = (row * 2654435761u) ^ (frame * 40503u);
        uint16_t *p = &cv_px[row * STARTUP_SPLASH_WIDTH];
        int32_t shift;

        r ^= r >> 13;
        if ((r & 7u) != 0u)
            continue;
        shift = (int32_t)((r >> 4) % 21u) - 10;
        if (shift > 0) {
            for (x = STARTUP_SPLASH_WIDTH - 1u; x >= (uint32_t)shift; x--)
                p[x] = p[x - (uint32_t)shift];
            for (x = 0u; x < (uint32_t)shift; x++)
                p[x] = C_BLACK;
        } else if (shift < 0) {
            uint32_t s = (uint32_t)(-shift);
            for (x = 0u; x + s < STARTUP_SPLASH_WIDTH; x++)
                p[x] = p[x + s];
            for (; x < STARTUP_SPLASH_WIDTH; x++)
                p[x] = C_BLACK;
        }
    }
    for (x = y * STARTUP_SPLASH_WIDTH; x < STARTUP_SPLASH_WIDTH * STARTUP_SPLASH_HEIGHT; x++)
        cv_px[x] = (x < (y + 2u) * STARTUP_SPLASH_WIDTH) ? swap16(0x07FFu) : C_BLACK;
    splash_blit();
}

static uint32_t phrase_popcount(uint32_t bits)
{
    uint32_t n = 0u;
    for (; bits; bits &= bits - 1u)
        n++;
    return n;
}

static void phrase_release_all(void)
{
    for (uint32_t k = 0; k < NKEYS; k++)
        if (phrase_held & (1u << k))
            release_live_key(k);
    phrase_held = 0u;
}

static void phrase_abort(void)
{
    phrase_release_all();
    if (phrase_state == PHRASE_REC) {
        phrase_count = 0u;
        phrase_length = 0u;
    }
    phrase_state = PHRASE_IDLE;
    phrase_down = 0u;
}

static void phrase_record(uint32_t key, uint32_t velocity, int down)
{
    uint32_t elapsed = plat_ms() - phrase_start;
    phrase_event_t *event;
    if (phrase_state != PHRASE_REC || elapsed >= PHRASE_MAX_MS)
        return;
    if (down) {
        /* Keep room to close every key that is still held when recording stops. */
        if (phrase_count + phrase_popcount(phrase_down) + 2u > PHRASE_EVENTS)
            return;
        phrase_down |= 1u << key;
    } else {
        if (!(phrase_down & (1u << key)))
            return;
        phrase_down &= ~(1u << key);
    }
    event = &phrase[phrase_count++];
    event->at = (uint16_t)elapsed;
    event->key = (uint8_t)(key | (down ? 0u : PHRASE_UP));
    event->velocity = (uint8_t)velocity;
    event->instrument = selected_instrument;
    event->octave = instrument_octave[selected_instrument];
}

static void phrase_finish_recording(void)
{
    uint32_t elapsed = plat_ms() - phrase_start;
    uint16_t length = (uint16_t)(elapsed > PHRASE_MAX_MS ? PHRASE_MAX_MS : elapsed);
    if (!phrase_count || length < PHRASE_MIN_MS) {
        phrase_count = 0u;
        phrase_length = 0u;
        phrase_state = PHRASE_IDLE;
        phrase_down = 0u;
        return;
    }
    for (uint32_t k = 0; k < NKEYS; k++) {
        if (phrase_down & (1u << k)) {
            uint8_t source = 0u;
            for (uint32_t i = phrase_count; i-- > 0u;)
                if ((phrase[i].key & ~PHRASE_UP) == k) {
                    source = (uint8_t)i;
                    break;
                }
            phrase[phrase_count] = phrase[source];
            phrase[phrase_count].at = (uint16_t)(length - 1u);
            phrase[phrase_count].key = (uint8_t)(k | PHRASE_UP);
            phrase_count++;
        }
    }
    phrase_down = 0u;
    phrase_length = length;
    phrase_state = PHRASE_PLAY;
    phrase_start = plat_ms();
    phrase_next = 0u;
}

static void phrase_play_event(const phrase_event_t *event)
{
    uint32_t key = event->key & ~PHRASE_UP;
    uint32_t bit = 1u << key;
    uint8_t saved_instrument = selected_instrument;
    int8_t saved_octave = instrument_octave[event->instrument];
    if (event->key & PHRASE_UP) {
        if (phrase_held & bit) {
            release_live_key(key);
            phrase_held &= ~bit;
        }
        return;
    }
    if ((previous_keys & bit) || (phrase_held & bit))
        return;
    selected_instrument = event->instrument;
    instrument_octave[event->instrument] = event->octave;
    trigger_live_key(key, event->velocity);
    selected_instrument = saved_instrument;
    instrument_octave[event->instrument] = saved_octave;
    phrase_held |= bit;
}

static void phrase_update(void)
{
    uint32_t now = plat_ms();
    uint32_t t;
    if (phrase_state == PHRASE_REC) {
        if (now - phrase_start >= PHRASE_MAX_MS)
            phrase_finish_recording();
        return;
    }
    if (phrase_state != PHRASE_PLAY)
        return;
    t = now - phrase_start;
    if (t >= phrase_length) {
        phrase_release_all();
        phrase_start += phrase_length;
        t = now - phrase_start;
        if (t >= phrase_length) {
            phrase_start = now;
            t = 0u;
        }
        phrase_next = 0u;
    }
    while (phrase_next < phrase_count && phrase[phrase_next].at <= t)
        phrase_play_event(&phrase[phrase_next++]);
}

static void phrase_button(int edit_held)
{
    if (edit_held) {
        phrase_abort();
        phrase_count = 0u;
        phrase_length = 0u;
    } else if (phrase_state == PHRASE_IDLE && phrase_count) {
        phrase_start = plat_ms();
        phrase_next = 0u;
        phrase_state = PHRASE_PLAY;
    } else if (phrase_state == PHRASE_IDLE) {
        phrase_count = 0u;
        phrase_down = 0u;
        phrase_start = plat_ms();
        phrase_state = PHRASE_REC;
    } else if (phrase_state == PHRASE_REC) {
        phrase_finish_recording();
    } else {
        phrase_abort();
    }
}

static uint8_t knob_setting_value(uint32_t knob);
static const char *knob_setting_label(uint32_t knob);
static void record_knob_turn(uint8_t knob, int32_t delta);
static void record_portrait_turn(int32_t delta);

static void set_cs80_preset(int32_t preset)
{
    preset %= CS80_PATCHES + 1;
    if (preset < 0)
        preset += CS80_PATCHES + 1;
    cs80_preset = (uint8_t)preset;
    if (preset) {
        for (uint32_t k = 0; k < 4u; k++)
            cs80_controls[k] = cs80_patches[preset - 1].controls[k];
    }
}

static void arp_stop(void)
{
    if (arp_key >= 0)
        release_live_key((uint32_t)arp_key);
    arp_key = -1;
    arp_held = 0u;
}

static void arp_update(void)
{
    uint32_t now = plat_ms();
    if (!arp_on || !arp_held || (int32_t)(now - arp_next_ms) < 0)
        return;
    arp_next_ms = now + 15000u / (uint32_t)requested_tempo;
    if (arp_key >= 0)
        release_live_key((uint32_t)arp_key);
    for (uint32_t n = 1u; n <= NKEYS; n++) {
        uint32_t key = (uint32_t)(arp_key + (int32_t)n) % NKEYS;
        if (arp_held & (1u << key)) {
            arp_key = (int32_t)key;
            trigger_live_key(key, 100u);
            return;
        }
    }
}

static void all_notes_off(void)
{
    arp_stop();
    phrase_abort();
    for (uint32_t k = 0; k < NKEYS; k++)
        release_live_key(k);
    requested_play = 0u;
    requested_escape_play = 0u;
    all_notes_off_requested = 1u;
}

static volatile uint8_t *effect_value(uint8_t *limit_out)
{
    volatile uint8_t *value = 0;
    uint8_t limit = 3u;
    switch (selected_instrument) {
    case 0u: case 1u: case 2u: value = &voice_controls[selected_instrument][1]; break;
    case 3u: case 4u: value = &string_controls[selected_instrument - 3u][1]; break;
    case 5u: value = &drum_reverb; break;
    case 6u:
        value = cs80_preset ? &cs80_controls[1] : &voice_controls[6][1];
        limit = cs80_preset ? 127u : 3u;
        break;
    default: value = &chord_controls[1]; break;
    }
    *limit_out = limit;
    return value;
}

static void cycle_effect(void)
{
    uint8_t limit;
    volatile uint8_t *value = effect_value(&limit);
    if (limit == 127u)
        *value = (uint8_t)((*value + 32u) & 127u);
    else
        *value = (uint8_t)((*value + 1u) & 3u);
    record_knob_turn(1u, 1);
}

static uint8_t guitar_mode_index(void)
{
    return (uint8_t)(guitar_sound * 2u + (guitar_one_shot ? 1u : 0u));
}

static const uint8_t bass_mode_sounds[BASS_TIMBRE_COUNT] = {
    0u, BASS_ELECTRIC_VARIANT, BASS_ACOUSTIC_VARIANT, 2u, 1u
};

static uint8_t bass_mode_index(void)
{
    for (uint32_t i = 0u; i < BASS_TIMBRE_COUNT; i++)
        if (bass_sound == bass_mode_sounds[i])
            return (uint8_t)(i * 2u + (bass_one_shot ? 1u : 0u));
    return 0u;
}

static void set_bass_mode(int32_t mode)
{
    mode %= (int32_t)BASS_MODE_COUNT;
    if (mode < 0)
        mode += (int32_t)BASS_MODE_COUNT;
    bass_sound = bass_mode_sounds[(uint32_t)mode / 2u];
    bass_one_shot = (uint8_t)(mode & 1);
}

static void step_bass_mode(int32_t delta)
{
    set_bass_mode((int32_t)bass_mode_index() + delta);
}

static void set_guitar_mode(int32_t mode)
{
    mode %= 8;
    if (mode < 0)
        mode += 8;
    guitar_sound = (uint8_t)(mode / 2);
    guitar_one_shot = (uint8_t)(mode & 1);
}

static void step_guitar_mode(int32_t delta)
{
    set_guitar_mode((int32_t)guitar_mode_index() + delta);
}

static void cycle_variant(void)
{
    if (selected_instrument == 3u)
        step_guitar_mode(1);
    else if (selected_instrument == 4u)
        step_bass_mode(1);
    else if (selected_instrument == 6u)
        set_cs80_preset((int32_t)cs80_preset + 1);
    else if (selected_instrument == 7u)
        chord_voice = (uint8_t)((chord_voice + 1u) % 5u);
}

static uint8_t sequence_sound_instrument(uint8_t sound)
{
    if (sound < 3u)
        return sound;
    if (sound < 7u)
        return 3u;
    if (sound < 12u)
        return 4u;
    if (sound < 25u)
        return 5u;
    if (sound < 36u)
        return 6u;
    return 7u;
}

static uint8_t sequence_sound_variant(uint8_t sound)
{
    if (sound < 3u)
        return 0u;
    if (sound < 7u)
        return (uint8_t)(sound - 3u);
    if (sound < 12u)
        return bass_mode_sounds[sound - 7u];
    if (sound < 25u)
        return (uint8_t)(sound - 12u);
    if (sound < 36u)
        return (uint8_t)(sound - 25u);
    return (uint8_t)(sound - 36u);
}

static uint8_t sequence_sound_index(uint8_t instrument, uint8_t variant)
{
    uint32_t i;
    if (instrument < 3u)
        return instrument;
    if (instrument == 3u)
        return (uint8_t)(3u + (variant < 4u ? variant : 0u));
    if (instrument == 4u) {
        for (i = 0u; i < BASS_TIMBRE_COUNT; i++)
            if (variant == bass_mode_sounds[i])
                return (uint8_t)(7u + i);
        return 7u;
    }
    if (instrument == 5u)
        return (uint8_t)(12u + (variant < ROSSFLM1_SAMPLE_COUNT ? variant : 0u));
    if (instrument == 6u)
        return (uint8_t)(25u + (variant <= CS80_PATCHES ? variant : 0u));
    return (uint8_t)(36u + (variant < 5u ? variant : 0u));
}

static void sequence_apply_sound(uint8_t sound)
{
    uint8_t instrument, variant;
    sound %= SEQUENCE_SOUND_COUNT;
    instrument = sequence_sound_instrument(sound);
    variant = sequence_sound_variant(sound);
    sequence_selected_sound = sound;
    selected_instrument = instrument;
    if (instrument == 3u)
        guitar_sound = variant;
    else if (instrument == 4u)
        bass_sound = variant;
    else if (instrument == 6u)
        set_cs80_preset(variant);
    else if (instrument == 7u)
        chord_voice = variant;
}

static uint8_t sequence_current_variant(void)
{
    if (selected_instrument == 3u)
        return guitar_sound;
    if (selected_instrument == 4u)
        return bass_sound;
    if (selected_instrument == 5u)
        return sequence_sound_variant(sequence_selected_sound);
    if (selected_instrument == 6u)
        return cs80_preset;
    if (selected_instrument == 7u)
        return chord_voice;
    return 0u;
}

static void sequence_store_track_sound(void)
{
    if (sequence_cursor_track == SEQUENCE_DRUM_TRACK)
        sequence.header.drum_variant = sequence_current_variant();
    else {
        sequence.header.instrument[sequence_cursor_track] = selected_instrument;
        sequence.header.variant[sequence_cursor_track] = sequence_current_variant();
    }
}

static void sequence_load_cursor_sound(void)
{
    int32_t event_index;
    if (sequence_cursor_track == SEQUENCE_DRUM_TRACK)
        sequence_apply_sound((uint8_t)(12u + sequence.header.drum_variant));
    event_index = sequence_find_event(sequence_cursor_track, sequence_cursor_step);
    if (event_index >= 0) {
        uint32_t event = sequence.events[event_index];
        sequence_apply_sound(sequence_sound_index(sequence_event_instrument(event),
                                                   sequence_event_variant(event)));
        if (sequence_cursor_track != SEQUENCE_DRUM_TRACK)
            sequence_duration_index = sequence_event_duration(event);
    } else if (sequence_cursor_track != SEQUENCE_DRUM_TRACK) {
        sequence_apply_sound(sequence_sound_index(sequence.header.instrument[sequence_cursor_track],
                                                   sequence.header.variant[sequence_cursor_track]));
    }
}

static void sequence_show_warning(uint8_t warning)
{
    sequence_warning = warning;
    sequence_warning_until = plat_ms() + 1800u;
    ui_drawn = 0u;
}

static void sequence_move_cursor(int32_t delta)
{
    int64_t next = (int64_t)sequence_cursor_step + delta;
    if (next < 0)
        next = 0;
    if (next >= (int64_t)SEQUENCE_MAX_STEPS)
        next = (int64_t)SEQUENCE_MAX_STEPS - 1;
    sequence_cursor_step = (uint16_t)next;
    sequence_extend_to((uint32_t)sequence_cursor_step + 1u);
    sequence_load_cursor_sound();
}

static void sequence_change_sound(int32_t delta)
{
    int32_t sound_delta;
    int32_t event_index;
    if (!delta)
        return;
    if (sequence_cursor_track == SEQUENCE_DRUM_TRACK) {
        sound_delta = ((int32_t)sequence.header.drum_variant + delta % (int32_t)ROSSFLM1_SAMPLE_COUNT)
            % (int32_t)ROSSFLM1_SAMPLE_COUNT;
        if (sound_delta < 0)
            sound_delta += (int32_t)ROSSFLM1_SAMPLE_COUNT;
        sequence_apply_sound((uint8_t)(12 + sound_delta));
        sequence_store_track_sound();
        ui_drawn = 0u;
        return;
    }
    if (sequence_selected_sound >= 12u && sequence_selected_sound < 25u)
        sound_delta = delta > 0 ? 12 : 11;
    else
        sound_delta = sequence_selected_sound < 12u
            ? sequence_selected_sound : sequence_selected_sound - 13;
    if (sequence_selected_sound >= 12u && sequence_selected_sound < 25u)
        delta += delta > 0 ? -1 : 1;
    sound_delta = (sound_delta + delta % (SEQUENCE_SOUND_COUNT - (int32_t)ROSSFLM1_SAMPLE_COUNT))
        % (SEQUENCE_SOUND_COUNT - (int32_t)ROSSFLM1_SAMPLE_COUNT);
    if (sound_delta < 0)
        sound_delta += SEQUENCE_SOUND_COUNT - (int32_t)ROSSFLM1_SAMPLE_COUNT;
    if (sound_delta >= 12)
        sound_delta += (int32_t)ROSSFLM1_SAMPLE_COUNT;
    event_index = sequence_find_event(sequence_cursor_track, sequence_cursor_step);
    if (event_index >= 0
        && !sequence_polyphony_ok(sequence_cursor_step,
                                  sequence_note_steps[sequence_event_duration(sequence.events[event_index])],
                                  event_index, sequence_sound_instrument((uint8_t)sound_delta))) {
        sequence_show_warning(1u);
        return;
    }
    sequence_apply_sound((uint8_t)sound_delta);
    if (event_index >= 0) {
        uint32_t old = sequence.events[event_index];
        uint8_t note = sequence_event_note(old);
        if (selected_instrument == 5u)
            note = sequence_current_variant();
        else if (sequence_event_instrument(old) == 5u)
            note = sequence_last_note_valid ? sequence_last_note : 60u;
        sequence.events[event_index] = sequence_event_make(
            sequence_event_step(old), note, sequence_event_duration(old),
            sequence_cursor_track, selected_instrument, sequence_current_variant());
    }
    sequence_store_track_sound();
}

static void sequence_change_duration(uint8_t duration)
{
    int32_t event_index;
    if (duration >= 7u || sequence_cursor_track == SEQUENCE_DRUM_TRACK)
        return;
    event_index = sequence_find_event(sequence_cursor_track, sequence_cursor_step);
    if (event_index < 0) {
        sequence_duration_index = duration;
        return;
    }
    if (!sequence_polyphony_ok(sequence_cursor_step, sequence_note_steps[duration], event_index,
                               sequence_event_instrument(sequence.events[event_index]))) {
        sequence_show_warning(1u);
        return;
    }
    {
        uint32_t old = sequence.events[event_index];
        sequence.events[event_index] = sequence_event_make(
            sequence_event_step(old), sequence_event_note(old), duration,
            sequence_cursor_track, sequence_event_instrument(old), sequence_event_variant(old));
        sequence_duration_index = duration;
        sequence_extend_to(sequence_event_step(old) + sequence_note_steps[duration]);
    }
}

static void sequence_remove_event(uint32_t index)
{
    uint32_t lock;
    if (index >= sequence.header.count)
        return;
    lock = plat_lock();
    for (uint32_t i = index + 1u; i < sequence.header.count; i++)
        sequence.events[i - 1u] = sequence.events[i];
    sequence.header.count--;
    plat_unlock(lock);
    ui_drawn = 0u;
}

static int sequence_insert_note(uint8_t note)
{
    int32_t event_index = sequence_find_event(sequence_cursor_track, sequence_cursor_step);
    uint8_t duration = sequence_cursor_track == SEQUENCE_DRUM_TRACK ? SEQUENCE_DRUM_NATURAL
        : event_index >= 0
        ? sequence_event_duration(sequence.events[event_index]) : sequence_duration_index;
    uint8_t variant = sequence_current_variant();
    uint32_t event, lock;
    if (event_index < 0 && sequence.header.count >= SEQUENCE_EVENT_MAX) {
        sequence_show_warning(2u);
        return 0;
    }
    event = sequence_event_make(sequence_cursor_step, note, duration, sequence_cursor_track,
                                selected_instrument, variant);
    if (sequence_cursor_step + sequence_event_length_steps(event, sequence.header.tempo)
        > 128u * sequence_steps_per_bar[sequence.header.meter]) {
        sequence_show_warning(2u);
        return 0;
    }
    if (!sequence_polyphony_ok(sequence_cursor_step, sequence_event_length_steps(event, SEQUENCE_MAX_TEMPO),
                               event_index, selected_instrument)) {
        sequence_show_warning(1u);
        return 0;
    }
    lock = plat_lock();
    if (event_index >= 0)
        sequence.events[event_index] = event;
    else {
        sequence.events[sequence.header.count] = event;
        sequence.header.count++;
    }
    plat_unlock(lock);
    sequence_store_track_sound();
    sequence_extend_to(sequence_cursor_step + sequence_event_length_steps(event, sequence.header.tempo));
    ui_drawn = 0u;
    return 1;
}

static void sequence_delete_note(void)
{
    int32_t event_index = sequence_find_event(sequence_cursor_track, sequence_cursor_step);
    if (event_index >= 0)
        sequence_remove_event((uint32_t)event_index);
}

static uint8_t sequence_master_duration(uint32_t position)
{
    uint32_t index = position * 7u / 4097u;
    return (uint8_t)(index > 6u ? 6u : index);
}

static void sequence_stop(void)
{
    if (sequence_playing) {
        sequence_playing = 0u;
        sequence_stop_requested = 1u;
        sequence_audio_started = 0u;
        sequence_audio_start = sequence_screen == SCREEN_SEQUENCE ? sequence_cursor_step : 0u;
    }
}

static void sequence_start(uint8_t from_playback)
{
    sequence_t *source;
    uint32_t start;
    sequence_audio_stealing = 0u;
    sequence_audio_steal_limit = SEQUENCE_VOICES;
    sequence_audio_events = 0;
    sequence_audio_event_count = 0u;
    if (from_playback) {
        if (sequence_selected_slot >= SEQUENCE_SLOTS || !sequence_slots[sequence_selected_slot].used)
            return;
        if (!sequence_load_slot(sequence_selected_slot, &sequence_playback)) {
            sequence_show_warning(4u);
            return;
        }
        source = &sequence_playback;
        if (sequence_selected_slot >= SEQUENCE_USER_SLOTS) {
            uint32_t index = sequence_selected_slot - SEQUENCE_USER_SLOTS;
            const sequence_builtin_t *builtin = &sequence_builtins[index];
            if (sequence_builtin_factory_matches(index, source, 1)) {
                if (index == 0u) {
                    sequence_audio_events = builtin->steal_events;
                    sequence_audio_event_count = builtin->steal_count;
                    sequence_audio_steal_limit = ESCAPE_EXPERIMENT_VOICES;
                    sequence_escape_cpu_peak = 0u;
                    sequence_escape_xruns_start = plat_xruns();
                } else {
                    sequence_audio_steal_limit = BUILTIN_STEAL_VOICES;
                    for (uint32_t i = 0u; i < builtin->steal_count; i++)
                        source->events[i] = sequence_trim_builtin_event(index,
                            sequence_arrange_builtin_event(index, builtin->steal_events[i]));
                    source->header.count = (uint16_t)builtin->steal_count;
                }
                sequence_audio_stealing = 1u;
            }
        }
        start = 0u;
    } else {
        source = &sequence;
        start = sequence_cursor_step;
    }
    sequence_audio_start = start;
    sequence_audio_step = start;
    sequence_audio_length = from_playback
        ? (uint32_t)source->header.bars * sequence_steps_per_bar[source->header.meter]
        : sequence_total_steps();
    sequence_audio_samples = 0.0f;
    sequence_audio_started = 0u;
    sequence_audio_from_saved = from_playback;
    sequence_follow_playback = !from_playback;
    sequence_playback_tempo = source->header.tempo;
    sequence_stop_requested = 0u;
    sequence_playing = 1u;
}

static void sequence_enter(void)
{
    if (!sequence_tempo_initialized) {
        sequence.header.tempo = (uint8_t)requested_tempo;
        sequence_tempo_initialized = 1u;
    }
    sequence_screen = SCREEN_SEQUENCE;
    sequence_warning = 0u;
    sequence_notice = SEQUENCE_NOTICE_NONE;
    sequence_last_play_press_valid = 0u;
    sequence_cursor_track = 0u;
    sequence_cursor_step = 0u;
    sequence_duration_index = sequence_master_duration(plat_master());
    sequence_load_cursor_sound();
    sequence_last_note = 60u;
    sequence_last_note_valid = 0u;
    ui_drawn = 0u;
}

static void sequence_to_main(void)
{
    sequence_stop();
    all_notes_off();
    sequence_screen = SCREEN_MAIN;
    ui_drawn = 0u;
}

static void sequence_open_playback(void)
{
    all_notes_off();
    sequence_scan_slots();
    sequence_warning = 0u;
    sequence_selected_slot = 0u;
    while (sequence_selected_slot < SEQUENCE_SLOTS && !sequence_slots[sequence_selected_slot].used)
        sequence_selected_slot++;
    sequence_screen = SCREEN_PLAYBACK;
    sequence_stop();
    if (sequence_builtin_seed_failed)
        sequence_show_warning(7u);
    ui_drawn = 0u;
}

static void sequence_edit_selected(void)
{
    uint8_t slot = sequence_selected_slot;
    if (slot >= SEQUENCE_SLOTS || !sequence_slots[slot].used) {
        sequence_show_warning(5u);
        return;
    }
    if (sequence_playing)
        sequence_stop();
    if (!sequence_load_slot(slot, &sequence)) {
        sequence_show_warning(4u);
        return;
    }
    sequence_editing_saved = 1u;
    sequence_editing_slot = slot;
    sequence_tempo_initialized = 1u;
    sequence_screen = SCREEN_SEQUENCE;
    sequence_warning = 0u;
    sequence_notice = SEQUENCE_NOTICE_NONE;
    sequence_last_play_press_valid = 0u;
    sequence_cursor_track = 0u;
    sequence_cursor_step = 0u;
    sequence_duration_index = sequence_master_duration(plat_master());
    sequence_follow_playback = 0u;
    sequence_load_cursor_sound();
    sequence_last_note = 60u;
    sequence_last_note_valid = 0u;
    ui_drawn = 0u;
}

static void sequence_step_slot(int32_t delta)
{
    int32_t slot = sequence_selected_slot;
    int32_t steps;
    if (!delta || !sequence_slot_count || sequence_playing)
        return;
    steps = delta % sequence_slot_count;
    if (steps < 0)
        steps = -steps;
    while (steps--) {
        int32_t moved;
        for (moved = 0; moved < (int32_t)SEQUENCE_SLOTS; moved++) {
            slot = (slot + (delta > 0 ? 1 : (int32_t)SEQUENCE_SLOTS - 1))
                % (int32_t)SEQUENCE_SLOTS;
            if (sequence_slots[slot].used)
                break;
        }
    }
    sequence_selected_slot = (uint8_t)slot;
    ui_drawn = 0u;
}

static void sequence_step_user_slot(int32_t delta)
{
    int32_t slot = sequence_selected_slot;
    int32_t steps;
    uint32_t used = 0u;
    if (!delta || sequence_playing)
        return;
    for (uint32_t i = 0; i < SEQUENCE_USER_SLOTS; i++)
        if (sequence_slots[i].used)
            used++;
    if (!used)
        return;
    if (slot >= (int32_t)SEQUENCE_USER_SLOTS || !sequence_slots[slot].used) {
        slot = 0;
        while (slot < (int32_t)SEQUENCE_USER_SLOTS && !sequence_slots[slot].used)
            slot++;
    }
    steps = delta % (int32_t)used;
    if (steps < 0)
        steps = -steps;
    while (steps--) {
        int32_t moved;
        for (moved = 0; moved < (int32_t)SEQUENCE_USER_SLOTS; moved++) {
            slot = (slot + (delta > 0 ? 1 : (int32_t)SEQUENCE_USER_SLOTS - 1))
                % (int32_t)SEQUENCE_USER_SLOTS;
            if (sequence_slots[slot].used)
                break;
        }
    }
    sequence_selected_slot = (uint8_t)slot;
    ui_drawn = 0u;
}

static void sequence_start_name_prompt(uint8_t slot)
{
    sequence_save_slot = slot;
    sequence_warning = 0u;
    sequence_name_cursor = 0u;
    sequence_name_length = 0u;
    if (sequence_editing_saved && slot == sequence_editing_slot) {
        for (uint32_t i = 0; i < SEQUENCE_NAME_LENGTH - 1u && sequence.header.name[i]; i++)
            sequence_name_length = (uint8_t)(i + 1u);
        while (sequence_name_length && sequence.header.name[sequence_name_length - 1u] == ' ')
            sequence_name_length--;
        sequence_name_value = (uint8_t)(sizeof SEQUENCE_NAME_ALPHABET - 2u);
        for (uint32_t i = 0; i < sizeof SEQUENCE_NAME_ALPHABET - 1u; i++) {
            if (SEQUENCE_NAME_ALPHABET[i] == sequence.header.name[0]) {
                sequence_name_value = (uint8_t)i;
                break;
            }
        }
    } else {
        sequence_name_value = (uint8_t)(sizeof SEQUENCE_NAME_ALPHABET - 2u);
        for (uint32_t i = 0; i < SEQUENCE_NAME_LENGTH; i++)
            sequence.header.name[i] = ' ';
        sequence.header.name[SEQUENCE_NAME_LENGTH - 1u] = 0;
    }
    sequence_screen = SCREEN_NAME;
    ui_drawn = 0u;
}

static void sequence_begin_save(void)
{
    sequence_scan_slots();
    if (sequence_editing_saved) {
        if (sequence_editing_slot < SEQUENCE_SLOTS && sequence_slots[sequence_editing_slot].used) {
            sequence_start_name_prompt(sequence_editing_slot);
            return;
        }
        sequence_editing_saved = 0u;
    }
    for (uint32_t slot = 0; slot < SEQUENCE_USER_SLOTS; slot++) {
        if (!sequence_slots[slot].used) {
            sequence_start_name_prompt((uint8_t)slot);
            return;
        }
    }
    sequence_selected_slot = 0u;
    while (sequence_selected_slot < SEQUENCE_USER_SLOTS && !sequence_slots[sequence_selected_slot].used)
        sequence_selected_slot++;
    sequence_screen = SCREEN_DELETE;
    ui_drawn = 0u;
}

static void sequence_name_turn(int32_t delta)
{
    int32_t next = delta % (int32_t)(sizeof SEQUENCE_NAME_ALPHABET - 1u);
    if (!delta)
        return;
    next = ((int32_t)sequence_name_value + next) % (int32_t)(sizeof SEQUENCE_NAME_ALPHABET - 1u);
    if (next < 0)
        next += (int32_t)(sizeof SEQUENCE_NAME_ALPHABET - 1u);
    sequence_name_value = (uint8_t)next;
    sequence.header.name[sequence_name_cursor] = SEQUENCE_NAME_ALPHABET[next];
    if (sequence_name_cursor >= sequence_name_length && SEQUENCE_NAME_ALPHABET[next] != ' ')
        sequence_name_length = (uint8_t)(sequence_name_cursor + 1u);
    while (sequence_name_length && sequence.header.name[sequence_name_length - 1u] == ' ')
        sequence_name_length--;
}

static uint8_t sequence_name_char_index(char character)
{
    for (uint32_t i = 0; i < sizeof SEQUENCE_NAME_ALPHABET - 1u; i++)
        if (SEQUENCE_NAME_ALPHABET[i] == character)
            return (uint8_t)i;
    return (uint8_t)(sizeof SEQUENCE_NAME_ALPHABET - 2u);
}

static void sequence_name_move(int32_t delta)
{
    int64_t cursor = (int64_t)sequence_name_cursor + delta;
    if (cursor < 0)
        cursor = 0;
    if (cursor >= (int32_t)SEQUENCE_NAME_LENGTH - 1)
        cursor = (int32_t)SEQUENCE_NAME_LENGTH - 2;
    sequence_name_cursor = (uint8_t)cursor;
    sequence_name_value = sequence_name_char_index(sequence.header.name[sequence_name_cursor]);
    ui_drawn = 0u;
}

static void sequence_name_delete(void)
{
    for (uint32_t i = sequence_name_cursor; i + 1u < SEQUENCE_NAME_LENGTH - 1u; i++)
        sequence.header.name[i] = sequence.header.name[i + 1u];
    sequence.header.name[SEQUENCE_NAME_LENGTH - 2u] = ' ';
    if (sequence_name_cursor)
        sequence_name_cursor--;
    sequence_name_length = 0u;
    for (uint32_t i = 0; i < SEQUENCE_NAME_LENGTH - 1u; i++)
        if (sequence.header.name[i] != ' ')
            sequence_name_length = (uint8_t)(i + 1u);
    sequence_name_value = sequence_name_char_index(sequence.header.name[sequence_name_cursor]);
    ui_drawn = 0u;
}

static void sequence_save_named(void)
{
    if (!sequence_name_length) {
        sequence_show_warning(3u);
        return;
    }
    sequence.header.name[sequence_name_length] = 0;
    if (sequence_store_slot(sequence_save_slot) != 0) {
        sequence_show_warning(4u);
        return;
    }
    sequence_scan_slots();
    sequence_screen = SCREEN_SEQUENCE;
    ui_drawn = 0u;
}

static void sequence_delete_selected(void)
{
    if (sequence_selected_slot >= SEQUENCE_USER_SLOTS || !sequence_slots[sequence_selected_slot].used)
        return;
    if (plat_store_save(sequence_slot_object(sequence_selected_slot), &sequence, 0u) != 0) {
        sequence_show_warning(4u);
        return;
    }
    sequence_scan_slots();
    sequence_start_name_prompt(sequence_selected_slot);
}

static void sequence_delete_saved_track(void)
{
    uint8_t deleted_slot = sequence_selected_slot;
    if (deleted_slot >= SEQUENCE_SLOTS || !sequence_slots[deleted_slot].used)
        return;
    if (sequence_playing)
        sequence_stop();
    if (plat_store_save(sequence_slot_object(deleted_slot), &sequence, 0u) != 0) {
        sequence_show_warning(6u);
        return;
    }
    if (sequence_editing_saved && sequence_editing_slot == deleted_slot)
        sequence_editing_saved = 0u;
    sequence_scan_slots();
    sequence_selected_slot = 0u;
    for (uint32_t offset = 1u; offset <= SEQUENCE_SLOTS; offset++) {
        uint32_t slot = ((uint32_t)deleted_slot + offset) % SEQUENCE_SLOTS;
        if (sequence_slots[slot].used) {
            sequence_selected_slot = (uint8_t)slot;
            break;
        }
    }
    ui_drawn = 0u;
}

static void sequence_change_meter(void)
{
    uint8_t meter = (uint8_t)((sequence.header.meter + 1u) % METER_COUNT);
    uint32_t max_steps = (uint32_t)sequence_steps_per_bar[meter] * 128u;
    uint32_t required = sequence_total_steps();
    if ((uint32_t)sequence_cursor_step + 1u > required)
        required = (uint32_t)sequence_cursor_step + 1u;
    for (uint32_t i = 0; i < sequence.header.count; i++) {
        uint32_t end = sequence_event_step(sequence.events[i])
            + sequence_event_length_steps(sequence.events[i], sequence.header.tempo);
        if (end > required)
            required = end;
    }
    if (required > max_steps) {
        sequence_show_warning(2u);
        return;
    }
    sequence.header.meter = meter;
    sequence.header.bars = (uint8_t)((required + sequence_steps_per_bar[meter] - 1u)
                                     / sequence_steps_per_bar[meter]);
    if (!sequence.header.bars)
        sequence.header.bars = 1u;
    ui_drawn = 0u;
}

static int sequence_set_tempo(uint8_t tempo)
{
    uint32_t required = 0u;
    for (uint32_t i = 0; i < sequence.header.count; i++) {
        uint32_t event = sequence.events[i];
        uint32_t end = sequence_event_step(event) + sequence_event_length_steps(event, tempo);
        if (end > required)
            required = end;
    }
    if (required > 128u * sequence_steps_per_bar[sequence.header.meter]) {
        sequence_show_warning(2u);
        return 0;
    }
    sequence.header.tempo = tempo;
    sequence_extend_to(required);
    if (sequence_playing && !sequence_audio_from_saved)
        sequence_playback_tempo = tempo;
    return 1;
}

static void sequence_help_toggle(void)
{
    if (sequence_screen == SCREEN_HELP) {
        sequence_screen = sequence_help_return_screen;
    } else if (sequence_screen == SCREEN_MAIN || sequence_screen == SCREEN_SEQUENCE
               || sequence_screen == SCREEN_PLAYBACK) {
        sequence_help_return_screen = sequence_screen;
        sequence_help_page = 0u;
        sequence_help_master = plat_master();
        sequence_screen = SCREEN_HELP;
    }
    ui_drawn = 0u;
}

static uint8_t sequence_help_page_count(void)
{
    return sequence_help_return_screen == SCREEN_SEQUENCE ? 4u
        : sequence_help_return_screen == SCREEN_PLAYBACK ? 2u : 4u;
}

static void sequence_show_notice(uint8_t notice)
{
    sequence_notice = notice;
    sequence_notice_until = plat_ms() + 1800u;
    ui_drawn = 0u;
}

static void sequence_show_fx_notice(uint8_t knob)
{
    const char *label = knob_setting_label(knob);
    sequence_notice_knob = knob;
    sequence_notice_value = knob_setting_value(knob);
    for (uint32_t i = 0; i < 3u; i++)
        sequence_notice_fx_label[i] = label[i];
    sequence_notice_fx_label[3] = 0;
    sequence_show_notice(SEQUENCE_NOTICE_FX);
}

#ifdef HAVE_SONG
static void toggle_escape_play(void)
{
    if (!BACKING_ENABLED)
        return;
    requested_escape_play = (uint8_t)!requested_escape_play;
    if (requested_escape_play) {
        requested_play = 0u;
        arp_stop();
        phrase_abort();
        for (uint32_t k = 0; k < NKEYS; k++)
            release_live_key(k);
    }
}
#endif

/* Quick layers, after Felucca's: hold FX, GLO, SEQ or LFO for a layer; a short tap with nothing else touched is the
 * button's own action. FX, GLO and SEQ turn the lowest keys into shortcuts (silent while the layer is held); LFO
 * keeps the keys playing and undoes any knob turns when it is let go (a momentary change). */
#define LAYER_HOLD_MS 400u
typedef struct {
    uint8_t strings[2][4], voice[TRACKS][4], chord[4], cs80[4], drum[4];
} controls_snapshot_t;

static controls_snapshot_t momentary;
static int8_t layer_btn = -1, fx_key = -1;
static uint8_t layer_used;
static uint32_t layer_start, layer_keys, tap_last_ms;
static volatile uint8_t *fx_target;
static uint8_t fx_saved;

static void snapshot_controls(controls_snapshot_t *s)
{
    for (uint32_t i = 0; i < 4u; i++) {
        for (uint32_t p = 0; p < 2u; p++)
            s->strings[p][i] = string_controls[p][i];
        for (uint32_t t = 0; t < TRACKS; t++)
            s->voice[t][i] = voice_controls[t][i];
        s->chord[i] = chord_controls[i];
        s->cs80[i] = cs80_controls[i];
    }
    s->drum[0] = drum_chorus;
    s->drum[1] = drum_reverb;
    s->drum[2] = drum_compression;
    s->drum[3] = drum_bass_boost;
}

static void restore_controls(const controls_snapshot_t *s)
{
    for (uint32_t i = 0; i < 4u; i++) {
        for (uint32_t p = 0; p < 2u; p++)
            string_controls[p][i] = s->strings[p][i];
        for (uint32_t t = 0; t < TRACKS; t++)
            voice_controls[t][i] = s->voice[t][i];
        chord_controls[i] = s->chord[i];
        cs80_controls[i] = s->cs80[i];
    }
    drum_chorus = s->drum[0];
    drum_reverb = s->drum[1];
    drum_compression = s->drum[2];
    drum_bass_boost = s->drum[3];
}

static void fx_restore(void)
{
    if (fx_target)
        *fx_target = fx_saved;
    fx_target = 0;
    fx_key = -1;
}

static void tap_tempo(void)
{
    uint32_t now = plat_ms();
    uint32_t gap = now - tap_last_ms;
    if (tap_last_ms && gap > 0u && gap <= 2000u) {
        int32_t tempo = clamp((int32_t)(60000u / gap), 70, 140);
        requested_tempo = tempo;
        settings.tempo = (uint8_t)tempo;
    }
    tap_last_ms = now;
}

static void phrase_clear(void)
{
    phrase_abort();
    phrase_count = 0u;
    phrase_length = 0u;
}

static void phrase_reverse(void)
{
    if (phrase_state == PHRASE_REC || !phrase_count || !phrase_length)
        return;
    phrase_release_all();
    for (uint32_t i = 0; i < phrase_count; i++) {
        phrase[i].at = (uint16_t)(phrase[i].at < phrase_length ? phrase_length - 1u - phrase[i].at : 0u);
        phrase[i].key ^= PHRASE_UP;
    }
    for (uint32_t i = 0, j = phrase_count - 1u; i < j; i++, j--) {
        phrase_event_t swap = phrase[i];
        phrase[i] = phrase[j];
        phrase[j] = swap;
    }
    phrase_next = 0u;
    phrase_start = plat_ms();
}

static void layer_tap(uint8_t button)
{
    if (button == B_FX)
        cycle_effect();
    else if (button == B_GLO)
        sequence_help_toggle();
    else if (button == B_LFO)
        backing_mute ^= MUTE_BASS;
    else if (button == B_SEQ)
        sequence_enter();
}

/* FX: keys 0..3 set the effect to that level while held. GLO: keys 0..2 mute chords / bass / drums, key 3 unmutes all,
 * key 4 taps the tempo. SEQ: key 0 clears the phrase, key 1 reverses it. */
static void layer_key(uint32_t key)
{
    if (layer_btn == B_FX) {
        uint8_t limit;
        volatile uint8_t *value;
        if (key > 3u || fx_target)
            return;
        value = effect_value(&limit);
        fx_target = value;
        fx_saved = *value;
        fx_key = (int8_t)key;
        *value = limit == 127u ? (uint8_t)(key * 32u) : (uint8_t)key;
    } else if (layer_btn == B_GLO) {
        if (key < 3u)
            backing_mute ^= (uint8_t)(1u << key);
        else if (key == 3u)
            backing_mute = 0u;
        else if (key == 4u)
            tap_tempo();
    } else if (layer_btn == B_SEQ) {
        if (key == 0u)
            phrase_clear();
        else if (key == 1u)
            phrase_reverse();
        else if (key == 2u)
            seq_clear();
    }
}

static void layer_key_up(uint32_t key)
{
    if ((int32_t)key == fx_key)
        fx_restore();
}

static int layer_swallows_keys(void)
{
    return layer_btn == B_FX || layer_btn == B_GLO || layer_btn == B_SEQ;
}

static void layer_update(uint32_t buttons, uint32_t pressed, int knob_moved)
{
    static const uint8_t order[4] = {B_FX, B_GLO, B_SEQ, B_LFO};
    uint32_t now = plat_ms();
    if (layer_btn < 0) {
        for (uint32_t i = 0; i < 4u; i++) {
            if (pressed & (1u << order[i])) {
                layer_btn = (int8_t)order[i];
                layer_start = now;
                layer_used = 0u;
                if (layer_btn == B_LFO)
                    snapshot_controls(&momentary);
                break;
            }
        }
        return;
    }
    if (knob_moved)
        layer_used = 1u;
    if (buttons & (1u << layer_btn))
        return;
    if (layer_btn == B_LFO && layer_used)
        restore_controls(&momentary);
    else if (!layer_used && now - layer_start < LAYER_HOLD_MS)
        layer_tap((uint8_t)layer_btn);
    fx_restore();
    layer_btn = -1;
}

static void sequence_input(uint32_t buttons, uint32_t pressed, uint32_t keys,
                           int32_t select_delta, int32_t algorithm_delta, int32_t preset_delta,
                           int32_t k1, int32_t k2, int32_t k3, int32_t k4, uint32_t master_position)
{
    uint8_t screen = sequence_screen;
    uint32_t keys_pressed = keys & ~previous_keys;
    if (screen == SCREEN_HELP) {
        if (pressed & ((1u << B_GLO) | (1u << B_SEQ) | (1u << B_HOME)))
            sequence_help_toggle();
        else {
            int32_t page_delta = select_delta + algorithm_delta + preset_delta + k1 + k2 + k3 + k4;
            int32_t master_delta = (int32_t)master_position - (int32_t)sequence_help_master;
            if (master_delta >= 64 || master_delta <= -64) {
                if (!page_delta)
                    page_delta = master_delta;
                sequence_help_master = master_position;
            }
            if (page_delta)
                sequence_help_page = (uint8_t)((sequence_help_page
                    + (page_delta > 0 ? 1u : sequence_help_page_count() - 1u))
                    % sequence_help_page_count());
        }
        previous_keys = keys;
        previous_buttons = buttons;
        portrait_master_last = master_position;
        ui_drawn = 0u;
        return;
    }

    if (pressed & (1u << B_HOME)) {
        if (screen == SCREEN_PLAYBACK)
            sequence_to_main();
        else if (screen == SCREEN_SEQUENCE)
            sequence_to_main();
        else
            sequence_screen = SCREEN_SEQUENCE;
        previous_keys = keys;
        previous_buttons = buttons;
        portrait_master_last = master_position;
        ui_drawn = 0u;
        return;
    }
    if (pressed & (1u << B_SEQ)) {
        if (screen == SCREEN_SEQUENCE)
            sequence_to_main();
        else if (screen == SCREEN_PLAYBACK)
            sequence_to_main();
        else
            sequence_screen = SCREEN_SEQUENCE;
        previous_keys = keys;
        previous_buttons = buttons;
        portrait_master_last = master_position;
        ui_drawn = 0u;
        return;
    }
    if ((pressed & (1u << B_GLO))
        && (screen == SCREEN_SEQUENCE || screen == SCREEN_PLAYBACK)) {
        sequence_help_toggle();
        previous_keys = keys;
        previous_buttons = buttons;
        portrait_master_last = master_position;
        return;
    }
    if (screen == SCREEN_SEQUENCE && sequence_playing && sequence_follow_playback) {
        uint32_t step_index = sequence_audio_step;
        if (step_index < SEQUENCE_MAX_STEPS && step_index != sequence_cursor_step) {
            sequence_cursor_step = (uint16_t)step_index;
            sequence_load_cursor_sound();
        }
        if (algorithm_delta)
            sequence_follow_playback = 0u;
    }

    if (screen == SCREEN_SEQUENCE && selected_instrument != 5u
        && (pressed & ((1u << B_OCTUP) | (1u << B_OCTDN)))) {
        int32_t delta = !!(pressed & (1u << B_OCTUP)) - !!(pressed & (1u << B_OCTDN));
        instrument_octave[selected_instrument] =
            (int8_t)clamp(instrument_octave[selected_instrument] + delta, -2, 2);
        sequence_notice_octave = instrument_octave[selected_instrument];
        sequence_show_notice(SEQUENCE_NOTICE_OCTAVE);
    }

    if (screen == SCREEN_SEQUENCE || screen == SCREEN_PLAYBACK) {
        for (uint32_t key = 0; key < NKEYS; key++) {
            uint32_t bit = 1u << key;
            if (screen == SCREEN_SEQUENCE) {
                if ((keys & bit) && !(previous_keys & bit)) {
                    if (sequence_cursor_track == SEQUENCE_DRUM_TRACK) {
                        sequence_apply_sound((uint8_t)(12u + key % ROSSFLM1_SAMPLE_COUNT));
                        sequence_store_track_sound();
                    }
                    if (selected_instrument == 5u) {
                        trigger_drum_sample(sequence_current_variant());
                        live_key_voice[key] = -1;
                    } else {
                        trigger_live_key(key, 100u);
                    }
                    sequence_last_note = selected_instrument == 5u
                        ? sequence_current_variant()
                        : (uint8_t)clamp(53 + (int32_t)key
                                         + 12 * instrument_octave[selected_instrument], 0, 127);
                    sequence_last_note_valid = 1u;
                    sequence_notice_note = sequence_last_note;
                    sequence_notice_instrument = selected_instrument;
                    sequence_notice_variant = sequence_current_variant();
                    sequence_show_notice(SEQUENCE_NOTICE_NOTE);
                    sequence_insert_note(sequence_last_note);
                } else if (!(keys & bit) && (previous_keys & bit)) {
                    release_live_key(key);
                }
            }
        }
        previous_keys = keys;
    } else {
        previous_keys = keys;
    }

    if (screen == SCREEN_SEQUENCE) {
        if ((buttons & (1u << B_EDIT)) && k1) {
            sequence_set_tempo((uint8_t)clamp((int32_t)sequence.header.tempo + k1, 70, SEQUENCE_MAX_TEMPO));
            k1 = 0;
            ui_drawn = 0u;
        }
        if (preset_delta) {
            int32_t track = ((int32_t)sequence_cursor_track + preset_delta % SEQUENCE_TRACKS)
                % SEQUENCE_TRACKS;
            if (track < 0)
                track += SEQUENCE_TRACKS;
            sequence_cursor_track = (uint8_t)track;
            sequence_load_cursor_sound();
            ui_drawn = 0u;
        }
        if (algorithm_delta)
            sequence_move_cursor(algorithm_delta);
        if (select_delta && (!sequence_playing || sequence_cursor_track == SEQUENCE_DRUM_TRACK))
            sequence_change_sound(select_delta);
        if ((pressed & (1u << B_SEL)) && !sequence_playing)
            sequence_change_meter();
        if ((pressed & (1u << B_REC)) && !sequence_playing)
            sequence_delete_note();
        if (pressed & (1u << B_PLAY)) {
            uint32_t now = plat_ms();
            if (sequence_last_play_press_valid
                && (uint32_t)(now - sequence_last_play_press) <= SEQUENCE_PLAY_RESET_MS) {
                if (sequence_playing)
                    sequence_stop();
                sequence_cursor_step = 0u;
                sequence_follow_playback = 0u;
                sequence_last_play_press_valid = 0u;
                sequence_load_cursor_sound();
                ui_drawn = 0u;
            } else {
                if (sequence_playing)
                    sequence_stop();
                else
                    sequence_start(0u);
                sequence_last_play_press = now;
                sequence_last_play_press_valid = 1u;
            }
        }
        if ((pressed & (1u << B_SAVE)) && !sequence_playing)
            sequence_begin_save();
        {
            uint8_t duration = sequence_master_duration(master_position);
            if (duration != sequence_master_duration(portrait_master_last) && !sequence_playing
                && sequence_cursor_track != SEQUENCE_DRUM_TRACK)
                sequence_change_duration(duration);
        }
        if (selected_instrument == 5u) {
            if (k1) drum_chorus = (uint8_t)clamp((int32_t)drum_chorus + k1, 0, 3);
            if (k2) drum_reverb = (uint8_t)clamp((int32_t)drum_reverb + k2, 0, 3);
            if (k3) drum_compression = (uint8_t)clamp((int32_t)drum_compression + k3, 0, 3);
            if (k4) drum_bass_boost = (uint8_t)clamp((int32_t)drum_bass_boost + k4, 0, 8);
        } else if (selected_instrument == 3u || selected_instrument == 4u) {
            uint32_t part = selected_instrument - 3u;
            int32_t deltas[4] = {k1, k2, k3, k4};
            for (uint32_t knob = 0; knob < 4u; knob++)
                if (deltas[knob])
                    string_controls[part][knob] =
                        (uint8_t)clamp((int32_t)string_controls[part][knob] + deltas[knob], 0, 3);
        } else if (selected_instrument == 7u) {
            int32_t deltas[4] = {k1, k2, k3, k4};
            for (uint32_t knob = 0; knob < 4u; knob++)
                if (deltas[knob])
                    chord_controls[knob] = (uint8_t)clamp(
                        (int32_t)chord_controls[knob] + deltas[knob], 0,
                        knob == 0u ? (int32_t)CHORD_TYPES - 1 : 3);
        } else if (selected_instrument == 6u && cs80_preset) {
            int32_t deltas[4] = {k1, k2, k3, k4};
            for (uint32_t knob = 0; knob < 4u; knob++)
                if (deltas[knob])
                    cs80_controls[knob] = (uint8_t)clamp(
                        (int32_t)cs80_controls[knob] + deltas[knob] * 4, 0, 127);
        } else {
            int32_t deltas[4] = {k1, k2, k3, k4};
            for (uint32_t knob = 0; knob < 4u; knob++)
                if (deltas[knob])
                    voice_controls[selected_instrument][knob] =
                        (uint8_t)clamp((int32_t)voice_controls[selected_instrument][knob]
                                       + deltas[knob], 0, 3);
        }
        if (k1) sequence_show_fx_notice(0u);
        if (k2) sequence_show_fx_notice(1u);
        if (k3) sequence_show_fx_notice(2u);
        if (k4) sequence_show_fx_notice(3u);
    } else if (screen == SCREEN_NAME) {
        sequence_name_turn(preset_delta);
        sequence_name_move(algorithm_delta);
        if (pressed & (1u << B_SEL))
            sequence_name_delete();
        if (pressed & (1u << B_SAVE))
            sequence_save_named();
    } else if (screen == SCREEN_DELETE) {
        sequence_step_user_slot(preset_delta);
        if (pressed & (1u << B_SEL))
            sequence_delete_selected();
    } else if (screen == SCREEN_PLAYBACK) {
        sequence_step_slot(preset_delta);
        if (keys_pressed) {
            sequence_edit_selected();
        } else if (pressed & (1u << B_REC)) {
            sequence_delete_saved_track();
        } else if (pressed & (1u << B_PLAY)) {
            if (sequence_playing)
                sequence_stop();
            else if (sequence_slot_count)
                sequence_start(1u);
            else
                sequence_show_warning(5u);
        }
    }
    previous_buttons = buttons;
    portrait_master_last = master_position;
}

void app_input_only(void)
{
    uint32_t buttons = plat_buttons();
    uint32_t keys;
    int32_t delta = plat_enc(EN_K1);
    int32_t k2 = plat_enc(EN_K2), k3 = plat_enc(EN_K3);
    int32_t instrument_delta = plat_enc(EN_SELECT);
    int32_t algorithm_delta = plat_enc(EN_ALGO);
    int32_t preset_delta = plat_enc(EN_PRESET);
    int32_t boost_delta = plat_enc(EN_K4);
    uint32_t master_position = plat_master();
    int32_t master_delta = (int32_t)master_position - (int32_t)portrait_master_last;
    int edit_held = (buttons & (1u << B_EDIT)) != 0u;
    uint32_t pressed = buttons & ~previous_buttons;
 #ifdef APP_TOUCH
    touch_poll();
    keys = touch_keys();
#else
    keys = plat_keys();
#endif
    if (sequence_screen != SCREEN_MAIN) {
        sequence_input(buttons, pressed, keys, instrument_delta, algorithm_delta, preset_delta,
                       delta, k2, k3, boost_delta, master_position);
        return;
    }
    if (pressed & (1u << B_PLAY)) {
        sequence_open_playback();
        previous_keys = keys;
        previous_buttons = buttons;
        portrait_master_last = master_position;
        return;
    }
    layer_update(buttons, pressed, delta || k2 || k3 || boost_delta);
    if (pressed & (1u << B_HOME))
        all_notes_off();
#ifdef APP_TOUCH
    if (requested_escape_play) {
        previous_keys = keys;
    } else {
        for (uint32_t k = 0; k < NKEYS; k++) {
            uint32_t bit = 1u << k;
            if ((keys & bit) && !(previous_keys & bit)) {
                uint32_t velocity;
                if (layer_swallows_keys()) {
                    layer_keys |= bit;
                    layer_used = 1u;
                    layer_key(k);
                    continue;
                }
                velocity = touch_vel(k);
                if (arp_on)
                    arp_held |= bit;
                else
                    trigger_live_key(k, velocity);
                phrase_record(k, velocity, 1);
                seq_record(k);
            } else if (!(keys & bit) && (previous_keys & bit)) {
                if (layer_keys & bit) {
                    layer_keys &= ~bit;
                    layer_key_up(k);
                    continue;
                }
                if (arp_on) {
                    arp_held &= ~bit;
                    if (arp_key == (int32_t)k) {
                        release_live_key(k);
                        arp_key = -1;
                    }
                } else {
                    release_live_key(k);
                }
                phrase_record(k, 0u, 0);
            }
        }
        previous_keys = keys;
    }
#endif
    if (requested_escape_play) {
        portrait_master_last = master_position;
        portrait_master_accumulator = 0;
        previous_buttons = buttons;
        return;
    }
    portrait_master_last = master_position;
    if (pressed & (1u << B_SEL))
        cycle_variant();
    if (pressed & (1u << B_ENV))
        backing_mute ^= MUTE_CHORDS;
    if (pressed & (1u << B_ARP)) {
        uint32_t held = previous_keys;
        arp_stop();
        arp_on = (uint8_t)!arp_on;
        if (arp_on) {
            for (uint32_t k = 0; k < NKEYS; k++)
                release_live_key(k);
            arp_held = held;
            arp_next_ms = plat_ms();
        }
    }
    arp_update();
    if (pressed & (1u << B_REC))
        phrase_button(edit_held);
    phrase_update();
    portrait_master_accumulator += master_delta;
    while (portrait_master_accumulator >= 32) {
        record_portrait_turn(1);
        portrait_master_accumulator -= 32;
    }
    while (portrait_master_accumulator <= -32) {
        record_portrait_turn(-1);
        portrait_master_accumulator += 32;
    }
    record_portrait_turn(instrument_delta);
    record_portrait_turn(algorithm_delta);
    record_portrait_turn(preset_delta);
    if (selected_instrument != 5u) {
        int32_t octave_delta = !!(pressed & (1u << B_OCTUP)) - !!(pressed & (1u << B_OCTDN));
        instrument_octave[selected_instrument] =
            (int8_t)clamp(instrument_octave[selected_instrument] + octave_delta, -2, 2);
    }
    if (delta && (buttons & (1u << B_EDIT))) {
        int32_t tempo = clamp(requested_tempo + delta, 70, 140);
        requested_tempo = tempo;
        settings.tempo = (uint8_t)tempo;
    }
    if (instrument_delta) {
        int32_t instrument = ((int32_t)selected_instrument + instrument_delta) % (int32_t)INSTRUMENTS;
        if (instrument < 0)
            instrument += (int32_t)INSTRUMENTS;
        selected_instrument = (uint8_t)instrument;
    }
    if (selected_instrument == 5u) {
        if (!(buttons & (1u << B_EDIT)) && delta)
            drum_chorus = (uint8_t)clamp((int32_t)drum_chorus + delta, 0, 3);
        if (k2)
            drum_reverb = (uint8_t)clamp((int32_t)drum_reverb + k2, 0, 3);
        if (k3)
            drum_compression = (uint8_t)clamp((int32_t)drum_compression + k3, 0, 3);
        if (boost_delta)
            drum_bass_boost = (uint8_t)clamp((int32_t)drum_bass_boost + boost_delta, 0, 8);
    }
    /* PRESET or ALGO selects variants; ALGO toggles piano engine and selects CHORD layout. */
    if (preset_delta || algorithm_delta) {
        int32_t d = preset_delta + (selected_instrument == 7u ? 0 : algorithm_delta);
        if (selected_instrument == 1u) {
            if (algorithm_delta % 2) {
                piano_sampled ^= 1u;
                ui_drawn = 0u;
            }
        } else if (selected_instrument == 4u)
            step_bass_mode(d);
        else if (selected_instrument == 3u)
            step_guitar_mode(d);
        else if (selected_instrument == 6u && d)
            set_cs80_preset((int32_t)cs80_preset + d);
        else if (selected_instrument == 7u) {
            int32_t voice = ((int32_t)chord_voice + d) % 5;
            if (voice < 0)
                voice += 5;
            chord_voice = (uint8_t)voice;
            chord_layout = (uint8_t)clamp((int32_t)chord_layout + algorithm_delta, 0, 1);
        }
    }
    if (selected_instrument == 3u || selected_instrument == 4u) {
        uint32_t part = selected_instrument - 3u;
        int32_t deltas[4] = {(edit_held ? 0 : delta), k2, k3, boost_delta};
        for (uint32_t k = 0; k < 4u; k++)
            if (deltas[k])
                string_controls[part][k] = (uint8_t)clamp((int32_t)string_controls[part][k] + deltas[k], 0, 3);
    }
    if (selected_instrument == 0u || selected_instrument == 1u
        || selected_instrument == 2u || (selected_instrument == 6u && !cs80_preset)) {
        int32_t deltas[4] = {(buttons & (1u << B_EDIT)) ? 0 : delta, k2, k3, boost_delta};
        for (uint32_t k = 0; k < 4u; k++)
            if (deltas[k])
                voice_controls[selected_instrument][k] =
                    (uint8_t)clamp((int32_t)voice_controls[selected_instrument][k] + deltas[k], 0, 3);
    }
    if (selected_instrument == 7u) {
        int32_t deltas[4] = {(edit_held ? 0 : delta), k2, k3, boost_delta};
        for (uint32_t k = 0; k < 4u; k++)
            if (deltas[k])
                chord_controls[k] = (uint8_t)clamp((int32_t)chord_controls[k] + deltas[k], 0,
                                                   k == 0u ? (int32_t)CHORD_TYPES - 1 : 3);

    }
    if (selected_instrument == 6u && cs80_preset) {
        int32_t deltas[4] = {(edit_held ? 0 : delta), k2, k3, boost_delta};
        for (uint32_t k = 0; k < 4u; k++)
            if (deltas[k])
                cs80_controls[k] = (uint8_t)clamp((int32_t)cs80_controls[k] + deltas[k] * 4, 0, 127);
    }
    record_knob_turn(0u, delta);
    record_knob_turn(1u, k2);
    record_knob_turn(2u, k3);
    record_knob_turn(3u, boost_delta);
    if ((buttons & (1u << B_SAVE)) && !(previous_buttons & (1u << B_SAVE))) {
        int rc = plat_store_save(OBJ_SETTINGS, &settings, sizeof settings);
        save_failed = (uint8_t)(rc != 0);
    }
    previous_buttons = buttons;
}

static void draw_text(int32_t x, int32_t y, const char *text, uint16_t color)
{
    cv_text(x, y, &FONT_XS, text, color);
}

static void draw_escape_prompt(void)
{
    static const uint16_t colors[8] = {
        RGB(80, 220, 255), RGB(255, 190, 70), RGB(120, 230, 120), RGB(80, 160, 255),
        RGB(255, 220, 100), RGB(100, 240, 180), RGB(255, 140, 100), RGB(160, 210, 255)
    };
    int32_t x;

    cv_begin(240, 96, C_BLACK);
    x = cv_text(70, 20, &FONT_XS, "Press ", colors[0]);
    x = cv_text(x, 20, &FONT_XS, "Edit ", colors[1]);
    x = cv_text(x, 20, &FONT_XS, "+ ", colors[2]);
    cv_text(x, 20, &FONT_XS, "Play", colors[3]);
    x = cv_text(60, 48, &FONT_XS, "to ", colors[4]);
    x = cv_text(x, 48, &FONT_XS, "Escape ", colors[5]);
    x = cv_text(x, 48, &FONT_XS, "New ", colors[6]);
    cv_text(x, 48, &FONT_XS, "York", colors[7]);
    cv_blit(0, 144);
}

static const uint16_t knob_colors[4] = {
    RGB(80, 220, 255), RGB(120, 230, 120), RGB(255, 140, 180), RGB(255, 190, 70)
};

static uint8_t knob_setting_value(uint32_t knob);
static uint16_t knob_image_shade(uint16_t color, uint8_t shade);

/* JC's brightness follows the battery bars (0..3); USB power or no reading shows full brightness. */
static uint8_t battery_brightness(void)
{
    static const uint8_t percent[4] = {25u, 45u, 70u, 100u};
    int bars = plat_battery();
    return bars >= 0 && bars <= 3 ? percent[bars] : 100u;
}

static uint16_t dim_color(uint16_t color, uint32_t percent)
{
    uint32_t r = ((color >> 11) & 31u) * percent / 100u;
    uint32_t g = ((color >> 5) & 63u) * percent / 100u;
    uint32_t b = (color & 31u) * percent / 100u;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

static uint8_t ui_jc_brightness = 100u;

static void draw_jc_face(void)
{
    uint32_t percent = battery_brightness();
    uint32_t i, out = 0u, run;
    uint32_t x = (240u - JC_FACE_WIDTH) / 2u;
    for (i = 0u; i < JC_FACE_RLE_SIZE; i += 2u) {
        uint8_t color = jc_face_rle[i + 1u];
        for (run = 0u; run < jc_face_rle[i]; run++, out++)
            if (color)
                cv_px[(out / JC_FACE_WIDTH) * 240u + x + out % JC_FACE_WIDTH] =
                    swap16(dim_color(portrait_color_visible ? knob_image_shade(knob_colors[portrait_color], color)
                                  : jc_face_palette[color], percent));
    }
}

static uint8_t knob_setting_value(uint32_t knob)
{
    uint8_t drum_values[4] = {
        drum_chorus, drum_reverb, drum_compression, drum_bass_boost
    };
    if (selected_instrument == 5u)
        return drum_values[knob];
    if (selected_instrument == 3u || selected_instrument == 4u)
        return string_controls[selected_instrument - 3u][knob];
    if (selected_instrument == 7u)
        return chord_controls[knob];
    if (selected_instrument == 6u && cs80_preset)
        return cs80_controls[knob];
    return voice_controls[selected_instrument][knob];
}

static const char *knob_setting_label(uint32_t knob)
{
    static const char *const drum_labels[4] = {"CHR", "REV", "CMP", "BAS"};
    static const char *const string_labels[4] = {"DRV", "CHR", "MOD", "DLY"};
    static const char *const piano_labels[4] = {"TON", "DEC", "CHR", "REV"};
    static const char *const synth_labels[4] = {"CUT", "RES", "ENV", "MOD"};
    static const char *const chord_labels[4] = {"TYP", "VOI", "INV", "SPR"};
    static const char *const cs80_labels[4] = {"BRI", "RES", "MIX", "VOL"};
    if (selected_instrument == 5u)
        return drum_labels[knob];
    if (selected_instrument == 3u || selected_instrument == 4u)
        return string_labels[knob];
    if (selected_instrument == 7u)
        return chord_labels[knob];
    if (selected_instrument == 6u && cs80_preset)
        return cs80_labels[knob];
    return selected_instrument == 1u ? piano_labels[knob] : synth_labels[knob];
}

static void record_knob_turn(uint8_t knob, int32_t delta)
{
    uint32_t frame;
    if (!delta)
        return;
    if (selected_instrument == 6u && cs80_preset) {
        if (!knob_image_visible)
            knob_image_frame = 3u;
        else if (knob_image_frame == 2u || knob_image_frame == 3u)
            knob_image_frame = 0u;
        else
            knob_image_frame++;
        knob_image_visible = 1u;
        knob_image_color = knob;
        return;
    }
    frame = knob_setting_value(knob);
    knob_image_visible = 1u;
    knob_image_frame = (uint8_t)(frame > 3u ? 3u : frame);
    knob_image_color = knob;
}

static void record_portrait_turn(int32_t delta)
{
    uint32_t turns;
    if (!delta)
        return;
    turns = (uint32_t)(delta < 0 ? -delta : delta);
    while (turns--) {
        if (portrait_color_visible)
            portrait_color = (uint8_t)((portrait_color + 1u) & 3u);
        else {
            portrait_color = 0u;
            portrait_color_visible = 1u;
        }
    }
}

static uint16_t knob_image_shade(uint16_t color, uint8_t shade)
{
    uint32_t red = ((color >> 11) & 31u) * shade / 3u;
    uint32_t green = ((color >> 5) & 63u) * shade / 3u;
    uint32_t blue = (color & 31u) * shade / 3u;
    return (uint16_t)((red << 11) | (green << 5) | blue);
}

static void draw_knob_image(void)
{
    uint32_t i, out = 0u, run;
    const uint8_t *rle;
    if (!knob_image_visible)
        return;
    rle = knob_image_rle[knob_image_frame];
    for (i = 0u; i < knob_image_rle_sizes[knob_image_frame]; i += 2u) {
        uint8_t shade = rle[i + 1u];
        for (run = 0u; run < rle[i]; run++, out++)
            if (shade)
                cv_px[(out / KNOB_IMAGE_WIDTH) * 240u + 150u + out % KNOB_IMAGE_WIDTH] =
                    swap16(knob_image_shade(knob_colors[knob_image_color], shade));
    }
}

static void draw_knob_settings(void)
{
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        const char *label = knob_setting_label(i);
        uint8_t value = knob_setting_value(i);
        char text[] = "K1 --- -";
        text[1] = (char)('1' + i);
        if (selected_instrument == 6u && cs80_preset) {
            char cs80_text[] = "K1 --- 000";
            cs80_text[1] = (char)('1' + i);
            cs80_text[3] = label[0];
            cs80_text[4] = label[1];
            cs80_text[5] = label[2];
            cs80_text[7] = (char)('0' + value / 100u);
            cs80_text[8] = (char)('0' + (value / 10u) % 10u);
            cs80_text[9] = (char)('0' + value % 10u);
            draw_text(150, 66 + (int32_t)i * 20, cs80_text, knob_colors[i]);
            continue;
        }
        text[3] = label[0];
        text[4] = label[1];
        text[5] = label[2];
        text[7] = (char)('0' + value);
        draw_text(150, 66 + (int32_t)i * 20, text, knob_colors[i]);
    }
}

static const char *const bass_sound_names[9] = {
    "MODEL", "NOMORE", "808", "REMOVED", "REMOVED", "REMOVED", "REMOVED",
    "ELECTRIC", "ACOUSTIC"
};

static const char *const bass_mode_labels[BASS_MODE_COUNT] = {
    "BASS: MODEL SUSTAIN", "BASS: MODEL NOTE",
    "BASS: ELECTRIC SUSTAIN", "BASS: ELECTRIC NOTE",
    "BASS: ACOUSTIC SUSTAIN", "BASS: ACOUSTIC NOTE",
    "BASS: 808 SUSTAIN", "BASS: 808 NOTE",
    "BASS: NOMORE SUSTAIN", "BASS: NOMORE NOTE"
};

static const char *const guitar_sound_names[4] = {
    "MODEL", "ELECTRIC", "NYLON", "ACOUSTIC"
};

static const char *const guitar_mode_labels[8] = {
    "GTR: MODEL SUSTAIN", "GTR: MODEL 1-SHOT",
    "GTR: ELECTRIC SUSTAIN", "GTR: ELECTRIC 1-SHOT",
    "GTR: NYLON SUSTAIN", "GTR: NYLON 1-SHOT",
    "GTR: ACOUSTIC SUSTAIN", "GTR: ACOUSTIC 1-SHOT"
};

static const char *sequence_sound_name(uint8_t instrument, uint8_t variant)
{
    if (instrument == 3u)
        return guitar_sound_names[variant < 4u ? variant : 0u];
    if (instrument == 4u)
        return bass_sound_names[variant < 9u ? variant : 0u];
    if (instrument == 5u)
        return drum_sample_labels[variant < ROSSFLM1_SAMPLE_COUNT ? variant : 0u];
    if (instrument == 6u)
        return variant && variant <= CS80_PATCHES ? cs80_patches[variant - 1u].name : "CS-50";
    if (instrument == 7u)
        return chord_instrument_names[variant < 5u ? variant : 0u];
    return instrument_names[instrument < INSTRUMENTS ? instrument : 0u];
}

static void sequence_draw_notice(void)
{
    if (sequence_notice == SEQUENCE_NOTICE_NOTE) {
        static const char *const pitches[12] = {
            "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
        };
        int32_t x = 6;
        if (sequence_notice_instrument != 5u) {
            int32_t octave = (int32_t)sequence_notice_note / 12 - 1;
            char octave_text[] = "-1";
            if (octave >= 0) {
                octave_text[0] = (char)('0' + octave);
                octave_text[1] = 0;
            }
            x = cv_text(x, 42, &FONT_XS, "NOTE ", C_GRAY);
            x = cv_text(x, 42, &FONT_XS, pitches[sequence_notice_note % 12u], C_HI);
            x = cv_text(x, 42, &FONT_XS, octave_text, C_HI);
            x = cv_text(x, 42, &FONT_XS, "  ", C_HI);
        } else {
            x = cv_text(x, 42, &FONT_XS, "LM-1 ", C_GRAY);
        }
        cv_text(x, 42, &FONT_XS,
                sequence_sound_name(sequence_notice_instrument, sequence_notice_variant), C_HI);
    } else if (sequence_notice == SEQUENCE_NOTICE_OCTAVE) {
        char text[] = "OCTAVE +0";
        int32_t octave = sequence_notice_octave;
        text[7] = octave < 0 ? '-' : '+';
        text[8] = (char)('0' + (octave < 0 ? -octave : octave));
        draw_text(6, 42, text, C_HI);
    } else if (sequence_notice == SEQUENCE_NOTICE_FX) {
        char text[] = "K1 --- 000";
        text[1] = (char)('1' + sequence_notice_knob);
        for (uint32_t i = 0; i < 3u; i++)
            text[3u + i] = sequence_notice_fx_label[i];
        text[7] = (char)('0' + sequence_notice_value / 100u);
        text[8] = (char)('0' + (sequence_notice_value / 10u) % 10u);
        text[9] = (char)('0' + sequence_notice_value % 10u);
        draw_text(6, 42, text, knob_colors[sequence_notice_knob]);
    }
}

static uint32_t sequence_view_bar(void)
{
    uint32_t step_index = sequence_playing && sequence_follow_playback
        ? sequence_audio_step : sequence_cursor_step;
    return step_index / sequence_steps_per_bar[sequence.header.meter];
}

static void sequence_draw_grid(void)
{
    static const char *const track_names[INSTRUMENTS] = {
        "JUNO", "PIANO", "PRO-1", "GUITAR", "BASS", "LM-1", "CS-50", "CHORD"
    };
    static const uint16_t colors[SEQUENCE_TRACKS] = {
        RGB(80, 220, 255), RGB(120, 230, 120), RGB(255, 140, 180), RGB(255, 190, 70),
        RGB(190, 150, 255)
    };
    uint32_t steps_per_bar = sequence_steps_per_bar[sequence.header.meter];
    uint32_t bar = sequence_view_bar();
    uint32_t first_step = bar * steps_per_bar;
    int32_t x0 = 43, width = 193, row_height = 16, row_gap = 20;
    float cell_width = (float)width / (float)steps_per_bar;
    char bar_text[] = "BAR 000/128";
    uint32_t shown_bar = bar + 1u, total_bars = sequence.header.bars;
    bar_text[4] = (char)('0' + shown_bar / 100u);
    bar_text[5] = (char)('0' + (shown_bar / 10u) % 10u);
    bar_text[6] = (char)('0' + shown_bar % 10u);
    bar_text[8] = (char)('0' + total_bars / 100u);
    bar_text[9] = (char)('0' + (total_bars / 10u) % 10u);
    bar_text[10] = (char)('0' + total_bars % 10u);
    draw_text(176, 12, bar_text, C_GRAY);
    for (uint32_t track = 0; track < SEQUENCE_TRACKS; track++) {
        int32_t y = 33 + (int32_t)track * row_gap;
        uint16_t color = colors[track];
        uint8_t instrument = track == SEQUENCE_DRUM_TRACK ? 5u : sequence.header.instrument[track];
        draw_text(4, y + 4, track_names[instrument < INSTRUMENTS ? instrument : 0u],
                  track == sequence_cursor_track ? C_WHITE : C_GRAY);
        cv_rect(x0, y, width, row_height, dim_color(color, track == sequence_cursor_track ? 34u : 18u));
        cv_line(x0, y, x0 + width - 1, y, color);
        cv_line(x0, y + row_height - 1, x0 + width - 1, y + row_height - 1, color);
        cv_line(x0, y, x0, y + row_height - 1, color);
        cv_line(x0 + width - 1, y, x0 + width - 1, y + row_height - 1, color);
        for (uint32_t step_index = 1u; step_index < steps_per_bar; step_index++) {
            int32_t x = x0 + (int32_t)(cell_width * (float)step_index);
            if (step_index % 4u == 0u) {
                uint16_t accent = dim_color(color, 78u);
                cv_line(x, y + 1, x, y + row_height - 2, accent);
                cv_line(x + 1, y + 1, x + 1, y + row_height - 2, accent);
            } else {
                cv_line(x, y + 1, x, y + row_height - 2, dim_color(color, 55u));
            }
        }
    }
    for (uint32_t i = 0; i < sequence.header.count; i++) {
        uint32_t event = sequence.events[i];
        uint32_t event_step = sequence_event_step(event);
        uint32_t track = sequence_event_track(event);
        int32_t x, y, w;
        y = 33 + (int32_t)track * row_gap + 3;
        if (track == SEQUENCE_DRUM_TRACK) {
            float start = (float)((int32_t)event_step - (int32_t)first_step);
            float end = start + sequence_event_length_visual(event, sequence.header.tempo);
            int32_t start_px, end_px;
            if (start >= (float)steps_per_bar || end <= 0.0f)
                continue;
            if (start < 0.0f)
                start = 0.0f;
            if (end > (float)steps_per_bar)
                end = (float)steps_per_bar;
            start_px = (int32_t)(cell_width * start);
            end_px = (int32_t)(cell_width * end);
            x = x0 + start_px;
            w = end_px - start_px;
        } else {
            uint32_t bar_step;
            if (event_step < first_step || event_step >= first_step + steps_per_bar)
                continue;
            bar_step = event_step - first_step;
            x = x0 + (int32_t)(cell_width * (float)bar_step) + 1;
            w = (int32_t)(cell_width * sequence_event_length_visual(event, sequence.header.tempo)) - 2;
        }
        if (w < 2)
            w = 2;
        if (x + w > x0 + width - 1)
            w = x0 + width - 1 - x;
        if (w > 0)
            cv_rect(x, y, w, 10, colors[track]);
    }
    if ((!sequence_playing || !sequence_follow_playback)
        && sequence_cursor_step / steps_per_bar == bar && ((plat_ms() / 300u) & 1u) == 0u) {
        uint32_t cursor_bar_step = sequence_cursor_step % steps_per_bar;
        int32_t x = x0 + (int32_t)(cell_width * (float)cursor_bar_step);
        int32_t y = 33 + (int32_t)sequence_cursor_track * row_gap;
        cv_rect(x - 1, y - 2, 3, row_height + 4, C_WHITE);
    }
    if (sequence_playing && sequence_audio_step / steps_per_bar == bar) {
        uint32_t play_bar_step = sequence_audio_step % steps_per_bar;
        uint32_t play_track = sequence_cursor_track;
        int32_t x = x0 + (int32_t)(cell_width * (float)play_bar_step);
        int32_t y = 33 + (int32_t)play_track * row_gap;
        cv_rect(x - 1, y - 2, 3, row_height + 4, C_HI);
    }
    if (sequence_warning && ((plat_ms() / 250u) & 1u) == 0u) {
        const char *message = sequence_warning == 1u ? "TOO MANY SIMULTANEOUS SOUNDS, SNAKE"
            : sequence_warning == 2u ? "SEQUENCE IS FULL"
            : sequence_warning == 3u ? "ENTER A TRACK NAME"
            : sequence_warning == 5u ? "NO SAVED TRACKS" : "SAVE FAILED";
        int32_t chars = 0;
        for (const char *p = message; *p; p++)
            chars++;
        int32_t x = (240 - chars * 6) / 2;
        cv_rect(24, 76, 192, 14, C_BLACK);
        draw_text(x > 4 ? x : 4, 79, message, RGB(255, 45, 45));
    }
}

static void sequence_draw_info(void)
{
    int32_t event_index = sequence_find_event(sequence_cursor_track, sequence_cursor_step);
    uint8_t instrument = selected_instrument;
    uint8_t variant = sequence_current_variant();
    uint8_t duration = sequence_duration_index;
    const char *name;
    char tempo_text[] = "BPM 000";
    char length_text[] = "0000 MS";
    uint8_t tempo = sequence.header.tempo;
    if (event_index >= 0) {
        uint32_t event = sequence.events[event_index];
        instrument = sequence_event_instrument(event);
        variant = sequence_event_variant(event);
        duration = sequence_event_duration(event);
    }
    name = sequence_sound_name(instrument, variant);
    cv_begin(240, 96, C_BLACK);
    draw_text(6, 12, "SOUND", C_GRAY);
    draw_text(46, 12, name, C_WHITE);
    draw_text(6, 26, "LENGTH", C_GRAY);
    if (sequence_cursor_track == SEQUENCE_DRUM_TRACK) {
        uint32_t samples = drum_sample_lengths[variant];
        uint32_t ms;
        if (event_index >= 0 && duration != SEQUENCE_DRUM_NATURAL) {
            uint32_t limit = (uint32_t)((float)sequence_note_steps[duration] * 661500.0f / (float)tempo);
            if (samples > limit)
                samples = limit;
        }
        ms = (samples * 10u + 220u) / 441u;
        if (ms > 9999u)
            ms = 9999u;
        length_text[0] = ms >= 1000u ? (char)('0' + ms / 1000u) : ' ';
        length_text[1] = ms >= 100u ? (char)('0' + (ms / 100u) % 10u) : ' ';
        length_text[2] = ms >= 10u ? (char)('0' + (ms / 10u) % 10u) : ' ';
        length_text[3] = (char)('0' + ms % 10u);
        draw_text(52, 26, length_text, C_HI);
    } else {
        draw_text(52, 26, sequence_note_names[duration < 7u ? duration : 0u], C_HI);
    }
    sequence_draw_notice();
    draw_text(6, 56, "PRESET: TRACK   ALGO: STEP   REC: ADD/DEL", C_DIM);
    draw_text(6, 78, "GLO=HELP", C_HI);
    draw_text(101, 78, sequence_meter_names[sequence.header.meter], C_HI);
    tempo_text[4] = (char)('0' + tempo / 100u);
    tempo_text[5] = (char)('0' + (tempo / 10u) % 10u);
    tempo_text[6] = (char)('0' + tempo % 10u);
    draw_text(150, 78, tempo_text, C_HI);
    cv_blit(0, 144);
}

static void sequence_draw_help(void)
{
    static const char *const main_lines[4][4] = {
        {"VERSION " APP_VERSION, "SELECT: SOUND", "KEYS: PLAY NOTES", "OCT: SHIFT PITCH"},
        {"SEL: SOUND VARIANT", "K1-K4: SOUND FX", "ENV: MUTE CHORDS", "LFO: MUTE BASS"},
        {"ARP: ARPEGGIATOR", "EDIT+K1: TEMPO", "SAVE: STORE TEMPO", "PLAY OPENS PLAYBACK"},
        {"SEQ OPENS SEQUENCER", "", "", ""}
    };
    static const char *const playback_lines[2][4] = {
        {"PRESET: PICK SONG", "PLAY: START / STOP", "REC: DELETE SONG", "KEY: LOAD TO EDIT"},
        {"FIRST KEY: IGNORED", "SAVE: UPDATE SONG", "HOME: EXIT PLAYBACK", "PRESET: NEXT SONG"}
    };
    static const char *const sequence_lines[4][4] = {
        {"PRESET: TRACK", "ALGO: STEP", "SELECT: SOUND", "MASTER: LENGTH"},
        {"KEYS: SET NOTE", "REC: ADD / DEL", "OCT +/-: OCTAVE", "K1-K4: FX"},
        {"EDIT+K1: BPM", "SEL: METER", "PLAY: START / STOP", "SAVE: STORE"},
        {"PLAYBACK: PICK SONG", "KEY: LOAD TO EDIT", "FIRST KEY: IGNORED", "SAVE: UPDATE SONG"}
    };
    char main_title[] = "MAIN HELP 1/4";
    char playback_title[] = "PLAYBACK HELP 1/2";
    char sequence_title[] = "SEQUENCER HELP 1/4";
    const char *const (*lines)[4];
    char *title;
    int32_t title_x;
    uint8_t title_page_index;
    if (sequence_help_return_screen == SCREEN_SEQUENCE) {
        lines = sequence_lines;
        title = sequence_title;
        title_x = 55;
        title_page_index = 15u;
    } else if (sequence_help_return_screen == SCREEN_PLAYBACK) {
        lines = playback_lines;
        title = playback_title;
        title_x = 61;
        title_page_index = 14u;
    } else {
        lines = main_lines;
        title = main_title;
        title_x = 80;
        title_page_index = 10u;
    }
    title[title_page_index] = (char)('1' + sequence_help_page);
    cv_begin(240, 144, C_BLACK);
    draw_text(title_x, 4, title, C_WHITE);
    for (uint32_t i = 0; i < 4u; i++)
        cv_text(8, 24 + (int32_t)i * 28, &FONT_M, lines[sequence_help_page][i], C_GRAY);
    cv_blit(0, 0);
    cv_begin(240, 96, C_BLACK);
    cv_text(8, 6, &FONT_M, "ANY KNOB: PAGE", C_HI);
    draw_text(8, 58, "GLO, SEQ OR HOME: RETURN", C_GRAY);
    cv_blit(0, 144);
}

static void sequence_draw_name(void)
{
    char name[SEQUENCE_NAME_LENGTH];
    for (uint32_t i = 0; i < SEQUENCE_NAME_LENGTH; i++)
        name[i] = sequence.header.name[i];
    name[SEQUENCE_NAME_LENGTH - 1u] = 0;
    cv_begin(240, 144, C_BLACK);
    draw_text(84, 14, "SAVE TRACK", C_WHITE);
    cv_text(26, 54, &FONT_M, name, C_HI);
    if ((plat_ms() / 300u) & 1u)
        cv_rect(26 + (int32_t)sequence_name_cursor * 12, 78, 10, 2, C_WHITE);
    draw_text(10, 98, "PRESET: LETTER / NUMBER", C_GRAY);
    draw_text(10, 112, "ALGO: MOVE   SEL: DELETE", C_GRAY);
    if (sequence_warning && ((plat_ms() / 250u) & 1u) == 0u)
        draw_text(65, 130, sequence_warning == 3u ? "ENTER A TRACK NAME" : "SAVE FAILED",
                  RGB(255, 45, 45));
    cv_blit(0, 0);
    cv_begin(240, 96, C_BLACK);
    draw_text(12, 72, "SAVE: STORE   HOME OR SEQ: CANCEL", C_HI);
    cv_blit(0, 144);
}

static void sequence_draw_delete(void)
{
    uint32_t selected_ordinal = 0u, total = 0u, first;
    for (uint32_t i = 0; i < SEQUENCE_USER_SLOTS; i++) {
        if (!sequence_slots[i].used)
            continue;
        if (i == sequence_selected_slot)
            selected_ordinal = total;
        total++;
    }
    first = selected_ordinal > 4u ? selected_ordinal - 4u : 0u;
    if (first + 8u > total)
        first = total > 8u ? total - 8u : 0u;
    cv_begin(240, 144, C_BLACK);
    draw_text(65, 8, "DELETE A TRACK", C_WHITE);
    draw_text(18, 30, "PRESET: CHOOSE TRACK", C_GRAY);
    for (uint32_t i = 0, ordinal = 0; i < SEQUENCE_USER_SLOTS; i++) {
        if (!sequence_slots[i].used)
            continue;
        if (ordinal >= first && ordinal < first + 8u) {
            int32_t y = 35 + (int32_t)(ordinal - first) * 13;
            if (i == sequence_selected_slot)
                cv_rect(12, y - 1, 210, 13, RGB(70, 70, 90));
            draw_text(18, y, sequence_slots[i].name,
                      i == sequence_selected_slot ? C_WHITE : C_GRAY);
        }
        ordinal++;
    }
    if (sequence_warning && ((plat_ms() / 250u) & 1u) == 0u)
        draw_text(78, 130, "SAVE FAILED", RGB(255, 45, 45));
    cv_blit(0, 0);
    cv_begin(240, 96, C_BLACK);
    draw_text(12, 72, "SEL: DELETE   HOME: CANCEL", C_HI);
    cv_blit(0, 144);
}

static void sequence_draw_playback(void)
{
    uint32_t selected_ordinal = 0u, total = 0u, first;
    for (uint32_t i = 0; i < SEQUENCE_SLOTS; i++) {
        if (!sequence_slots[i].used)
            continue;
        if (i == sequence_selected_slot)
            selected_ordinal = total;
        total++;
    }
    first = selected_ordinal > 4u ? selected_ordinal - 4u : 0u;
    if (first + 8u > total)
        first = total > 8u ? total - 8u : 0u;
    cv_begin(240, 144, C_BLACK);
    draw_text(82, 6, "PLAYBACK MODE", C_WHITE);
    if (!sequence_slot_count) {
        draw_text(38, 56, "NO SAVED TRACKS", C_GRAY);
    } else {
        draw_text(18, 22, sequence_playing ? "PLAYING" : "PRESET: CHOOSE A TRACK", C_GRAY);
        for (uint32_t i = 0, ordinal = 0; i < SEQUENCE_SLOTS; i++) {
            if (!sequence_slots[i].used)
                continue;
            if (ordinal >= first && ordinal < first + 8u) {
                int32_t y = 31 + (int32_t)(ordinal - first) * 14;
                if (i == sequence_selected_slot)
                    cv_rect(12, y - 1, 210, 14, RGB(70, 70, 90));
                draw_text(18, y, sequence_slot_name(i),
                          i == sequence_selected_slot ? C_WHITE : C_GRAY);
            }
            ordinal++;
        }
    }
    if (sequence_warning && ((plat_ms() / 250u) & 1u) == 0u)
        draw_text(48, 120, sequence_warning == 5u ? "NO SAVED TRACKS"
                  : sequence_warning == 6u ? "DELETE FAILED"
                  : sequence_warning == 7u ? "BUILT-IN SAVE FAILED" : "TRACK LOAD FAILED",
                  RGB(255, 45, 45));
    cv_blit(0, 0);
    cv_begin(240, 96, C_BLACK);
    if (sequence_audio_events && sequence_selected_slot == SEQUENCE_USER_SLOTS) {
        char cpu_text[] = "PEAK CPU 000%";
        char xrun_text[] = "XRUN 000";
        uint32_t cpu = sequence_escape_cpu_peak;
        uint32_t xruns = plat_xruns() - sequence_escape_xruns_start;
        if (cpu > 999u)
            cpu = 999u;
        if (xruns > 999u)
            xruns = 999u;
        cpu_text[9] = (char)('0' + cpu / 100u);
        cpu_text[10] = (char)('0' + (cpu / 10u) % 10u);
        cpu_text[11] = (char)('0' + cpu % 10u);
        xrun_text[5] = (char)('0' + xruns / 100u);
        xrun_text[6] = (char)('0' + (xruns / 10u) % 10u);
        xrun_text[7] = (char)('0' + xruns % 10u);
        draw_text(6, 42, cpu_text, C_GRAY);
        draw_text(145, 42, xrun_text, C_GRAY);
    }
    draw_text(6, 70, "PLAY START/STOP  REC DELETE", C_HI);
    draw_text(6, 82, "PRESET SELECT  KEY EDIT  HOME EXIT", C_GRAY);
    draw_text(180, 70, "GLO: HELP", C_HI);
    cv_blit(0, 144);
}

static void sequence_draw(void)
{
    cv_begin(240, 144, C_BLACK);
    draw_text(94, 8, "SEQUENCER", C_WHITE);
    sequence_draw_grid();
    cv_blit(0, 0);
    sequence_draw_info();
}

void app_frame(void)
{
    static uint32_t sequence_last_draw;
    uint32_t i;
    uint32_t now;
    char tempo_text[8], octave_text[] = "OCT +0";
    int32_t tempo;
    uint8_t playing;
    int redraw, drum_label_changed = 0;

    app_input_only();
    if (sequence_audio_events && sequence_playing) {
        uint32_t cpu = plat_cpu_pct();
        if (cpu > sequence_escape_cpu_peak)
            sequence_escape_cpu_peak = cpu;
    }
#ifdef HAVE_SONG
    if (requested_escape_play) {
        if (!ui_escape_play) {
            app_splash();
            draw_escape_prompt();
            ui_escape_play = 1u;
        }
        ui_drawn = 0u;
        ui_drum_sample_visible = 0u;
        plat_lights(1u << B_PLAY, 0u, 0u, 0u, 1);
        return;
    }
    if (ui_escape_play) {
        ui_escape_play = 0u;
        ui_drawn = 0u;
        if ((int32_t)(plat_ms() - startup_splash_until) < 0)
            app_splash();
    }
#endif
    if ((int32_t)(plat_ms() - startup_splash_until) < 0) {
        splash_sweep(STARTUP_MS - (uint32_t)(startup_splash_until - plat_ms()));
        return;
    }
    splash_sweep_frame = 0xFFFFu;
    if (sequence_screen != SCREEN_MAIN) {
        now = plat_ms();
        if (sequence_warning && (int32_t)(now - sequence_warning_until) >= 0) {
            sequence_warning = 0u;
            ui_drawn = 0u;
        }
        if (sequence_notice && (int32_t)(now - sequence_notice_until) >= 0) {
            sequence_notice = SEQUENCE_NOTICE_NONE;
            ui_drawn = 0u;
        }
        if (!ui_drawn || now - sequence_last_draw >= 100u) {
            if (sequence_screen == SCREEN_SEQUENCE)
                sequence_draw();
            else if (sequence_screen == SCREEN_HELP)
                sequence_draw_help();
            else if (sequence_screen == SCREEN_NAME)
                sequence_draw_name();
            else if (sequence_screen == SCREEN_DELETE)
                sequence_draw_delete();
            else
                sequence_draw_playback();
            ui_drawn = 1u;
            sequence_last_draw = now;
        }
        {
            uint32_t lights = 0u;
            if (sequence_screen == SCREEN_SEQUENCE || sequence_screen == SCREEN_NAME
                || sequence_screen == SCREEN_DELETE
                || (sequence_screen == SCREEN_HELP && sequence_help_return_screen == SCREEN_SEQUENCE))
                lights |= 1u << B_SEQ;
            if (sequence_screen == SCREEN_HELP && sequence_help_return_screen == SCREEN_MAIN)
                lights |= 1u << B_GLO;
            if (sequence_playing || sequence_screen == SCREEN_PLAYBACK
                || (sequence_screen == SCREEN_HELP && sequence_help_return_screen == SCREEN_PLAYBACK))
                lights |= 1u << B_PLAY;
            if (sequence_screen == SCREEN_SEQUENCE
                && sequence_find_event(sequence_cursor_track, sequence_cursor_step) >= 0)
                lights |= 1u << B_REC;
            if (sequence_screen == SCREEN_PLAYBACK && sequence_slot_count
                && sequence_selected_slot < SEQUENCE_SLOTS
                && sequence_slots[sequence_selected_slot].used)
                lights |= 1u << B_REC;
            if (sequence_screen == SCREEN_NAME)
                lights |= 1u << B_SAVE;
            plat_lights(lights, sequence_screen == SCREEN_SEQUENCE ? previous_keys : 0u, 0u, 0u, 1);
        }
        return;
    }
    now = plat_ms();
    if (ui_drum_sample_event != drum_sample_event) {
        ui_drum_sample_event = drum_sample_event;
        ui_drum_sample = last_drum_sample;
        ui_drum_sample_until = now + 800u;
        ui_drum_sample_visible = 1u;
        drum_label_changed = 1;
    } else if (ui_drum_sample_visible && (int32_t)(now - ui_drum_sample_until) >= 0) {
        ui_drum_sample_visible = 0u;
        drum_label_changed = 1;
    }
    tempo = requested_tempo;
    playing = requested_play;
    redraw = drum_label_changed || !ui_drawn || battery_brightness() != ui_jc_brightness
        || tempo != ui_tempo || playing != ui_playing || requested_seq != ui_seq || save_failed != ui_save_failed
        || portrait_color != ui_portrait_color
        || portrait_color_visible != ui_portrait_color_visible
        || knob_image_visible != ui_knob_image_visible
        || knob_image_frame != ui_knob_image_frame || knob_image_color != ui_knob_image_color
        || selected_instrument != ui_instrument || chord_voice != ui_chord_voice
                || chord_layout != ui_chord_layout || cs80_preset != ui_cs80_preset
                || (uint8_t)(phrase_state | (phrase_count ? 4u : 0u)) != ui_phrase_state
        || instrument_octave[selected_instrument] != ui_octave
        || drum_bass_boost != ui_drum_bass_boost || bass_mode_index() != ui_bass_mode
        || guitar_mode_index() != ui_guitar_mode
        || drum_chorus != ui_drum_chorus
        || drum_reverb != ui_drum_reverb || drum_compression != ui_drum_compression;
    for (i = 0; i < TRACKS; i++) {
        uint32_t count = hits[i] > 8u ? 8u : hits[i];
        if (count != ui_hits[i])
            redraw = 1;
    }
    for (i = 0; i < 4u; i++)
        if ((selected_instrument == 3u || selected_instrument == 4u)
            && ui_string_controls[selected_instrument - 3u][i] != string_controls[selected_instrument - 3u][i])
            redraw = 1;
        else if (selected_instrument == 7u && ui_chord_controls[i] != chord_controls[i])
            redraw = 1;
        else if (selected_instrument == 6u && cs80_preset
                 && ui_cs80_controls[i] != cs80_controls[i])
            redraw = 1;
        else if (selected_instrument != 3u && selected_instrument != 4u && selected_instrument != 5u
                 && selected_instrument != 7u && !(selected_instrument == 6u && cs80_preset)
                 && ui_voice_controls[selected_instrument][i] != voice_controls[selected_instrument][i])
            redraw = 1;
    if (redraw) {
        tempo_text[0] = (char)('0' + tempo / 100);
        tempo_text[1] = (char)('0' + (tempo / 10) % 10);
        tempo_text[2] = (char)('0' + tempo % 10);
        tempo_text[3] = 0;
        cv_begin(240, 144, C_BLACK);
#ifdef PLISSKEN_EMULATOR_ONLY
        draw_text(12, 12, "EMU ONLY", C_WHITE);
#else
        draw_text(12, 12, APP_NAME, C_WHITE);
#endif
        draw_text(172, 12, "BPM", C_GRAY);
        draw_text(202, 12, tempo_text, C_HI);
        draw_text(12, 38, playing ? "PLAYING  7 PARTS" : "STOPPED  7 PARTS", playing ? C_HI : C_GRAY);
        draw_jc_face();
        if (requested_seq)
            draw_text(108, 76, "SEQ", C_HI);
        ui_jc_brightness = battery_brightness();

        for (i = 0; i < 4u; i++) {
            int32_t y = 66 + (int32_t)i * 20;
            uint16_t color = selected_instrument == i ? RGB(80, 220, 255) : C_WHITE;
            draw_text(18, y, track_names[i], color);
            if (hits[i]) {
                uint32_t bars = hits[i] > 8u ? 8u : hits[i];
                cv_rect(96, y + 2, (int32_t)bars * 5, 7, C_GRAY);
            }
        }
        draw_knob_settings();
        cv_blit(0, 0);
        cv_begin(240, 96, C_BLACK);
        for (i = 4u; i < TRACKS; i++) {
            int32_t y = 1 + (int32_t)(i - 4u) * 15;
            uint16_t color = selected_instrument == i ? RGB(80, 220, 255) : C_WHITE;
            draw_text(18, y, track_names[i], color);
            if (hits[i]) {
                uint32_t bars = hits[i] > 8u ? 8u : hits[i];
                cv_rect(96, y + 2, (int32_t)bars * 5, 7, i == 5u ? C_HI : C_GRAY);
            }
        }
        draw_text(18, 46, "CHORD", selected_instrument == 7u ? RGB(80, 220, 255) : C_WHITE);
        draw_knob_image();
        draw_text(12, 63, "KEYS:", C_GRAY);
        draw_text(48, 63, selected_instrument == 7u
                  ? chord_instrument_names[chord_voice]
                  : selected_instrument == 6u && cs80_preset
                    ? cs80_patches[cs80_preset - 1u].name
                    : selected_instrument == 1u && piano_sampled
                      ? "PIANO SAMPLE" : instrument_names[selected_instrument], C_HI);
        if (selected_instrument != 5u && selected_instrument != 7u) {
            int32_t octave = instrument_octave[selected_instrument];
            octave_text[4] = octave < 0 ? '-' : '+';
            octave_text[5] = (char)('0' + (octave < 0 ? -octave : octave));
            draw_text(158, 63, octave_text, C_HI);
        }
        draw_text(12, 78, plat_audio_status() ? "AUDIO INIT FAILED" :
                  ui_drum_sample_visible ? drum_sample_labels[ui_drum_sample] :
                  save_failed ? "SAVE FAILED" :
                  selected_instrument == 1u
                    ? (piano_sampled ? "PIANO: SAMPLE" : "PIANO: SYNTH") :
                  selected_instrument == 3u ? guitar_mode_labels[guitar_mode_index()] :
                  selected_instrument == 4u ? bass_mode_labels[bass_mode_index()] :
                  selected_instrument == 6u ? "PRESET: PATCH" :
                  selected_instrument == 7u ? (chord_layout ? "ALGO: SPLIT" : "ALGO: 1 KEY") : "EDIT+K1: BPM  SAVE: STORE",
                  plat_audio_status() || save_failed ? C_HI : C_GRAY);
        if (phrase_state != PHRASE_IDLE)
            draw_text(204, 63, phrase_state == PHRASE_REC ? "REC" : "LOOP",
                      phrase_state == PHRASE_REC ? RGB(255, 90, 90) : C_HI);
        else if (phrase_count)
            draw_text(204, 63, "PHR", C_GRAY);
        draw_text(178, 82, "GLO: HELP", C_HI);
        cv_blit(0, 144);
        ui_tempo = tempo;
        ui_phrase_state = (uint8_t)(phrase_state | (phrase_count ? 4u : 0u));
        ui_instrument = selected_instrument;
        ui_chord_voice = chord_voice;
        ui_chord_layout = chord_layout;
        ui_cs80_preset = cs80_preset;
        ui_octave = instrument_octave[selected_instrument];
        ui_drum_bass_boost = drum_bass_boost;
        ui_bass_mode = bass_mode_index();
        ui_guitar_mode = guitar_mode_index();
        ui_drum_chorus = drum_chorus;
        ui_drum_reverb = drum_reverb;
        ui_drum_compression = drum_compression;
        for (i = 0; i < 2u; i++)
            for (uint32_t k = 0; k < 4u; k++)
                ui_string_controls[i][k] = string_controls[i][k];
        for (i = 0; i < TRACKS; i++)
            for (uint32_t k = 0; k < 4u; k++)
                ui_voice_controls[i][k] = voice_controls[i][k];
        for (i = 0; i < 4u; i++)
            ui_chord_controls[i] = chord_controls[i];
        for (i = 0; i < 4u; i++)
            ui_cs80_controls[i] = cs80_controls[i];
        ui_playing = playing;
        ui_seq = requested_seq;
        ui_save_failed = save_failed;
        ui_knob_image_visible = knob_image_visible;
        ui_knob_image_frame = knob_image_frame;
        ui_knob_image_color = knob_image_color;
        ui_portrait_color = portrait_color;
        ui_portrait_color_visible = portrait_color_visible;
        for (i = 0; i < TRACKS; i++)
            ui_hits[i] = hits[i] > 8u ? 8u : hits[i];
        ui_drawn = 1u;
    }
    plat_lights((playing ? (1u << B_PLAY) : 0u) | (phrase_state != PHRASE_IDLE ? (1u << B_REC) : 0u)
                | ((backing_mute & MUTE_CHORDS) ? (1u << B_ENV) : 0u)
                | ((backing_mute & MUTE_BASS) ? (1u << B_LFO) : 0u)
                | ((backing_mute & MUTE_DRUMS) ? (1u << B_GLO) : 0u)
                | (arp_on ? (1u << B_ARP) : 0u)
                | ((requested_escape_play || requested_seq) ? (1u << B_SEQ) : 0u),
                previous_keys, 0u, 0u, 1);
}

static int text_is(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

int app_expect(const char *what, int *got)
{
#ifdef PLAT_HOST
    if (text_is(what, "seq_stealing")) {
        *got = sequence_audio_stealing;
        return 1;
    }
    if (text_is(what, "seq_steals")) {
        *got = (int)sequence_steals;
        return 1;
    }
    if (text_is(what, "seq_steal_drops")) {
        *got = (int)sequence_steal_drops;
        return 1;
    }
    if (text_is(what, "seq_steal_peak")) {
        *got = (int)sequence_steal_peak;
        return 1;
    }
    if (text_is(what, "seq_steal_limit")) {
        *got = sequence_audio_steal_limit;
        return 1;
    }
    if (text_is(what, "seq_active_drums")) {
        *got = 0;
        for (uint32_t i = 0u; i < ROSSFLM1_SAMPLE_COUNT; i++)
            if (drum_samples[i].data || drum_samples[i].adpcm_data)
                (*got)++;
        return 1;
    }
    if (text_is(what, "seq_active_piano")) {
        *got = 0;
        for (uint32_t i = 0u; i < PIANO_VOICES; i++)
            if (piano[i].active)
                (*got)++;
        return 1;
    }
    if (text_is(what, "seq_mix_peak_milli")) {
        *got = (int)(sequence_mix_peak * 1000.0f);
        return 1;
    }
    if (text_is(what, "seq_mix_over_one")) {
        *got = (int)sequence_mix_over_one;
        return 1;
    }
    if (text_is(what, "seq_mix_peak_step")) {
        *got = (int)sequence_mix_peak_step;
        return 1;
    }
#endif
    if (text_is(what, "keys")) {
        *got = (int)previous_keys;
        return 1;
    }
    if (what[0] == 'h' && what[1] == 'i' && what[2] == 't' && what[3] == 's'
        && what[4] >= '0' && what[4] < '0' + TRACKS && what[5] == 0) {
        *got = (int)hits[what[4] - '0'];
        return 1;
    }
    if (what[0] == 's' && what[1] == 'o' && what[2] == 'l' && what[3] == 'o' && what[4] >= '0'
        && what[4] <= '9' && what[5] == 0) {
        solo_mask = what[4] < '0' + TRACKS ? 1u << (what[4] - '0') : 0x7Fu;
        *got = (int)solo_mask;
        return 1;
    }
    if (text_is(what, "seq_run")) {
        *got = requested_seq;
        return 1;
    }
    if (text_is(what, "seq_screen")) {
        *got = sequence_screen == SCREEN_SEQUENCE;
        return 1;
    }
    if (text_is(what, "seq_help")) {
        *got = sequence_screen == SCREEN_HELP;
        return 1;
    }
    if (text_is(what, "main_help")) {
        *got = sequence_screen == SCREEN_HELP && sequence_help_return_screen == SCREEN_MAIN;
        return 1;
    }
    if (text_is(what, "playback_help")) {
        *got = sequence_screen == SCREEN_HELP && sequence_help_return_screen == SCREEN_PLAYBACK;
        return 1;
    }
    if (text_is(what, "seq_help_page")) {
        *got = sequence_help_page;
        return 1;
    }
    if (text_is(what, "playback_mode")) {
        *got = sequence_screen == SCREEN_PLAYBACK;
        return 1;
    }
    if (text_is(what, "seq_track")) {
        *got = sequence_cursor_track;
        return 1;
    }
    if (text_is(what, "seq_cursor")) {
        *got = sequence_cursor_step;
        return 1;
    }
    if (text_is(what, "seq_view_bar")) {
        *got = (int)sequence_view_bar();
        return 1;
    }
    if (text_is(what, "seq_follow")) {
        *got = sequence_follow_playback;
        return 1;
    }
    if (text_is(what, "seq_events")) {
        *got = sequence.header.count;
        return 1;
    }
    if (text_is(what, "seq_note")) {
        int32_t event_index = sequence_find_event(sequence_cursor_track, sequence_cursor_step);
        *got = event_index >= 0 ? sequence_event_note(sequence.events[event_index]) : -1;
        return 1;
    }
    if (text_is(what, "seq_event_duration")) {
        int32_t event_index = sequence_find_event(sequence_cursor_track, sequence_cursor_step);
        *got = event_index >= 0 ? sequence_event_duration(sequence.events[event_index]) : -1;
        return 1;
    }
    if (what[0] == 'd' && what[1] == 'r' && what[2] == 'u' && what[3] == 'm'
        && what[4] == '_' && what[5] == 'a' && what[6] == 'c' && what[7] == 't'
        && what[8] == 'i' && what[9] == 'v' && what[10] == 'e'
        && what[11] >= '0' && what[11] <= '9' && what[12] == 0) {
        *got = drum_samples[what[11] - '0'].data != 0;
        return 1;
    }
    if (text_is(what, "seq_notice")) {
        *got = sequence_notice;
        return 1;
    }
    if (text_is(what, "seq_notice_value")) {
        *got = sequence_notice_value;
        return 1;
    }
    if (text_is(what, "seq_notice_knob")) {
        *got = sequence_notice_knob;
        return 1;
    }
    if (text_is(what, "seq_duration")) {
        *got = sequence_duration_index;
        return 1;
    }
    if (text_is(what, "seq_meter")) {
        *got = sequence.header.meter;
        return 1;
    }
    if (text_is(what, "seq_bars")) {
        *got = sequence.header.bars;
        return 1;
    }
    if (text_is(what, "seq_playing")) {
        *got = sequence_playing;
        return 1;
    }
    if (text_is(what, "seq_tempo")) {
        *got = sequence.header.tempo;
        return 1;
    }
    if (text_is(what, "seq_playback_tempo")) {
        *got = sequence_playback_tempo;
        return 1;
    }
    if (text_is(what, "seq_audio_step")) {
        *got = (int)sequence_audio_step;
        return 1;
    }
    if (text_is(what, "seq_saved")) {
        *got = sequence_slot_count;
        return 1;
    }
    if (text_is(what, "seq_warning")) {
        *got = sequence_warning;
        return 1;
    }
    if (text_is(what, "seq_sound")) {
        *got = sequence_selected_sound;
        return 1;
    }
    if (text_is(what, "seq_instrument")) {
        *got = selected_instrument;
        return 1;
    }
    if (text_is(what, "seq_steps")) {
        int n = 0;
        for (uint32_t s = 0; s < 16u; s++)
            n += ((settings.seq_mask[0] | settings.seq_mask[1] | settings.seq_mask[2]) >> s & 1u)
                 || settings.seq_perc[s];
        *got = n;
        return 1;
    }
    if (text_is(what, "playing")) {
        *got = requested_play;
        return 1;
    }
    if (text_is(what, "escape_mode")) {
        *got = requested_escape_play;
        return 1;
    }
    if (text_is(what, "instrument")) {
        *got = selected_instrument;
        return 1;
    }
    if (text_is(what, "chord_voice")) {
        *got = chord_voice;
        return 1;
    }
    if (text_is(what, "chord_layout")) {
        *got = chord_layout;
        return 1;
    }
    if (text_is(what, "phrase_state")) { *got = phrase_state; return 1; }
    if (text_is(what, "phrase_count")) { *got = phrase_count; return 1; }
    if (text_is(what, "phrase_held")) { *got = (int)phrase_held; return 1; }
    if (text_is(what, "cs80_preset")) {
        *got = cs80_preset;
        return 1;
    }
    if (text_is(what, "octave")) {
        *got = instrument_octave[selected_instrument];
        return 1;
    }
    if (text_is(what, "drum_bass_boost")) {
        *got = drum_bass_boost;
        return 1;
    }
    if (text_is(what, "drum_sample")) {
        *got = last_drum_sample;
        return 1;
    }
    if (text_is(what, "bass_sound")) {
        *got = bass_sound;
        return 1;
    }
    if (text_is(what, "bass_mode")) { *got = bass_mode_index(); return 1; }
    if (text_is(what, "bass_one_shot")) { *got = bass_one_shot; return 1; }
    if (text_is(what, "jc_brightness")) { *got = battery_brightness(); return 1; }
    if (text_is(what, "guitar_sound")) { *got = guitar_sound; return 1; }
    if (text_is(what, "guitar_mode")) { *got = guitar_mode_index(); return 1; }
    if (text_is(what, "guitar_one_shot")) { *got = guitar_one_shot; return 1; }
    if (text_is(what, "backing_mute")) { *got = backing_mute; return 1; }
    if (text_is(what, "arp_on")) { *got = arp_on; return 1; }
    if (text_is(what, "drum_chorus")) { *got = drum_chorus; return 1; }
    if (text_is(what, "drum_reverb")) { *got = drum_reverb; return 1; }
    if (text_is(what, "drum_compression")) { *got = drum_compression; return 1; }
    if (what[0] == 'f' && what[1] == 'x' && what[2] >= '1' && what[2] <= '4' && !what[3]
        && (selected_instrument == 3u || selected_instrument == 4u)) {
        *got = string_controls[selected_instrument - 3u][what[2] - '1'];
        return 1;
    }
    if (what[0] == 'f' && what[1] == 'x' && what[2] >= '1' && what[2] <= '4' && !what[3]
        && selected_instrument == 7u) {
        *got = chord_controls[what[2] - '1'];
        return 1;
    }
    if (what[0] == 'f' && what[1] == 'x' && what[2] >= '1' && what[2] <= '4' && !what[3]
        && selected_instrument == 6u && cs80_preset) {
        *got = cs80_controls[what[2] - '1'];
        return 1;
    }
    if (what[0] == 'f' && what[1] == 'x' && what[2] >= '1' && what[2] <= '4' && !what[3]
        && selected_instrument != 5u) {
        *got = voice_controls[selected_instrument][what[2] - '1'];
        return 1;
    }
#ifdef PLAT_HOST
    if (text_is(what, "test_legacy_builtins") || text_is(what, "test_pretrim_builtins")) {
        int saved = 0;
        int legacy = text_is(what, "test_legacy_builtins");
        for (uint32_t i = 0u; i < SEQUENCE_BUILTIN_COUNT; i++) {
            sequence_prepare_builtin(i, legacy, 0);
            if (sequence_store_slot(SEQUENCE_USER_SLOTS + i) == 0)
                saved++;
        }
        sequence_reset();
        sequence_scan_slots();
        *got = saved;
        return 1;
    }
    if (text_is(what, "chord_piano_keys")) {
        uint32_t keys = 0u;
        for (uint32_t i = 0; i < PIANO_VOICES; i++)
            if (piano[i].key)
                keys |= 1u << (piano[i].key - 1u);
        *got = (int)keys;
        return 1;
    }
    if (text_is(what, "portrait_color")) {
        *got = portrait_color;
        return 1;
    }
    if (text_is(what, "portrait_color_visible")) {
        *got = portrait_color_visible;
        return 1;
    }
    if (text_is(what, "knob_image_frame")) {
        *got = knob_image_frame;
        return 1;
    }
    if (text_is(what, "knob_image_visible")) {
        *got = knob_image_visible;
        return 1;
    }
    if (text_is(what, "live_hz")) {
        *got = (int)(last_live_increment * SAMPLE_RATE + 0.5f);
        return 1;
    }
#endif
    if (text_is(what, "tempo")) {
        *got = requested_tempo;
        return 1;
    }
    if (text_is(what, "step")) {
        *got = (int)rendered_step;
        return 1;
    }
    if (text_is(what, "stereo")) {
        *got = stereo_diff_samples > 2147483647u ? 2147483647 : (int)stereo_diff_samples;
        return 1;
    }
    return 0;
}
