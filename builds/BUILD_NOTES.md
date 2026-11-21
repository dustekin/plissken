# Test build notes

Each test build is saved as `plissken###.fwsc` with an increasing number. Newest first.
Add an entry here whenever a new build is saved.

## 023
- Versioned `v0.20-rc1` release-candidate package, including the sampled piano, guitar, and bass updates and the
  corrected startup phrase.
- All host scenarios and focused audio checks pass; the normal FM-1 target build passed. Not hardware-tested.
- Device image 481,092 B; package 614,938 B; identity `FM-1_1981002000`; build ID `37FC50C0`; SHA-256
  `79773DB1D6BA473045F763AC363A518840721081D930CE54786E88F9EDF83385`.

## 022
- Restores the original startup gain and voices the repeated low E2/E3 pulse as a single E4 pulse, matching the
  clear ending pitch while reducing the stacked startup notes.
- Host startup test passes at -16.6 dBFS peak (between the -18 and -13 dBFS bounds); bass sample audio passes.
- Normal FM-1 target build passed; hardware-tested, and the user reports it works fine. Device image 481,084 B;
  package 614,938 B; build ID `06D94827`; SHA-256 `77F08107DD3222A0C7CFE517F5C56A38C642F6F1B1745502E2A63D13AD29F8FD`.

## 021
- Reduces only the automatic Pro-1 startup phrase to one-eighth gain; main-screen instrument levels are unchanged.
- Host startup test passes at -29.0 dBFS peak with a -27 dBFS ceiling. Normal FM-1 target build passed. On hardware,
  the first note started buzzy and repeated; reducing gain did not fix it. Build 022 changes the repeated notes instead.
  Device image 481,140 B; package 614,938 B; build ID `CCC64AA3`; SHA-256
  `B66019547ACCB163A8BE15563739F4E105575B5BD394ED0C688893A0E0962C60`.

## 020
- Adds Electric and Acoustic bass samples, Sustain/Note modes for all bass timbres, and Electric Sustain startup;
  boosts 808 and removes the four BassPlunk recordings.
- Bass host audio/mode, UI, and sequencer tests pass; guitar and piano sample regressions pass. The broader keyboard
  tone check still fails its Juno velocity-response assertion (1.47x RMS ratio).
- Normal FM-1 target build passed. On hardware, main-screen sounds were good, but the startup phrase buzzed; build
  021 tested a lower phrase level without resolving the buzz. Device image 481,144 B; package 614,938 B; build ID
  `C5B84EE5`; SHA-256 `800A10EE531E01950E9458C29F7EB39324F7ED4ABBE102DC2DA1CED8B5A69706`.

## 018
- Adds The Fog as an editable, restored-on-boot preloaded track: 904 events, 45 bars, 4/4 at 123 BPM, using
  piano, Juno, bass, CS-50 and LM-1 drums. The cover MIDI remains a local reference and is not distributed.
- Centers the 240x144 startup splash vertically on the 240x240 display. No piano sound changes are included;
  piano research is recorded in the README for a later session.
- Target build passed; host startup, screens, UI, Fog, and built-in sequence scenarios passed. Startup peak:
  -14.4 dBFS. The project owner tested build 018 on an FM-1.
- Device image 489,952 B; package 614,938 B; build ID `7BAB38E6`; SHA-256
  `E6D499853F309F86C812B479B6C3ACA910200A3F1204E84C79A8F7A42E9B6C8E`.

## 017
- Saved-sequence LM-1 events are mixed at 42% of live-drum level so the chorused Juno lane and bass remain dominant;
  live keyboard drums are unchanged.
- Halloween isolated levels: Juno peak -16.30 dBFS / RMS -37.41; percussion peak -24.08 / RMS -45.86.
  Full preview peak -14.40 dBFS / RMS -34.23.
- Device image 486,448 B; package 614,938 B; build ID `0F107337`; SHA-256
  `F9FBEA962111B693C50C365416F72869D0C970102DC6A9B485CE89509FF1294C`.
- Physical FM-1 test passed; pitched Juno/bass parts are now dominant over percussion.

## 016
- Halloween comparison build: the 400-event former piano lane is rendered by the chorused Juno engine rather than
  CS-50 or piano, while remaining lowered one octave.
- Host sequence and full 201.079 s render pass; peak -14.10 dBFS, RMS -33.12 dBFS.
- Device image 486,440 B; package 614,938 B; build ID `8D0B222B`; SHA-256
  `E7CE9A84F2B630BE12629FB89B4138D0BA843E9AA8B54AAF1B6A1EE532436277`.
- Not hardware-tested.

## 015
- Added Halloween as a second protected, editable built-in track selected through PLAY/STOP; the sequence is 5/4 at
  120 BPM, uses 80 bars, and assigns the synth stem to Juno, with piano, bass, and LM-1 drums.
