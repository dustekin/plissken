/* SPDX-License-Identifier: MPL-2.0 */
#include "fastmath.h"
#include "bass_samples.h"
#include "lm1_samples.h"
#include "startup_splash.h"
#include "jc_face.h"
#include "knob_images.h"

#define SETTINGS_MAGIC 0x504C4953u
#define TRACKS 7
#define INSTRUMENTS (TRACKS + 1u)
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
    uint32_t age;
    uint32_t duration;
    uint8_t active;
    uint8_t key;
} piano_t;

static uint32_t trigger_piano(int32_t note, uint32_t duration, uint32_t velocity);

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
    uint32_t length;
    float position;
    float rate;
    float gain;
} sample_voice_t;

typedef struct {
    uint32_t tick;
    uint16_t duration;
    uint8_t part;
    uint8_t note;
    uint8_t velocity;
} song_event_t;

#if defined(__has_include)
#if __has_include("dsp/song.h")
#include "dsp/song.h"
#define HAVE_SONG 1
#endif
#endif
#ifdef HAVE_SONG
#define DEFAULT_TEMPO SONG_BPM
#else
#define DEFAULT_TEMPO 108u
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
} settings_t;

static settings_t settings;
static volatile int32_t requested_tempo = DEFAULT_TEMPO;
static volatile uint8_t requested_play;
static volatile uint8_t requested_escape_play;
static volatile uint32_t hits[TRACKS];
static volatile uint32_t rendered_step;
static volatile uint32_t stereo_diff_samples;
static volatile uint8_t audio_playing;
static uint8_t audio_escape_play;
static uint32_t previous_buttons;
static uint32_t previous_keys;
static uint32_t startup_splash_until;
static int8_t live_key_voice[NKEYS];
static uint8_t live_key_instrument[NKEYS];
static uint8_t live_key_chord_voice[NKEYS];
static uint8_t selected_instrument;
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
static volatile uint8_t bass_sound;
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
static uint8_t ui_playing, ui_escape_play, ui_save_failed, ui_drawn, ui_drum_bass_boost;
static uint8_t ui_bass_sound;
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
static uint8_t startup_active;
static uint8_t startup_event;
static uint32_t startup_sample;
static const uint32_t startup_offsets[6] = {0u, 8269u, 16538u, 24806u, 33075u, 41344u};
static const uint32_t startup_durations[6] = {4134u, 4134u, 4134u, 5167u, 4134u, 4823u};
static const uint8_t startup_notes[6] = {52u, 59u, 64u, 64u, 71u, 76u};
static const char *const track_names[TRACKS] = {
    "JUNO", "PIANO", "PRO-1", "GUITAR", "BASS / SAMP B", "LM-1", "CS-50/CS-80"
};
static const char *const instrument_names[INSTRUMENTS] = {
    "JUNO", "PIANO", "PRO-1", "GUITAR", "BASS / SAMP B", "LM-1", "CS-50/CS-80", "CHORD"
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
    float feedback = (1.0f - g) * (g * g * g * voice->lp1 + g * g * voice->lp2 + g * voice->lp3 + voice->lp4);
    u = ladder_tanh((x * 0.5f - k * feedback) * voice->ladder_den);
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
    voice->length = length;
    voice->position = 0.0f;
    voice->rate = 1.0f;
    voice->gain = 1.0f;
}

static void trigger_drum_sample(uint32_t sample)
{
    sample %= ROSSFLM1_SAMPLE_COUNT;
    trigger_sample(&drum_samples[sample], drum_sample_data[sample], drum_sample_lengths[sample]);
    last_drum_sample = (uint8_t)sample;
    drum_sample_event++;
}

static float sample_voice(sample_voice_t *voice, float level)
{
    uint32_t position;
    float sample;
    if (!voice->data)
        return 0.0f;
    if (voice->position >= voice->length) {
        voice->data = 0;
        return 0.0f;
    }
    position = (uint32_t)voice->position;
    sample = (float)voice->data[position];
    if (voice->rate != 1.0f && position + 1u < voice->length) {
        float fraction = voice->position - (float)position;
        sample += ((float)voice->data[position + 1u] - sample) * fraction;
    }
    voice->position += voice->rate;
    return sample * (level * voice->gain / 128.0f);
}

