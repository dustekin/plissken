# Plissken

Always use a M-Flasher to install Plissken and then to install firmware over it. Your FM1 can and will brick
randomly.

Plissken is an instrument and sequencer engine. It plays only three built in songs.

The rest you make yourself. It is not a self playing grovebox. 

It is an early prototype: one shared clock drives an eight-voice Juno-style synth with chord stabs and stereo chorus, 
piano chords and notes, a Pro One-style line, simple guitar power chords, electric bass, and 13 LM-1 drum samples.

## Controls

| Control | Function |
| --- | --- |
| PLAY | Open the saved-song selector; start or stop the selected song |
| SELECT encoder | Choose the sound played by the piano keys: Juno, piano, Pro-1, guitar, bass, LM-1 drums, CS-50/CS-80 or CHORD |
| PRESET / ALGO knobs | Switch instrument variants: bass, guitar modes, CS-50/CS-80 patches, piano engine (ALGO), and CHORD voice/layout |
| SEQ | Open the visual sequencer editor |
| EDIT (tap) | Enter Groovebox; exit again with EDIT or HOME |
| FX / SEL | FX cycles the selected sound's effect setting; SEL steps through guitar modes, bass, CS-80 and CHORD variants |
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

## Groovebox

Groovebox is experimental and basic. Its patterns, transitions, controls, and mix are expected to evolve after
more hardware listening.

A short EDIT press on the main screen opens Groovebox with generated Action or Horror music. PRESET selects the
style, SELECT changes the live lead through Juno, piano, Pro-1, guitar, bass, CS-50/CS-80 and CHORD while excluding
LM-1 drums, and SEL holds the current variation instead of changing it automatically every
four bars; automatic changes introduce a new melody, bass rhythm/register, drum accents and transition fill.
Press SEL again to resume automatic changes. ALGO or K4 sets energy, K1 sets tempo, K2 changes swing,
and K3 transposes the generated music. REC schedules a fill. PLAY starts or stops the accompaniment. GLO opens
the Groovebox help screen without stopping playback. Piano keys remain available as a live lead using the
currently selected Plissken instrument.
From Playback, EDIT enters Groovebox; SEQ or a piano key still opens the selected song in the sequencer.

Action has syncopated LM-1 rhythm, Pro-1 arpeggios, piano stabs and restrained bass at 132 BPM. Horror begins at
92 BPM with sustained CS-50, piano stabs, sparse bass and drums, minor-second Juno tension and descending Pro-1
fills. Startup and four-bar transitions randomly select sections without immediately repeating the current one.
Generated and live notes fall down the display in Plissken's instrument colours. EDIT or HOME exits, releases all
live notes and restores the previous main-screen tempo. Groovebox is generated from original patterns rather than
playing the supplied reference recordings or reproducing their MIDI melodies.