- Conversion retains 950 events (42 Juno, 400 piano, 22 bass, 486 drums) from separate local reference stems. The
  piano-stem notes are transposed down one octave and rendered by the chorused Juno voice; bass/Juno win voice-budget collisions,
  reducing the first preview's tinny piano/closed-hat balance. The source MIDI files remain outside the repository.
- Host sequencer scenarios pass. CS-50 full 201.079 s stereo preview: peak -14.10 dBFS, RMS -32.15 dBFS.
- Device image 486,440 B; package 614,938 B; build ID `53FA0EC8`; SHA-256
  `896DD2BC6C18F1E0D6235692D849720BA6DA11EE91CAD0D3D5D20B23517BAED7`.
- Not hardware-tested.

## 014
- Startup music and splash shortened to 3 s: the final second (the stacked closing chord) is removed because it distorted badly on hardware.
- Host tests pass (startup peak -14.4 dBFS).
- Physical FM-1 test passed; user confirmed the startup no longer distorts.

## 013
- Startup: cyan scanline-sweep animation over the splash (900 ms), splash held for the full 4 s music.
- Startup music: Pro-1 keyboard phrase from escape_from_ny.mid (0:21-0:25) replaces the CS-50 chime.
- Host tests pass (startup peak -13.8 dBFS); not yet tested on a physical FM-1.
- Device image 482,552 B; package 614,938 B.

## 012
- Hardware candidate: Escape From Firmware uses a sixteenth-step onset grid for CS-50 with baseline per-part event counts preserved (Juno/Piano/Pro-1/CS-50/drums = 204/76/35/242/393).
- Selected CS-50 onset error improves to 0.00 average / 0.04 maximum sixteenth steps.
- Physical FM-1 test passed; user confirmed the build is fine.
- Device image 481,952 B; package 614,938 B.

## 011
- GLO: HELP text is now highlighted yellow on the main, playback, and sequencer screens.
- Physical FM-1 test passed; user confirmed the GLO help labels work correctly.
- Device image 481,952 B; package 614,938 B.

## 010
- Added the built-in Escape From Firmware sequencer track and loading saved tracks from Playback for editing.
- GLO opens contextual help; the main help identifies PLAY as Playback and SEQ as the sequencer screen.
- Device image 481,952 B; package 614,938 B.

## 008
- All instrument names white (the selected one stays cyan).
- CHORD another 20% quieter (Juno chord voice compensated for the Juno boost).
- Juno 20% louder.
- SEQ indicator moved to the middle of the screen (drawn over the JC portrait).

## 007
- Song mode (EDIT+SEQ / EDIT+PLAY) and PLAY backing loop disabled (BACKING_ENABLED 0 in app.c) until the distortion is fixed; their tests moved to tests/disabled/. Later: try fewer instruments, e.g. guitar track off.
- SEQ now starts with a basic drum beat if the pattern is empty (before, an empty pattern was silent: only the SEQ icon showed). Record more by playing drum keys on LM-1 while it runs.
- CHORD instrument ~30% quieter.
- CHORD label is white like the other instruments.

## 006
- Song mode: Juno voice level 0.38 -> 0.26. Juno chords between ~14s and ~28s of the song were driving the master tanh into saturation (distortion); peaks are now below the saturation knee.

## 005
- Song mode: Juno voice level reduced 0.55 -> 0.38 so the song mix no longer drives the output tanh hard (peaks into tanh were ~1.8, now lower). Host song render RMS compared against the older engine.
## 004

- Song included again: builds 002 and 003 were made without dsp/song.h, so EDIT+SEQ song mode was missing and the default tempo was 108 instead of 80. 004 includes the song (image 475,448 B).
- No other firmware changes; all host tests now pass.

## 003

- Sound switching moved to the knobs: PRESET (or ALGO) now picks the guitar riff, bass sample, CS-50/CS-80 patch and CHORD voice; ALGO sets the CHORD layout (1 KEY / SPLIT). EDIT+K4 and EDIT+SELECT no longer switch sounds, so K4 is always the effect knob. SEL still steps variants.
- Bottom screen line now reads PRESET: <name> (guitar, bass, CS-50) or ALGO: <layout> (CHORD).
- Instrument list: "BASS / SAMP B" renamed "BASS".

## 002

- Guitar riffs replaced with three user-provided recordings (`r1`, `r11`, `r13`): EDIT+K4 / SEL cycle synth, Riff 1-3.
- Guitar riff playback is 3x longer: each riff loops its data three times, each pass quieter (no extra flash data).
- Drum sequencer: SEQ tap runs/stops a 4-track 16-step pattern, live-recorded while the drums are selected, saved with SAVE. SEQ-held key 3 clears it. Song mode moved to EDIT+SEQ.
- SEQ button light and an on-screen "SEQ" mark show the sequencer is running.
- Earlier this cycle: bass samples, louder Juno, function buttons, battery-dimmed JC image, Felucca-style quick layers (FX, GLO, SEQ, LFO).
- Image 443,508 B; package 614,938 B.