static uint32_t trigger_bass_sample(int32_t note, uint32_t velocity)
{
    static const int32_t sample_octave_offset = -24;
    static const int8_t *const data[] =     {bass_nomore, bass_buzz};
    static const uint32_t lengths[] = {
            BASS_NOMORE_LENGTH, BASS_BUZZ_LENGTH
    };
    uint32_t index = bass_sample_next++ % MONO_VOICES;
    sample_voice_t *voice = &bass_sample_voices[index];
    uint32_t sample = (uint32_t)bass_sound - 1u;
    voice->data = data[sample];
    voice->length = lengths[sample];
    voice->position = 0.0f;
    voice->rate = fm_exp2f((float)(note - 36 + sample_octave_offset) * (1.0f / 12.0f));
    voice->gain = velocity_gain(velocity);
    return index;
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

    if ((s & 3u) == 0u) {
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

    if (s == 2u || s == 6u || s == 10u || s == 14u) {
        trigger_piano(piano_roots[(s >> 2) & 3u] + 12 + piano_melody[(s >> 2) & 3u], 12000u, 100u);
        hits[1]++;
    }
    if (pro_pattern[s] >= 0) {
        uint32_t index = trigger_mono(pro_one, PRO_ONE_VOICES, root + 12 + pro_pattern[s], 12000u, 600u);
        pro_one[index].velocity_gain = velocity_gain(90u);
        hits[2]++;
    }
    if (bass_pattern[s] >= 0) {
        trigger_bass_note(root - 24 + bass_pattern[s], 9000u, 90u);
        hits[4]++;
    }
    if (cs80_preset) {
        i = trigger_cs80(root + 12 + cs_pattern[s & 3u], 7000u, cs80_preset);
        cs80[i].velocity_gain = velocity_gain(90u);
    } else {
        i = trigger_pooled(cs50, CS50_VOICES, root + 12 + cs_pattern[s & 3u], 7000u);
        cs50[i].velocity_gain = velocity_gain(90u);
    }
    hits[6]++;
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
    rendered_step = s;
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
    voice->age = 0u;
    voice->duration = duration;
    voice->active = 1u;
    voice->key = 0u;
    return (uint32_t)(voice - piano);
}

#define CHORD_TYPES 8u
#define CHORD_SHAPE_KEY 12u

static uint32_t trigger_chord(int32_t root, uint32_t duration, uint32_t velocity, uint32_t key)
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
        uint8_t instrument = chord_instruments[chord_voice];
        if (instrument == 1u) {
            index = trigger_piano(notes[i], duration, velocity);
            piano[index].key = (uint8_t)(key + 1u);
        } else if (instrument == 0u) {
            index = trigger_pooled(juno, JUNO_VOICES, notes[i], duration);
            juno[index].key = (uint8_t)(key + 1u);
            juno[index].velocity_gain = velocity_gain(velocity);
        } else if (instrument == 6u) {
            index = trigger_pooled(cs50, CS50_VOICES, notes[i], duration);
            cs50[index].key = (uint8_t)(key + 1u);
            cs50[index].velocity_gain = velocity_gain(velocity);
        } else if (instrument == 2u) {
            index = trigger_pooled(pro_one, PRO_ONE_VOICES, notes[i], duration);
            pro_one[index].key = (uint8_t)(key + 1u);
            pro_one[index].velocity_gain = velocity_gain(velocity);
        } else {
            index = trigger_pooled(guitar, GUITAR_VOICES, notes[i], duration);
            guitar[index].key = (uint8_t)(key + 1u);
            guitar[index].velocity_gain = velocity_gain(velocity);
        }
        if (i == 0u)
            first = index;
    }
    live_key_chord_voice[key] = chord_instruments[chord_voice];
    return first;
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
    case 3u: voice = &guitar[index]; break;
    case 4u:
        if (live_key_bass_sample[key])
            live_key_bass_sample[key] = 0u;
        else
            voice = &bass[index];
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
        if (chord_layout && key >= CHORD_SHAPE_KEY) {
            /* Split layout: upper keys pick the chord type; keys beyond the last type are silent. */
            if (key - CHORD_SHAPE_KEY < CHORD_TYPES)
                chord_controls[0] = (uint8_t)(key - CHORD_SHAPE_KEY);
#ifdef PLAT_HOST
            last_live_increment = 0.0f;
#endif
            live_key_voice[key] = -1;
            return;
        }
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
    float sum = 0.0f, thump, remaining;
    float decay_scale = decay_scales[controls[1]];
    float brightness = 1.0f + 0.18f * (float)controls[0];
    uint32_t k;
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
    float increment, duty, saw, pulse, sub, x, amp, cutoff;
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
    saw = bandlimited_saw(voice->phase, increment);
    pulse = bandlimited_pulse(voice->phase, increment, duty);
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
    float increment, sub_increment, a, b, sub, x, amp, cutoff, vibrato;
    if (voice->age >= voice->duration) {
        voice->active = 0u;
        return 0.0f;
    }
    voice->age++;
    amp = voice->age < 30u ? (float)voice->age * (1.0f / 30.0f) : 1.0f;
    if (voice->duration - voice->age < 600u)
        amp *= (float)(voice->duration - voice->age) * (1.0f / 600.0f);
    vibrato = voice->age > 4000u ? 1.0f + 0.0035f * fm_sinf((float)voice->age * (5.5f / SAMPLE_RATE) * FM_TWO_PI) : 1.0f;
    if (controls[3])
        vibrato += 0.004f * (float)controls[3] * fm_sinf((float)voice->age * (5.5f / SAMPLE_RATE) * FM_TWO_PI);
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
        if (bass_sample_voices[i].data)
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
    }
    for (i = 0; i < GUITAR_VOICES; i++)
        guitar[i].active = 0u;
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
    requested_escape_play = 0u;
    audio_playing = 0u;
    audio_escape_play = 0u;
    settings.magic = SETTINGS_MAGIC;
    settings.tempo = DEFAULT_TEMPO;
    requested_tempo = DEFAULT_TEMPO;
    if (plat_store_load(OBJ_SETTINGS, &settings, sizeof settings) == (int)sizeof settings
        && settings.magic == SETTINGS_MAGIC && settings.tempo >= 70u && settings.tempo <= 140u)
        requested_tempo = settings.tempo;
    else {
        settings.magic = SETTINGS_MAGIC;
        settings.tempo = DEFAULT_TEMPO;
    }
    samples_per_step = 661500.0f / (float)requested_tempo;
    build_piano_table();
    build_tanh_table();
}

