# Plissken

Plissken is a seven-part soundtrack sketch for the M-VAVE FM-1. It is an early prototype: one shared clock drives
an eight-voice Juno-style synth with chord stabs and stereo chorus, piano chords and notes, a Pro One-style line,
simple guitar power chords, electric bass, and LM-1 kick, snare, and hi-hat samples.

## Controls

| Control | Function |
| --- | --- |
| PLAY | Currently disabled (backing track and song mode are switched off) |
| SELECT encoder | Choose the sound played by the piano keys: Juno, piano, Pro-1, guitar, bass, LM-1 drums, CS-50/CS-80 or CHORD |
| PRESET / ALGO knobs | Switch the sound of the selected instrument: bass (synth or six samples), guitar (synth or three riffs), CS-50/CS-80 patches, CHORD voice (PRESET) and layout (ALGO) |
| SEQ | Start or stop the drum sequencer; with an empty pattern it starts a basic kick/snare/hat beat. A SEQ marker shows mid-screen while it runs |
| FX / SEL | FX cycles the selected sound's effect setting; SEL steps to the next variant |
| REC | Record, loop and replay a live phrase; EDIT + REC clears it |
| OCT− / OCT+ | Shift the selected pitched keyboard sound down or up an octave, within −2 to +2 |
| EDIT + K1 | Set tempo from 70 to 140 BPM |
| K1–K3 (LM-1 selected) | Choose chorus, reverb and compression presets: 0 off, 1–3 on |
| K4 (LM-1 selected) | Set bass boost from 0 (off) to 8 for live and backing drums |
| K1–K4 (guitar or bass selected) | Drive, chorus, modulation and delay; 0 off, 1–3 presets |
| K1–K4 (piano selected) | Tone, decay, chorus and reverb; 0 unchanged, 1–3 settings |
| K1–K4 (Juno, Pro-1 or CS-50 selected) | Filter cutoff, resonance, filter envelope and modulation; 0 unchanged, 1–3 settings |
| SAVE | Store the current tempo |
| MASTER | Set output volume |
| Piano keys | Play the selected sound; LM-1 maps keys cyclically to kick, snare, and hi-hat |

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
names are activity meters only. In drum sequencer use, drum keys record only while LM-1 is selected, on the nearest
16th step. CHORD plays one chord per key; K1 sets the chord type and ALGO chooses the 1 KEY or SPLIT layout.
At startup, the display shows the color skyline artwork from `assets/startup.png`, packed into a four-color RGB565 palette, for at least 1.5 seconds while the CS-50 voice plays the opening six-note phrase.

## Current limitations

PLAY and song mode are disabled in this build because the backing arrangement distorted; they are planned to return
after further work on gain staging and instrument count. The arrangement cannot be edited in the firmware, and
selecting and playing preloaded MIDI files is planned but not yet implemented. Drum playback uses three locally supplied WAV samples;
the generated sample data is not included in the repository. Confirm their license before distributing firmware.
Guitar and bass modeling are intentionally simple. Bluetooth and per-track mute/level controls are not implemented.

The prototype is intended to establish that all seven parts can run together and to provide a baseline for measuring
CPU, memory, and audio stability before refining the sound engines.

## Emulator and hardware safety

The browser emulator needs the separately built
`build/emulator-only/dont-install-on-your-fm1-plissken-emulator-only.fwsc` compatibility
package. **Never install it on an FM-1:** it relaxes the audio clock check for an emulator-specific case, and the
package can still be installed despite its `EMU ONLY` label. The normal hardware-target package retains the stock
clock check. `AUDIO INIT FAILED` indicates an audio initialization failure.