The sequencer displays one whole bar at a time; the number of steps depends on the time signature.
In the sequencer editor, PRESET chooses one of four pitched-sound tracks or the fifth LM-1 track, ALGO moves the step cursor, SELECT sets the pitched track's
default sound for new notes, SEL cycles the time signature, and moving MASTER changes only the selected pitched note's length. Hold EDIT
and turn ALGO to jump to the start of another bar; jumping beyond the song's end extends it up to 128 bars.
Piano keys play and
insert or replace the note at the cursor, including during playback. REC removes a selected note when playback is
stopped. OCT− / OCT+ change the octave of pitched sounds; note, octave and K1–K4 effect changes briefly appear
as text. PLAY starts or stops the working sequence. GLO opens the larger-text help, where any knob changes pages;
GLO, SEQ or HOME returns to the editor. SEQ or HOME exits the editor. SAVE prompts for a name and stores the song;
later saves update that song. EDIT + SAVE prompts for a name and saves a copy in a free user slot, then edits the
copy instead. A built-in copy uses its editable arrangement, not its separate factory Playback overlay.
If all user slots are full, delete a song in Playback first.
Each pitched note stores its own instrument and variant. Changing a track default with the SELECT encoder does not
change existing notes; the selected note's sound stays displayed until the cursor moves. Hold EDIT and turn SELECT
to change only the selected note, or to choose the sound for a new note at an empty step. Moving away from an empty
step restores the default. Notes are coloured by instrument: Juno cyan, piano green, Pro-1 pink, guitar orange,
bass red, LM-1 purple, CS-50/CS-80 yellow, and CHORD white. Track labels show defaults; GLO has a colour legend.
With playback stopped, EDIT + PLAY opens a confirmation to apply the selected sound and variant to notes starting
in the current pitched track and bar. PLAY confirms; HOME or SEQ cancels. Other bars and tracks and notes sustained
from an earlier bar are unchanged. The drum track is excluded.
EDIT + HOME stops playback and prompts to clear the working song. SEL confirms; HOME or SEQ cancels, leaving
playback stopped. Clearing discards unsaved edits and detaches the working song from its saved slot without deleting
it, so the next SAVE creates a new user song.
Untouched factory songs may use a separate Playback overlay; editing and saving a built-in can switch Playback to
its editable arrangement. Use EDIT + SAVE to preserve the built-in and make a user copy.
During playback the display and cursor follow the music; moving ALGO pauses follow for editing. Stop and restart
playback to resume following. Press PLAY twice quickly to stop and return to the start. On the LM-1 track, SELECT chooses a sample and piano keys choose and insert their
individual samples. Different drum samples can share a step; REC removes only the selected sample.
With playback stopped, EDIT + REC copies the selected track's current bar, and EDIT + SEL pastes it at the
selected destination bar. Pitched bars can be pasted to pitched tracks and drum bars to the drum track, provided
the time signature matches. If the destination contains events, SEL confirms replacement and HOME or SEQ cancels.
Only events starting in each bar are copied or replaced; notes spanning into it are unchanged. An empty copied bar
can clear an occupied destination after confirmation. The clipboard remains available across song loads until
reboot and is not saved with the song.
New drum hits play the full sample, and their displayed length follows the sample duration and song tempo; MASTER
does not change drum lengths. LM-1 is not offered in the other tracks' sound choices. Existing saved songs remain
playable with their original drum lengths.
Up to 16 user songs can be saved, plus the built-in Escape From Firmware, Halloween, and The FMog tracks. To play
The FMog, press PLAY, turn PRESET to select it, then press PLAY again. Press EDIT or SEQ to open the selected song
for editing. Pressing a piano key also opens it; that first key is only navigation and is not played or recorded.
To delete a selected song, press REC, then REC again to confirm; HOME or SEQ cancels. Playback stops when the
confirmation opens.
Untouched original built-ins update automatically; edited built-ins are
preserved. Build 036 removes the empty opening bar from untouched factory Escape and The FMog, including their
editor views; Halloween's drum intro is unchanged. Older firmware may not recognize the trimmed built-ins as
factory songs after downgrade. Deleting a built-in discards its edits and restores the current factory version on the next boot. Piano
parts use the selected sampled or additive engine. Each track
supports up to 950 events and 128 bars. Dense arrangements can be entered and saved. Editor preview and saved-song
Playback are capped at five simultaneous sound voices; chord events count as three or four voices, pitched samples
are cut off at their selected duration, and drum tails reserve voices conservatively at the fastest tempo.
Build 038 trimmed additional full empty opening bars from stored Escape songs when loading them. Build 044 removes
that load-time trim so edited Escape songs retain their intended opening silence; it cannot restore silence that
was already removed and saved.
Build 043 removes 30 steps from the previously trimmed factory Escape opening, including its initial Juno note,
so both the editor arrangement and factory Playback begin at the first LM-1 hit. Untouched older factory versions
upgrade automatically; edited Escape saves are not migrated. The shortened song is padded to whole bars, and older
firmware may not recognize it as a factory song after downgrade.
Build 044 rejects note lengths beyond the 128-bar limit and counts live editor keys in the five-voice playback
budget. Live notes may be skipped or cut according to the same priorities as sequenced notes.

