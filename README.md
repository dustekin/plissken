# Plissken 0.11 Release

Plissken is an early seven-part soundtrack sketch prototype for the M-VAVE FM-1. It is built on `fm1-base`; the
`base/` directory is the build system and platform submodule.

It's a crappy little firmware, not meant to be serious, just wanted to prove I could do it with vibecoding.

ALWAYS FOLLOW INSTALL PROCEDURE LAID OUT IN https://thefirmwarehub.com/kb/  !!!
Use a bootloader! Never install on the 47 key without converting because you will damage your hardware calibration irreversibly!

Contact: raggadus@gmail.com

Please do what you like with the firmware, but give credit.

The current arrangement plays seven parts on a shared clock:

1. Eight-voice Juno-style synth with four-note chord stabs and stereo chorus
2. Piano chords and notes, including live FM-1 key input
3. Pro One-style monophonic sequence
4. Simple power-chord guitar
5. Electric-bass line
6. LM-1 kick, snare and hi-hat samples
7. CS-50-style line

This is a concurrency and resource-budget prototype, not a finished instrument. All 13 Rossf Linn LM-1 one-shots
are packed from the source WAVs at build time; the generated sample header is ignored and not checked in.
The normal PLAY loop uses a fixed 16-step arrangement. With `dsp/song.h` present, EDIT+PLAY toggles the
preloaded MIDI song mode; while it is active, live keys and editing controls are ignored. Press EDIT+PLAY again
to stop the song and return to instrument mode. Synthesis and effects are deliberately simple and need further
listening and refinement on the FM-1. The instrument screen shows the user-provided portrait at the top center,
packed as a 33×48 four-color grayscale image with transparent background. The first turn of the MASTER control or
SELECT, ALGO, or PRESET encoder colors it cyan; each subsequent detent advances through the on-screen knob colors
(cyan, green, pink, yellow), regardless of EDIT state. MASTER is an analog pot, so it advances once per 32 position
counts. K1–K4 do not affect the top-center portrait. The Juno, Pro One, bass, guitar, and
CS-50 oscillator edges use polyBLEP band-limiting to reduce high-note aliasing without oversampling or additional
audio buffers.
At startup, a color skyline splash is shown for at least 1.5 seconds while the CS-50 voice plays its opening
six-note phrase; the 240×144 image is packed into a four-color RGB565 palette for the FM-1 display.

DON'T PRESS PLAY, IT SOUNDS AWFUL.
I c
## Build and test

From this directory, use the VS Code terminal opened in WSL (Ubuntu), not PowerShell. The build commands use Bash
syntax and the WSL toolchain/SDK paths; PowerShell attempts have caused problems in this workflow.

```sh
python3 base/tools/build.py . --host
build/host/plissken_host tests/basic.txt build/host
build/host/plissken_host tests/bass_sounds.txt build/host
build/host/plissken_host tests/bass_sample_audio.txt build/host
build/host/plissken_host tests/guitar_samples.txt build/host
build/host/plissken_host tests/function_buttons.txt build/host
build/host/plissken_host tests/drum_samples.txt build/host
build/host/plissken_host tests/chord_mode.txt build/host
python3 tests/check_chord_render.py build/host/plissken_host
python3 tests/check_keyboard_tones.py build/host/plissken_host
python3 tests/check_cs80_render.py build/host/plissken_host
python3 tools/render_cs80_chords.py build/host/plissken_host renders/cs80-chords
build/host/plissken_host tests/keyboard.txt build/host
build/host/plissken_host tests/escape_mode.txt build/host
build/host/plissken_host tests/song.txt build/host
build/host/plissken_host tests/octave.txt build/host
build/host/plissken_host tests/drum_bass_boost.txt build/host
build/host/plissken_host tests/drum_effects.txt build/host
build/host/plissken_host tests/string_effects.txt build/host
build/host/plissken_host tests/voice_controls.txt build/host
build/host/plissken_host tests/voice_all_parts.txt build/host
python3 tests/check_voice_controls.py
python3 tests/check_velocity.py build/host/plissken_host
python3 tests/check_drum_effects.py build/host
python3 tests/check_drum_boost.py build/host
build/host/plissken_host tests/startup.txt build/host
build/host/plissken_host tests/ui.txt build/host
build/host/plissken_host tests/all_parts.txt build/host
```

