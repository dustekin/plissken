# Plissken

Plissken is a live-performance instrument prototype for the M-VAVE FM-1. It is built on `fm1-base`; the `base/`
directory is the build system and platform submodule. The keyboard offers Juno, piano, Pro-1, guitar, bass, LM-1
drums, CS-50/CS-80 and CHORD sounds, along with a live phrase recorder and a visual five-track sequencer with
saved-song playback.

This checkout is the **v0.12 release candidate**. The published, signed v0.11 release does not include the visual
sequencer and saved-song playback features described below.

It's a crappy little firmware, not meant to be serious, just wanted to prove I could do it with vibecoding.

ALWAYS FOLLOW INSTALL PROCEDURE LAID OUT IN https://thefirmwarehub.com/kb/  !!!
Use a bootloader! Never install on the 47 key without converting because you will damage your hardware calibration irreversibly!

Contact: raggadus@gmail.com

Please do what you like with the firmware, but give credit.

The original seven-part backing arrangement and EDIT + PLAY song mode are disabled because their mix distorted
(`BACKING_ENABLED` is 0 in `app.c`). A reduced, four-voice sequencer transcription of the authorized render-test
MIDI is available as **Escape From Firmware**; if deleted, it is restored on the next boot. PLAY opens the saved-song
selector, not the disabled backing arrangement. This remains a concurrency and resource-budget prototype, not a
finished instrument. All 13 Rossf Linn LM-1 one-shots are packed
from the source WAVs at build time; the generated sample header is ignored and not checked in. Synthesis and
effects are deliberately simple and need further listening and refinement on the FM-1. The instrument screen shows
the user-provided portrait at the top center,
packed as a 33×48 four-color grayscale image with transparent background. The first turn of the MASTER control or
SELECT, ALGO, or PRESET encoder colors it cyan; each subsequent detent advances through the on-screen knob colors
(cyan, green, pink, yellow), regardless of EDIT state. MASTER is an analog pot, so it advances once per 32 position
counts. K1–K4 do not affect the top-center portrait. The Juno, Pro One, bass, guitar, and
CS-50 oscillator edges use polyBLEP band-limiting to reduce high-note aliasing without oversampling or additional
audio buffers.
At startup, a color skyline splash is shown for at least 1.5 seconds while the CS-50 voice plays its opening
six-note phrase; the 240×144 image is packed into a four-color RGB565 palette for the FM-1 display.

## v0.12 release candidate

Build 012 has been physically tested on the FM-1 and confirmed working. This candidate adds:

- A saved-song Playback screen with 16 user slots plus the built-in **Escape From Firmware** track. REC deletes the
  selected song; pressing a piano key loads it into the sequencer for editing, and that opening key is not recorded.
  SAVE updates the selected song.
- A visual five-track sequencer with four pitched lanes and an LM-1 drum lane, saved tempo, playback-follow, and
  note/sample editing.
- Contextual GLO help on the main, Playback, and sequencer screens.

The candidate is function-complete for its current feature scope. Further work may explore a higher voice budget,
sound-quality improvements, and general quality-of-life refinements.

## Build and test

From this directory, use the VS Code terminal opened in WSL (Ubuntu), not PowerShell. The build commands use Bash
syntax and the WSL toolchain/SDK paths; PowerShell attempts have caused problems in this workflow.

```sh
python3 base/tools/build.py . --host
for test in tests/*.txt; do
  build/host/plissken_host "$test" build/host
done
python3 tests/test_song_to_sequence.py
python3 tests/check_chord_render.py build/host/plissken_host
python3 tests/check_keyboard_tones.py build/host/plissken_host
python3 tests/check_cs80_render.py build/host/plissken_host
python3 tools/render_cs80_chords.py build/host/plissken_host renders/cs80-chords
python3 tests/check_voice_controls.py
python3 tests/check_velocity.py build/host/plissken_host
python3 tests/check_drum_effects.py build/host
python3 tests/check_drum_boost.py build/host
```