When a new event would exceed the five-voice budget, Playback may cut older voices to make room. Drums have the
highest priority; piano, Pro-1, guitar, bass and CS sounds share the middle priority; Juno has the lowest.
Lower-priority incoming events can still be skipped, and chord events may require cutting multiple voices. This
applies to user songs, edited built-ins and editor preview as well as untouched factory songs. Factory Playback
overlays do not change their saved arrangements.

Internal builds 027 and 028 stream a denser Escape arrangement from flash during untouched factory Playback;
the editable saved arrangement is unchanged. Build 027 uses four voices and build 028 tests five for Escape only.
Build 029 also allows five voices in the sequencer; build 034 tests five in Halloween and The FMog's factory overlays.
Build 030 changes Escape's factory Playback bass from Electric to NoMore, following the selected bass Sustain/Note
mode. Build 031 uses 808 at a reduced level in that overlay; build 032 uses Acoustic bass. Build 033 removes the
MIDI bass part from the five-voice Escape Playback overlay. Build 036 compares an overlay with the lower notes of
matched piano octaves removed; its piano thinning does not alter the editable saved arrangement.
Individual instrument voice pools can be smaller than five. Older four-voice firmware may reject dense songs; back
up saved songs before downgrading.
Escape's Playback screen shows the maximum sampled, smoothed CPU percentage and detected late renders (XRUN)
since playback started. A zero XRUN count does not guarantee timing safety; listen and test on hardware.

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

With **PIANO** selected, turn **ALGO** to switch between the additive engine and the sampled piano.
The sampled mode is selected by default after reboot. It has been tested on the FM-1 and runs fine; turn ALGO again
to select the additive piano.

With **GUITAR** selected, PRESET or ALGO cycles these eight modes: modeled sustain, modeled one-shot, Electric sustain,
Electric one-shot, Nylon sustain, Nylon one-shot, Acoustic sustain, and Acoustic one-shot. SEL advances through the
same sequence. Electric sustain is selected at startup. Sustain modes respond to key release; one-shot modes let the
plucked sound finish its natural tail. The guitar timbre choices are also available to sequencer tracks.

With **BASS** selected, PRESET, ALGO or SEL cycles modeled sustain/note, Electric sustain/note, Acoustic sustain/note,
808 sustain/note, and NoMore sustain/note. Electric sustain is selected at startup. Sustain responds to key release;
Note lets the sample finish its natural tail. The 808 sample has an increased level. The old Plunk samples and their
sequencer variants have been removed.

The screen lists the instruments in white, with the selected keyboard sound in cyan-blue. The small bars beside the
names are activity meters only. In the sequencer, the editor grid displays tracks and note lengths. CHORD plays one
chord per key; K1 sets the chord type and ALGO chooses the 1 KEY or SPLIT layout. In SPLIT, all keys play chords:
F4–C5 (keys 12–19, counting from zero) also select major, minor, sus2, sus4, dominant 7, major 7, minor 7, or
diminished respectively; C#5–G5 use the selected type. The sequencer uses the current chord controls on playback;
chord type is not stored per event. The first page of main-screen GLO help shows the build's release version, or DEV
for unversioned builds.
At startup, the display shows the color skyline artwork from `assets/startup.png`, centered vertically and packed into
a four-color RGB565 palette, for 3 seconds. A cyan scanline sweeps down the image while the Pro-1 voice plays a short
keyboard phrase.

## Current limitations

The preloaded MIDI/backing arrangement is disabled in this build because it distorted; it may return after further
work on gain staging and instrument count. PLAY instead selects and plays songs saved in the visual sequencer.
Escape From Firmware, Halloween, and The FMog are the preloaded sequencer tracks; additional preloaded MIDI selection
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