The chord render check verifies that a held Pro One chord sustains all three notes, not just the last-triggered
voice, and that each note has its octave-sub component. The keyboard tone check verifies Juno, piano and Pro One
fundamentals across the playable range, velocity response and headroom, and all eight Juno voices sounding
together. The CS-80 render check exercises all ten independently designed patches, their output levels, and the
four continuous tone controls.

The chord-demo command writes one 44.1 kHz, 16-bit stereo F-major WAV per CS-80-inspired patch to
`renders/cs80-chords/`. The matching VS Code task, **Render CS-80 chord demos**, rebuilds the host renderer first.
The **Render Escape from New York MIDI** task renders the embedded MIDI song through EDIT+PLAY and saves the
verified 44.1 kHz, 16-bit stereo file as `renders/escape-from-new-york.wav`.

The build automatically packs all 13 Rossf Linn LM-1 WAVs from `references/rossf-lm1-drum-set/` as 8-bit mono.
The original audio files are preserved; converted firmware samples are generated at build time.
`NoMoreBass.wav` and the Bass808/BassPlunk1–4 WAVs and the three GuitarRiff WAVs are the project owner's recordings packed as
8-bit mono. See [CREDITS.md](CREDITS.md)
for sample details.

Build two distinct firmware packages: for device firmware, use `python3 base/tools/build.py .` with the JieLi
toolchain and AC79 SDK configured as described in the `fm1-base` documentation; for the browser emulator, use the
separate emulator-only command below. Do not use the emulator-only package on an FM-1.

For the Joakim Eriksson CPU-level browser emulator, build a separate compatibility package with
`python3 base/tools/build.py . --emulator-only`. Its output is `build/emulator-only/plissken-emulator-only.fwsc`,
and its screen says **EMU ONLY**. This build accepts two zero audio PLL registers to accommodate the emulator.
**Never install it on an FM-1**: the file is a firmware package and the warning is not an installation barrier.
The normal `build/plissken.fwsc` retains the stock hardware clock check and is not overwritten by this command.
An audio initialization failure is shown on the main screen as **AUDIO INIT FAILED**.

## GitHub releases

The current release is version **0.11**. Push the `v0.11` tag to build and attest `plissken-0.11.fwsc` and publish the
GitHub release titled **Plissken 0.11 Release**. Do not replace the attested package by hand. Future releases use the same `v*`
tag workflow. Release metadata is in `fm1.toml`, and the Firmware Hub card text and sound provenance are in the root
`fmhub.json`.

## Controls

- **PLAY** and **song mode (EDIT + PLAY)** are currently disabled (the backing track and song distorted); the
  buttons do nothing for now. Re-enable by setting `BACKING_ENABLED` to 1 in `app.c`.
- **SELECT encoder** chooses the sound played by the piano keys: Juno, piano, Pro-1, guitar, bass, LM-1 drums,
  CS-50/CS-80, or CHORD. The left-side instrument list highlights the current keyboard selection in cyan-blue;
  Bass is labeled BASS, CS-50/CS-80 shares one row, and CHORD appears below it.
- **OCT− / OCT+** shift the selected pitched keyboard sound by one octave, from −2 to +2. Each sound remembers its
  setting until reboot; held notes keep their pitch. LM-1 drum samples and the backing parts are unchanged.
- **FX** cycles the K2 effect setting (0–3) of the selected sound; **SEL** steps to the next variant (guitar riff, bass sample,
  CS-80 preset, or chord voice).
- **ENV / LFO / GLO** mute the backing chords / bass / drums (lit while muted). Pro-1 is unaffected.
- **ARP** arpeggiates held keys at 16th-note rate (lit while on). **HOME** stops playback and releases all notes.
  **SEQ** starts/stops the drum sequencer; a small SEQ marker shows mid-screen over the portrait while it runs.