The checked-in v0.12 candidate `dsp/escape_sequence.h` is derived from the authorized render MIDI via
`python3 tools/midi_to_song.py <file.mid>` and `python3 tools/song_to_sequence.py`. It uses a sixteenth-step
onset grid for CS-50 while preserving baseline per-part event counts; the transcription remains within the
sequencer's event and four-voice limits. Regenerate it with
`python3 tools/song_to_sequence.py --part-grid 6=1 --preserve-part-counts dsp/song.h build/gen/lm1_samples.h dsp/escape_sequence.h`.
Before changing the conversion settings, `python3 tools/song_to_sequence.py --report` prints per-part merge,
polyphony, event-cap, onset-error and duration-error diagnostics without writing the generated header.
Use `--report --part-grid 6=1 --preserve-part-counts` to test a CS-50 sixteenth-note onset grid while preserving
the default conversion's per-part event counts. Matching baseline events are retained where they survive the
voice limit, and remaining slots are sampled evenly within each part. This diagnostic still does not write the
arrangement.
To write a separate candidate header for a hardware experiment, omit `--report` and provide an explicit output
path; for example:

```sh
python3 tools/song_to_sequence.py --part-grid 6=1 --preserve-part-counts \
  dsp/song.h build/gen/lm1_samples.h build/escape_sequence.cs50.h
```

The chord render check verifies that a held Pro One chord sustains all three notes, not just the last-triggered
voice, and that each note has its octave-sub component. The keyboard tone check verifies Juno, piano and Pro One
fundamentals across the playable range, velocity response and headroom, and all eight Juno voices sounding
together. The CS-80 render check exercises all ten independently designed patches, their output levels, and the
four continuous tone controls. Drum boost checks compare low-frequency energy for both live and saved-sequence
drums.

The chord-demo command writes one 44.1 kHz, 16-bit stereo F-major WAV per CS-80-inspired patch to
`renders/cs80-chords/`. The matching VS Code task, **Render CS-80 chord demos**, rebuilds the host renderer first.
The **Render Escape from New York MIDI** task is currently unavailable: it still expects `tests/song.txt`, which
is in `tests/disabled/` while song playback is disabled.

The build automatically packs all 13 Rossf Linn LM-1 WAVs from `references/rossf-lm1-drum-set/` as 8-bit mono.
The original audio files are preserved; converted firmware samples are generated at build time.
`NoMoreBass.wav`, Bass808, BassPlunk1, BassPlunk3 and BassPlunk4, and the three GuitarRiff WAVs are the project owner's recordings packed as
8-bit mono. See [CREDITS.md](CREDITS.md)
for sample details.

Build two distinct firmware packages: for device firmware, use `python3 base/tools/build.py .` with the JieLi
toolchain and AC79 SDK configured as described in the `fm1-base` documentation; for the browser emulator, use the
separate emulator-only command below. Do not use the emulator-only package on an FM-1.

For the Joakim Eriksson CPU-level browser emulator, build a separate compatibility package with
`python3 base/tools/build.py . --emulator-only`. Its output is
`build/emulator-only/dont-install-on-your-fm1-plissken-emulator-only.fwsc`,
and its screen says **EMU ONLY**. This build accepts two zero audio PLL registers to accommodate the emulator.
**Never install it on an FM-1**: the file is a firmware package and the warning is not an installation barrier.
The normal `build/plissken.fwsc` retains the stock hardware clock check and is not overwritten by this command.
An audio initialization failure is shown on the main screen as **AUDIO INIT FAILED**.

## GitHub releases

The current published release, **v0.11**, remains signed and unchanged. This checkout is the **v0.12 release
candidate**; build 012 was tested on hardware. Pushing a `v0.12` tag triggers the release workflow, which builds
and attests the versioned package from that tag. Do not replace the attested v0.11 package by hand. Product and
install metadata are in `fm1.toml`; the Firmware Hub card text and sound provenance are in the root `fmhub.json`.

## Controls