void app_boot(void)
{
    startup_requested = 1u;
    startup_splash_until = plat_ms() + 1500u;
}

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t i, j;
    uint8_t strings[2][4];
    uint8_t voice_settings[TRACKS][4];
    uint8_t cs80_settings[4];
    uint8_t piano_effects[4];
    int32_t tempo = requested_tempo;
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

    if (playing != audio_playing) {
        audio_playing = playing;
        if (playing) {
            step = 15u;
            samples_into_step = 0u;
        }
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
    if (!playing && !escape_play && !startup_active && !chorus_tail && !drum_effect_tail
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
        if (startup_active) {
            while (startup_event < 6u && startup_sample >= startup_offsets[startup_event]) {
                trigger_pooled(cs50, CS50_VOICES, startup_notes[startup_event],
                               startup_durations[startup_event]);
                startup_event++;
            }
            if (startup_event == 6u && startup_sample >= startup_offsets[5] + startup_durations[5])
                startup_active = 0u;
            startup_sample++;
        }
#ifdef HAVE_SONG
        if (escape_play)
            song_advance(2646000.0f / ((float)tempo * (float)SONG_PPQ));
        else
#endif
        if (playing) {
            if ((float)samples_into_step >= samples_per_step) {
                samples_into_step = 0u;
                step = (step + 1u) & 15u;
                trigger_step(step);
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
                juno_mix += juno_voice(&juno[j], 0.20f, voice_settings[0]);
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
        }
        for (j = 0; j < MONO_VOICES; j++) {
            if (bass[j].active)
                bass_mix += bass_voice(&bass[j], 0.7f);
            voices_active |= bass[j].active;
            if (bass_sample_voices[j].data)
                bass_mix += sample_voice(&bass_sample_voices[j], 0.7f);
            voices_active |= bass_sample_voices[j].data != 0;
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
    }
    for (i = 0; i < GUITAR_VOICES; i++)
        guitar[i].active = 0u;
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
    bass_sound = 0u;
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
}

void app_splash(void)
{
    uint32_t i, out = 0u, run;

    cv_begin(STARTUP_SPLASH_WIDTH, STARTUP_SPLASH_HEIGHT, C_BLACK);
    for (i = 0u; i < STARTUP_SPLASH_RLE_SIZE; i += 2u) {
        uint16_t color = swap16(startup_splash_palette[startup_splash_rle[i + 1u]]);
        for (run = 0u; run < startup_splash_rle[i]; run++)
            cv_px[out++] = color;
    }
    cv_blit(0, 0);
    lcd_fill(0, STARTUP_SPLASH_HEIGHT, STARTUP_SPLASH_WIDTH,
             240u - STARTUP_SPLASH_HEIGHT, C_BLACK);
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
static void record_knob_turn(uint8_t knob, int32_t delta);
static void record_portrait_turn(int32_t delta);

void app_input_only(void)
{
    uint32_t buttons = plat_buttons();
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
#ifdef HAVE_SONG
    if (pressed & (1u << B_PLAY)) {
        if (edit_held) {
            requested_escape_play = (uint8_t)!requested_escape_play;
            if (requested_escape_play) {
                requested_play = 0u;
                phrase_abort();
                for (uint32_t k = 0; k < NKEYS; k++)
                    release_live_key(k);
            }
        } else if (!requested_escape_play) {
            requested_play = (uint8_t)!requested_play;
        }
    }
#else
    if (pressed & (1u << B_PLAY))
        requested_play = (uint8_t)!requested_play;
#endif
#ifdef APP_TOUCH
    uint32_t keys;
    touch_poll();
    keys = touch_keys();
    if (requested_escape_play) {
        previous_keys = keys;
    } else {
        for (uint32_t k = 0; k < NKEYS; k++) {
            uint32_t bit = 1u << k;
            if ((keys & bit) && !(previous_keys & bit)) {
                uint32_t velocity = touch_vel(k);
                trigger_live_key(k, velocity);
                phrase_record(k, velocity, 1);
            } else if (!(keys & bit) && (previous_keys & bit)) {
                release_live_key(k);
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
    if (selected_instrument == 6u && edit_held && instrument_delta) {
        int32_t preset = ((int32_t)cs80_preset + instrument_delta) % (CS80_PATCHES + 1);
        if (preset < 0)
            preset += CS80_PATCHES + 1;
        cs80_preset = (uint8_t)preset;
        if (preset) {
            for (uint32_t k = 0; k < 4u; k++)
                cs80_controls[k] = cs80_patches[preset - 1].controls[k];
        }
    } else if (selected_instrument == 7u && edit_held && instrument_delta) {
        int32_t voice = ((int32_t)chord_voice + instrument_delta) % 5;
        if (voice < 0)
            voice += 5;
        chord_voice = (uint8_t)voice;
    } else if (instrument_delta) {
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
    if (selected_instrument == 4u && edit_held && boost_delta)
        bass_sound = (uint8_t)clamp((int32_t)bass_sound + boost_delta, 0, 2);
    if (selected_instrument == 3u || selected_instrument == 4u) {
        uint32_t part = selected_instrument - 3u;
        int32_t deltas[4] = {(edit_held ? 0 : delta), k2, k3,
                             (selected_instrument == 4u && edit_held) ? 0 : boost_delta};
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
        int32_t deltas[4] = {(edit_held ? 0 : delta), k2, k3, edit_held ? 0 : boost_delta};
        for (uint32_t k = 0; k < 4u; k++)
            if (deltas[k])
                chord_controls[k] = (uint8_t)clamp((int32_t)chord_controls[k] + deltas[k], 0,
                                                   k == 0u ? (int32_t)CHORD_TYPES - 1 : 3);
        if (edit_held && boost_delta)
            chord_layout = (uint8_t)clamp((int32_t)chord_layout + boost_delta, 0, 1);
    }
    if (selected_instrument == 6u && cs80_preset) {
        int32_t deltas[4] = {(edit_held ? 0 : delta), k2, k3, boost_delta};
        for (uint32_t k = 0; k < 4u; k++)
            if (deltas[k])
                cs80_controls[k] = (uint8_t)clamp((int32_t)cs80_controls[k] + deltas[k] * 4, 0, 127);
    }
    record_knob_turn(0u, ((selected_instrument == 4u || selected_instrument == 6u) && edit_held)
                     ? 0 : delta);
    record_knob_turn(1u, k2);
    record_knob_turn(2u, k3);
    record_knob_turn(3u, selected_instrument == 4u && edit_held ? 0 : boost_delta);
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

static void draw_jc_face(void)
{
    uint32_t i, out = 0u, run;
    uint32_t x = (240u - JC_FACE_WIDTH) / 2u;
    for (i = 0u; i < JC_FACE_RLE_SIZE; i += 2u) {
        uint8_t color = jc_face_rle[i + 1u];
        for (run = 0u; run < jc_face_rle[i]; run++, out++)
            if (color)
                cv_px[(out / JC_FACE_WIDTH) * 240u + x + out % JC_FACE_WIDTH] =
                    swap16(portrait_color_visible ? knob_image_shade(knob_colors[portrait_color], color)
                                  : jc_face_palette[color]);
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
    static const char *const drum_labels[4] = {"CHR", "REV", "CMP", "BAS"};
    static const char *const string_labels[4] = {"DRV", "CHR", "MOD", "DLY"};
    static const char *const piano_labels[4] = {"TON", "DEC", "CHR", "REV"};
    static const char *const synth_labels[4] = {"CUT", "RES", "ENV", "MOD"};
    static const char *const chord_labels[4] = {"TYP", "VOI", "INV", "SPR"};
    static const char *const cs80_labels[4] = {"BRI", "RES", "MIX", "VOL"};
    uint8_t values[4] = {drum_chorus, drum_reverb, drum_compression, drum_bass_boost};
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        char text[] = "K1 --- -";
        text[1] = (char)('1' + i);
        if (selected_instrument == 6u && cs80_preset) {
            char cs80_text[] = "K1 --- 000";
            cs80_text[1] = (char)('1' + i);
            cs80_text[3] = cs80_labels[i][0];
            cs80_text[4] = cs80_labels[i][1];
            cs80_text[5] = cs80_labels[i][2];
            cs80_text[7] = (char)('0' + cs80_controls[i] / 100u);
            cs80_text[8] = (char)('0' + (cs80_controls[i] / 10u) % 10u);
            cs80_text[9] = (char)('0' + cs80_controls[i] % 10u);
            draw_text(150, 66 + (int32_t)i * 20, cs80_text, knob_colors[i]);
            continue;
        } else if (selected_instrument == 5u) {
            text[3] = drum_labels[i][0];
            text[4] = drum_labels[i][1];
            text[5] = drum_labels[i][2];
            text[7] = (char)('0' + values[i]);
        } else if (selected_instrument == 3u || selected_instrument == 4u) {
            text[3] = string_labels[i][0];
            text[4] = string_labels[i][1];
            text[5] = string_labels[i][2];
            text[7] = (char)('0' + string_controls[selected_instrument - 3u][i]);
        } else if (selected_instrument == 7u) {
            text[3] = chord_labels[i][0];
            text[4] = chord_labels[i][1];
            text[5] = chord_labels[i][2];
            text[7] = (char)('0' + chord_controls[i]);
        } else {
            const char *label = selected_instrument == 1u ? piano_labels[i] : synth_labels[i];
            text[3] = label[0];
            text[4] = label[1];
            text[5] = label[2];
            text[7] = (char)('0' + voice_controls[selected_instrument][i]);
        }
        draw_text(150, 66 + (int32_t)i * 20, text, knob_colors[i]);
    }
}

static const char *const bass_sound_labels[3] = {
    "EDIT+K4: SYNTH", "EDIT+K4: NOMORE", "EDIT+K4: BUZZ"
};

void app_frame(void)
{
    uint32_t i;
    uint32_t now;
    char tempo_text[8], octave_text[] = "OCT +0";
    int32_t tempo;
    uint8_t playing;
    int redraw, drum_label_changed = 0;

    app_input_only();
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
    if ((int32_t)(plat_ms() - startup_splash_until) < 0)
        return;
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
    redraw = drum_label_changed || !ui_drawn
        || tempo != ui_tempo || playing != ui_playing || save_failed != ui_save_failed
        || portrait_color != ui_portrait_color
        || portrait_color_visible != ui_portrait_color_visible
        || knob_image_visible != ui_knob_image_visible
        || knob_image_frame != ui_knob_image_frame || knob_image_color != ui_knob_image_color
        || selected_instrument != ui_instrument || chord_voice != ui_chord_voice
                || chord_layout != ui_chord_layout || cs80_preset != ui_cs80_preset
                || (uint8_t)(phrase_state | (phrase_count ? 4u : 0u)) != ui_phrase_state
        || instrument_octave[selected_instrument] != ui_octave
        || drum_bass_boost != ui_drum_bass_boost || bass_sound != ui_bass_sound
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

        for (i = 0; i < 4u; i++) {
            int32_t y = 66 + (int32_t)i * 20;
            uint16_t color = selected_instrument == i ? RGB(80, 220, 255) : hits[i] ? C_WHITE : C_DIM;
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
            uint16_t color = selected_instrument == i ? RGB(80, 220, 255) : hits[i] ? C_WHITE : C_DIM;
            draw_text(18, y, track_names[i], color);
            if (hits[i]) {
                uint32_t bars = hits[i] > 8u ? 8u : hits[i];
                cv_rect(96, y + 2, (int32_t)bars * 5, 7, i == 5u ? C_HI : C_GRAY);
            }
        }
        draw_text(18, 46, "CHORD", selected_instrument == 7u ? RGB(80, 220, 255) : C_DIM);
        draw_knob_image();
        draw_text(12, 63, "KEYS:", C_GRAY);
        draw_text(48, 63, selected_instrument == 7u
                  ? chord_instrument_names[chord_voice]
                  : selected_instrument == 6u && cs80_preset
                    ? cs80_patches[cs80_preset - 1u].name : instrument_names[selected_instrument], C_HI);
        if (selected_instrument != 5u && selected_instrument != 7u) {
            int32_t octave = instrument_octave[selected_instrument];
            octave_text[4] = octave < 0 ? '-' : '+';
            octave_text[5] = (char)('0' + (octave < 0 ? -octave : octave));
            draw_text(158, 63, octave_text, C_HI);
        }
        draw_text(12, 78, plat_audio_status() ? "AUDIO INIT FAILED" :
                  ui_drum_sample_visible ? drum_sample_labels[ui_drum_sample] :
                  save_failed ? "SAVE FAILED" :
                  selected_instrument == 4u ? bass_sound_labels[bass_sound] :
                  selected_instrument == 6u ? "EDIT+SEL: PATCH" :
                  selected_instrument == 7u ? (chord_layout ? "EDIT+K4: SPLIT" : "EDIT+K4: 1 KEY") : "EDIT+K1: BPM  SAVE: STORE",
                  plat_audio_status() || save_failed ? C_HI : C_GRAY);
        if (phrase_state != PHRASE_IDLE)
            draw_text(204, 78, phrase_state == PHRASE_REC ? "REC" : "LOOP",
                      phrase_state == PHRASE_REC ? RGB(255, 90, 90) : C_HI);
        else if (phrase_count)
            draw_text(204, 78, "PHR", C_GRAY);
        cv_blit(0, 144);
        ui_tempo = tempo;
        ui_phrase_state = (uint8_t)(phrase_state | (phrase_count ? 4u : 0u));
        ui_instrument = selected_instrument;
        ui_chord_voice = chord_voice;
        ui_chord_layout = chord_layout;
        ui_cs80_preset = cs80_preset;
        ui_octave = instrument_octave[selected_instrument];
        ui_drum_bass_boost = drum_bass_boost;
        ui_bass_sound = bass_sound;
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
    plat_lights((playing ? (1u << B_PLAY) : 0u) | (phrase_state != PHRASE_IDLE ? (1u << B_REC) : 0u),
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