- **Drum sequencer:** four tracks (kick, snare, closed hat, and one free percussion sound) on 16 steps at the current
  tempo. Tapping SEQ with an empty pattern starts a basic beat (kick, snare, hats). Select the drums (LM-1) and
  play drum keys: each hit is recorded on the nearest 16th step and loops.
  Key 1 = kick, key 2 = snare, key 3 = hat, any other drum key goes on the percussion track (one sound per step).
  SAVE stores the pattern with the tempo. GLO's drum mute silences it too.
- **Quick layers** (after Felucca): holding FX, GLO, SEQ or LFO for 0.4 s, or touching a key, knob or PLAY meanwhile,
  gives a layer instead of the tap action above. The keys are silent while FX, GLO or SEQ is held; the numbers count
  from the lowest key.
  - **FX held:** keys 1–4 set the effect to 0–3 for as long as the key is down.
  - **GLO held:** keys 1–3 mute chords / bass / drums, key 4 unmutes all, key 5 taps the tempo; **PLAY** restarts from the top.
  - **SEQ held:** key 1 clears the recorded phrase, key 2 reverses it, key 3 clears the drum sequencer.
  - **LFO held:** the keys still play; any knob turns are undone when LFO is let go.
- **Battery:** the JC portrait at the top dims as the battery bars drop (100%, 70%, 45%, 25% for 3 to 0 bars). It is
  full brightness on USB power.
- **EDIT + K1** changes tempo from 70 to 140 BPM.
- **K1–K3**, when LM-1 is selected, choose chorus, reverb, and compression presets respectively: 0 is off and 1–3
  select progressively different settings. These apply to live and backing drums and reset on reboot. Reverb is a
  lightweight multi-tap delay, not a physical room model.
- **K4**, when LM-1 is selected, adjusts drum bass boost from 0 (off) to 8. It affects both live and backing drums.
  The setting resets on reboot.
- **LM-1 keys:** key indices 0–12 trigger Rossf samples in this order: kick, snare, closed hat, open hat, high tom,
  low tom, clap, cowbell, tambourine, cabasa, clave, high conga, low conga. The mapping repeats every 13 keys;
  drums are one-shots and do not transpose. MIDI drum event notes map by note number modulo 13. The sample name
  flashes on the lower screen line for about 0.8 seconds whenever a drum is triggered.
- **K1–K4**, when guitar or bass is selected: drive, chorus, modulation and delay. Each is 0 (off) to 3;
  modulation 1–2 are flangers and 3 is a phaser. Settings are independent per instrument and reset on reboot.
- **Bass sound:** while BASS is selected, turn PRESET (or ALGO) to select the built-in synth or one of six
  samples (NoMore, 808, Plunk 1–4). K4 adjusts bass delay. All sampled bass sounds play two octaves below the
  key's normal pitch by default; octave controls still transpose them.
- **Guitar sound:** while GUITAR is selected, turn PRESET (or ALGO) to select the built-in synth or one of three riff
  samples (Riff 1–3); SEL also steps through them. Key 1 plays a riff at its recorded pitch and other keys transpose it.
  K4 adjusts guitar delay.
- **CHORD:** select CHORD, then play one key per chord. Turn PRESET to switch among CHORD PIANO,
  CHORD JUNO, CHORD CS-50, CHORD PRO-1, and CHORD GUITAR. The selected sound uses that instrument's existing
  K1–K4 sound settings; change those settings by selecting the instrument outside CHORD. K1 selects the chord
  type (0 major, 1 minor, 2 sus2, 3 sus4, 4 dominant 7, 5 major 7, 6 minor 7, 7 diminished); K2 selects voicing,
  K3 inversion, and K4 spread. Turn ALGO to choose the layout: **1 KEY** (every key plays a chord
  using the K1 type) or **SPLIT** (the lowest 12 keys, F3–E4, are chromatic roots; the next eight keys pick
  the chord type 0–7 and play nothing; the top keys are silent). Chords are live keyboard performance
  only, not a new backing track or looper. The layout resets on reboot.
- **Guitar tone:** a Karplus-Strong plucked-string model (noise burst in a damped delay loop) through a single-coil
  pickup comb, a mildly overdriven valve-style amp stage and a speaker low-pass, for a Fender-into-amp character.
