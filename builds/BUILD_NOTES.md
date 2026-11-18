# Test build notes

Each test build is saved as `plissken###.fwsc` with an increasing number. Newest first.
Add an entry here whenever a new build is saved.

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
