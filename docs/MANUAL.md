# Plissken

Plissken is a seven-part soundtrack sketch for the M-VAVE FM-1. It is an early prototype: one shared clock drives
an eight-voice Juno-style synth with chord stabs and stereo chorus, piano chords and notes, a Pro One-style line,
simple guitar power chords, electric bass, and 13 LM-1 drum samples.

## Controls

| Control | Function |
| --- | --- |
| PLAY | Open the saved-song selector; start or stop the selected song |
| SELECT encoder | Choose the sound played by the piano keys: Juno, piano, Pro-1, guitar, bass, LM-1 drums, CS-50/CS-80 or CHORD |
| PRESET / ALGO knobs | Switch the sound of the selected instrument: bass (synth or five samples), guitar (synth or three riffs), CS-50/CS-80 patches, CHORD voice (PRESET) and layout (ALGO) |
| SEQ | Open the visual sequencer editor |
| FX / SEL | FX cycles the selected sound's effect setting; SEL steps to the next variant |
| REC | Record, loop and replay a live phrase; EDIT + REC clears it |
| OCT− / OCT+ | Shift the selected pitched keyboard sound down or up an octave, within −2 to +2 |
| EDIT + K1 | Set tempo from 70 to 140 BPM |
| K1–K3 (LM-1 selected) | Choose chorus, reverb and compression presets: 0 off, 1–3 on |
| K4 (LM-1 selected) | Set bass boost from 0 (off) to 8 for live and backing drums |
| K1–K4 (guitar or bass selected) | Drive, chorus, modulation and delay; 0 off, 1–3 presets |
| K1–K4 (piano selected) | Tone, decay, chorus and reverb; 0 unchanged, 1–3 settings |
| K1–K4 (Juno, Pro-1 or CS-50 selected) | Filter cutoff, resonance, filter envelope and modulation; 0 unchanged, 1–3 settings |
| SAVE | Store the current tempo on the main screen; save a named song in the sequencer editor |
| MASTER | Set output volume |
| Piano keys | Play the selected sound; LM-1 maps keys cyclically to its 13 samples |

In the sequencer editor, PRESET chooses one of four pitched-sound tracks or the fifth LM-1 track, ALGO moves the step cursor, SELECT chooses the track's
sound, SEL cycles the time signature, and moving MASTER changes only the selected pitched note's length. Piano keys play and
insert or replace the note at the cursor, including during playback. REC removes a selected note or adds one at an
empty cell. OCT− / OCT+ change the octave of pitched sounds; note, octave and K1–K4 effect changes briefly appear
as text. PLAY starts or stops the working sequence. GLO opens the larger-text help, where any knob changes pages;
GLO, SEQ or HOME returns to the editor. SEQ or HOME exits the editor. SAVE prompts for a name and stores the song.
During playback the display and cursor follow the music; moving ALGO pauses follow for editing. Stop and restart
playback to resume following. On the LM-1 track, SELECT chooses a sample and piano keys choose and insert their
individual samples. Different drum samples can share a step; REC adds or removes only the selected sample.
New drum hits play the full sample, and their displayed length follows the sample duration and song tempo; MASTER
does not change drum lengths. LM-1 is not offered in the other tracks' sound choices. Existing saved songs remain
playable with their original drum lengths.
Up to 16 user songs can be saved, plus the built-in Escape From Firmware, Halloween, and The Fog tracks. To play
The Fog, press PLAY, turn PRESET to select it, then press PLAY again. Press a piano key instead of PLAY to open it
for editing; that first key is not recorded. Deleting a built-in song restores it on the next boot. Each track
supports up to 950 events and 128 bars. Playback allows at most four simultaneous sound voices; chord events count
as multiple voices, and pitched samples are cut off at their selected duration. Drum tails reserve voices conservatively at the fastest
tempo so later tempo changes cannot exceed that limit.

In the sequencer, EDIT + K1 adjusts the working song's tempo (70–140 BPM), shown beside the time signature;
K1 alone still adjusts the sound. SAVE stores the tempo with the song, and playback uses that saved tempo without
changing the main-screen tempo. Older songs saved without a tempo play at the current main-screen tempo. On the main
screen, EDIT + K1 adjusts its separate tempo, and SAVE keeps that tempo across reboot.
BPM counts quarter notes. The sequencer uses a fixed sixteenth-note grid, so one grid
step lasts 15/BPM seconds. A 4/4 bar has 16 steps, a 5/4 bar has 20, an 8/4 bar has 32, and 8/8 and 16/16 bars
have 16 each.
Changing the time signature changes the bar grouping, not the playback speed.

Each pitched sound remembers its own octave until reboot. Held notes keep their pitch until released; new notes use
the new setting. Octave buttons do not change the LM-1 samples or the backing parts. The screen shows the selected
pitched sound's octave and a color-coded K1–K4 column for its knob settings. LM-1's CHR (chorus), REV (reverb),
CMP (compression) and BAS (bass boost) settings reset to 0 on
reboot. The reverb is a lightweight multi-tap delay, not a physical room model.
Guitar and bass have independent settings for drive, chorus, modulation (two flangers and a phaser), and delay.
The effects apply to live notes and the backing part and reset on reboot.
Piano and synth controls also affect live and backing notes, reset on reboot, and use 0 for the unmodified base
sound. The existing Juno chorus remains enabled as part of that base sound. Piano reverb uses a lightweight
multi-tap delay rather than a physical room model.

The screen lists the instruments in white, with the selected keyboard sound in cyan-blue. The small bars beside the
names are activity meters only. In the sequencer, the editor grid displays tracks and note lengths. CHORD plays one
chord per key; K1 sets the chord type and ALGO chooses the 1 KEY or SPLIT layout.
At startup, the display shows the color skyline artwork from `assets/startup.png`, centered vertically and packed into
a four-color RGB565 palette, for 3 seconds. A cyan scanline sweeps down the image while the Pro-1 voice plays a short
keyboard phrase.

## Current limitations

The preloaded MIDI/backing arrangement is disabled in this build because it distorted; it may return after further
work on gain staging and instrument count. PLAY instead selects and plays songs saved in the visual sequencer.
Escape From Firmware, Halloween, and The Fog are the preloaded sequencer tracks; additional preloaded MIDI selection
is not implemented. Drum playback uses the 13 Rossf LM-1 samples packed at build time; see the project credits
for their license and attribution.
Guitar and bass modeling are intentionally simple. Bluetooth and per-track mute/level controls are not implemented.

The prototype is intended to establish that all seven parts can run together and to provide a baseline for measuring
CPU, memory, and audio stability before refining the sound engines.

## Emulator and hardware safety

The browser emulator needs the separately built
`build/emulator-only/dont-install-on-your-fm1-plissken-emulator-only.fwsc` compatibility
package. **Never install it on an FM-1:** it relaxes the audio clock check for an emulator-specific case, and the
package can still be installed despite its `EMU ONLY` label. The normal hardware-target package retains the stock
clock check. `AUDIO INIT FAILED` indicates an audio initialization failure.