- **Synth bass tone:** the SYNTH bass is tuned to the dark, soft-attack bass of the reference track: a four-pole
  low-pass at about 250 Hz, harmonics 1-4 roughly even, steep roll-off above. Measured numerically only; audition by ear.
- **Phrase recorder:** press **REC** to start recording live key playing (any keyboard instrument, including
  CHORD), press REC again to stop and loop it immediately, press REC again to stop the loop, and REC once more
  to replay it. **EDIT+REC** clears the phrase so a new one can be recorded. One phrase of up to 16 s / 128 key
  events is kept in RAM (lost on reboot); each note replays on the instrument and octave it was played with,
  and keys you hold yourself take priority over the loop. It runs free of the backing track's tempo. The REC
  light is on while recording or looping, and the screen shows REC, LOOP or PHR (stored).
- **K1–K4**, piano: tone, decay, chorus and reverb. For Juno, Pro-1 and CS-50: filter cutoff, resonance,
  filter envelope and modulation. Each setting is 0 (unchanged) to 3 and resets on reboot; the Juno's existing
  chorus remains part of its base sound. Controls apply to live and backing notes.
- **CS-80-inspired sounds:** while CS-50 is selected, turn PRESET (or ALGO) to cycle between the original
  CS-50 sound and ten independently designed CS-80-inspired patches. K1–K4 control Brilliance, Resonance, Mix,
  and Volume on these patches, with 0–127 values and fine steps per encoder detent. These patches also replace
  the CS-50 backing part while selected; CHORD CS-50 retains the original CS-50 renderer. This compact renderer
  is an inspiration, not a full CS-80 emulation. Verify timing and sound on an FM-1 before relying on its CPU
  headroom; host timing does not measure device DSP load.
- **SAVE** stores the tempo for the next boot.
- **MASTER** controls output level.
- **Piano keys** play the selected sound; for LM-1 drums, keys map cyclically to kick, snare, and hi-hat.
- Live key velocity and MIDI-note velocity affect the level of Juno, Pro-1, guitar, bass, and CS-50 voices;
  piano already uses velocity for level and brightness.

The screen shows the selected keyboard sound and its color-coded K1–K4 settings on the right. Instrument names are
white, the selected one cyan-blue; the small bars beside them are activity meters only. The backing track is
currently disabled; LM-1 sample loading is not implemented yet.
After a knob is turned, its matching color-tinted snake portrait appears below K4. The displayed image follows
the setting: 0 uses `snake sma4.png`, 1 uses `snake sma2.png`, 2 uses `snake sma3.png`, and 3 uses `snake sma.png`.
On CS-50/CS-80 patches, each knob turn instead advances through `sma`, `sma1`, `sma2`, and `sma3`, then repeats
from `sma1`, even when that knob is already at its minimum or maximum. The supplied `snake-sma4.png` artwork is
used for the `sma1` slot because no `snake-sma1.png` asset is present.
The image and most recently used knob's color remain visible until another knob is turned. The portrait is
hidden at startup until the first knob turn.

## License

Plissken's application code is licensed under [MPL-2.0](LICENSE). Third-party code and assets may have different
terms; see [CREDITS.md](CREDITS.md) and [base/LICENSING.md](base/LICENSING.md).

## Future direction

Known issues and ideas for upcoming work:

1. Guitar FX knobs K1–K3 (drive, chorus, modulation) do not audibly change the sound; K4 (delay) works well.
2. Juno is still quiet compared with the other instruments.
3. Bass FX knobs K1–K3 have the same problem as the guitar's.
4. CHORD is still too loud.
5. Consider a visual (grid) sequencer.
6. Getting the MIDI song/PLAY working again will probably require trimming instruments (e.g. dropping guitar).
7. Splice sounds may be used with artist credits (see [CREDITS.md](CREDITS.md)).

Planned work includes further improving sound quality within the FM-1's RAM and processor budget, expanding the
budget-conscious Orchid-inspired chord/phrase feature, auditioning and refining the selectable bass sounds, support
for selecting additional preloaded MIDI files, and additional original graphics inspired by *Escape from New York*.
Three user-provided one-note bass recordings are currently switchable. Verify rights before distributing other
MIDI, samples, or artwork.