- **PLAY** opens the saved-song selector. Choose a song with PRESET and press PLAY to start or stop it; REC deletes
  the selected song (stopping playback first if needed). Press any piano key to load the selected song into the
  sequencer for editing; that first key only opens the editor. SAVE writes edits back to the selected song. HOME exits.
  **GLO** opens playback help; turn any knob to change pages, and press GLO, SEQ or HOME to return.
  The original MIDI/backing arrangement and EDIT + PLAY remain disabled because their mix distorted. The reduced
  **Escape From Firmware** track is a separate sequencer transcription. Do not re-enable the original arrangement
  by changing `BACKING_ENABLED` alone; first fix and verify the mix.
- **SELECT encoder** chooses the sound played by the piano keys: Juno, piano, Pro-1, guitar, bass, LM-1 drums,
  CS-50/CS-80, or CHORD. The left-side instrument list highlights the current keyboard selection in cyan-blue;
  Bass is labeled BASS, CS-50/CS-80 shares one row, and CHORD appears below it.
- **OCT− / OCT+** shift the selected pitched keyboard sound by one octave, from −2 to +2. Each sound remembers its
  setting until reboot; held notes keep their pitch. LM-1 drum samples are not transposed.
- **FX** cycles the K2 effect setting (0–3) of the selected sound; **SEL** steps to the next variant (guitar riff, bass sample,
  CS-80 preset, or chord voice).
- **ENV / LFO** mute backing chords / bass (lit while muted). They have no audible effect while backing playback
  is disabled; Pro-1 is unaffected.
- **GLO** opens main-screen help; turn any knob to change pages, and press GLO, SEQ or HOME to return. In playback
  and the sequencer, GLO opens help for that screen.
- **ARP** arpeggiates held keys at 16th-note rate (lit while on). **HOME** stops playback and releases all notes.
- **Visual sequencer:** **SEQ** opens the editor with four pitched-sound tracks and a fifth LM-1 drum track;
  press SEQ or HOME to exit. PRESET moves between tracks,
  ALGO moves the step cursor, SELECT chooses that track's sound, SEL cycles the time signature, MASTER sets
  the selected note's length, and EDIT + K1 adjusts the song's BPM (70–140). Piano keys play and insert or replace a
  note at the cursor, including while the sequence plays. REC deletes the selected note and does nothing when the
  cursor is on an empty cell. OCT− / OCT+ change the octave of pitched sounds. Note, octave and K1–K4 effect
  changes briefly appear as text. PLAY starts or stops the working sequence; a quick double-press stops it and
  returns the cursor to the beginning. Grid lines are emphasized after every four steps. GLO opens the larger-text
  help; turn any knob to change pages. To edit a saved song, select it in the playback screen and press any piano
  key; the opening key is not recorded, and SAVE updates that song.
  During playback the display and cursor follow the music. Moving ALGO pauses follow so you can edit another
  position; stop and restart playback to resume following. On the LM-1 track, SELECT chooses a drum sound and
  piano keys choose and insert individual samples; different samples can share a step. REC removes only the
  selected sample and never adds one. Drum hits play their full samples; their bar length reflects sample duration
  and tempo. MASTER does not change it. LM-1 is excluded from the other tracks' sound choices; older
  saved songs retain their original drum lengths.
  SAVE names and saves the sequence; up to 16 user songs plus **Escape From Firmware** can
  be stored, with about 950 events per song and up to 128 bars. Playback is limited to four simultaneous sound
  voices (chord sounds count as multiple voices); pitched samples are limited to their note length.
  Song tempo is saved with each song and does not change the main-screen tempo. Older saved songs without a tempo
  play at the current main-screen tempo.
- **Quick layers** (after Felucca): holding FX, GLO, SEQ or LFO for 0.4 s, or touching a key, knob or PLAY meanwhile,
  gives a layer instead of the tap action above. The keys are silent while FX, GLO or SEQ is held; the numbers count
  from the lowest key.
  - **FX held:** keys 1–4 set the effect to 0–3 for as long as the key is down.
  - **GLO held:** keys 1–3 mute chords / bass / drums, key 4 unmutes all, and key 5 taps the tempo.
  - **SEQ held:** key 1 clears the recorded phrase, key 2 reverses it, key 3 clears the legacy drum pattern.
  - **LFO held:** the keys still play; any knob turns are undone when LFO is let go.
- **Battery:** the JC portrait at the top dims as the battery bars drop (100%, 70%, 45%, 25% for 3 to 0 bars). It is
  full brightness on USB power.
- **EDIT + K1** changes tempo from 70 to 140 BPM.
- **K1–K3**, when LM-1 is selected, choose chorus, reverb, and compression presets respectively: 0 is off and 1–3
  select progressively different settings. These apply to live drums and reset on reboot. Reverb is a
  lightweight multi-tap delay, not a physical room model.
- **K4**, when LM-1 is selected, adjusts drum bass boost from 0 (off) to 8. It affects live and sequencer drums.
  The setting resets on reboot.
- **LM-1 keys:** key indices 0–12 trigger Rossf samples in this order: kick, snare, closed hat, open hat, high tom,
  low tom, clap, cowbell, tambourine, cabasa, clave, high conga, low conga. The mapping repeats every 13 keys;
  drums are one-shots and do not transpose. When song playback is enabled, MIDI drum event notes map by note number
  modulo 13. The sample name flashes on the lower screen line for about 0.8 seconds whenever a drum is triggered.
- **K1–K4**, when guitar or bass is selected: drive, chorus, modulation and delay for live notes. Each is 0 (off) to 3;
  modulation 1–2 are flangers and 3 is a phaser. Settings are independent per instrument and reset on reboot.
- **Bass sound:** while BASS is selected, turn PRESET (or ALGO) to select the built-in synth or one of five
  samples (NoMore, 808, Plunk 1, 3 or 4). K4 adjusts bass delay. All sampled bass sounds play two octaves below the
  key's normal pitch by default; octave controls still transpose them.
  Plunk 2 is no longer packed; previously saved sequencer songs containing it cannot play and show a load error.
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
  and keys you hold yourself take priority over the loop. It runs independently of the drum sequencer tempo. The REC
  light is on while recording or looping, and the screen shows REC, LOOP or PHR (stored).
- **K1–K4**, piano: tone, decay, chorus and reverb. For Juno, Pro-1 and CS-50: filter cutoff, resonance,
  filter envelope and modulation. Each setting is 0 (unchanged) to 3 and resets on reboot; the Juno's existing
  chorus remains part of its base sound. Controls apply to live notes; backing playback is disabled.
- **CS-80-inspired sounds:** while CS-50 is selected, turn PRESET (or ALGO) to cycle between the original
  CS-50 sound and ten independently designed CS-80-inspired patches. K1–K4 control Brilliance, Resonance, Mix,
  and Volume on these patches, with 0–127 values and fine steps per encoder detent. When backing playback is
  enabled, these patches replace the CS-50 backing part while selected; CHORD CS-50 retains the original renderer.
  This compact renderer is an inspiration, not a full CS-80 emulation. Verify timing and sound on an FM-1 before
  relying on its CPU headroom; host timing does not measure device DSP load.
- **SAVE** stores the tempo for the next boot on the main screen; in the sequencer editor it saves the named song.
- **MASTER** controls output level.
- **Piano keys** play the selected sound; LM-1 keys cycle through all 13 drum samples.
- Live key velocity and MIDI-note velocity affect the level of Juno, Pro-1, guitar, bass, and CS-50 voices;
  piano already uses velocity for level and brightness.

The screen shows the selected keyboard sound and its color-coded K1–K4 settings on the right. Instrument names are
white, the selected one cyan-blue; the small bars beside them are activity meters only. The backing track is
currently disabled. Custom LM-1 sample loading is not implemented; the shipped samples are packed into the build.
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
5. Getting the preloaded MIDI song/backing arrangement working again will probably require trimming instruments
   (e.g. dropping guitar); saved visual-sequencer playback is implemented separately.
7. Splice sounds may be used with artist credits (see [CREDITS.md](CREDITS.md)).

Planned work includes further improving sound quality within the FM-1's RAM and processor budget, expanding the
budget-conscious Orchid-inspired chord/phrase feature, auditioning and refining the selectable bass sounds, support
for selecting additional preloaded MIDI files, and additional original graphics inspired by *Escape from New York*.
Three user-provided one-note bass recordings are currently switchable. Verify rights before distributing other
MIDI, samples, or artwork.
