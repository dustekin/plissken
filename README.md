# Plissken

**HUMAN CONTENT**

WARNING - One user has reported bricking after installing but thry didn't use F-Install. I highly recommend you don't use browser installers for firmware, they seem to cause problems! I have hardware tested Plissken multiple times, including here https://youtube.com/shorts/GOW90MxLPbM?feature=share. Please always use a bootloader!

Here is a guide to FM-Flasher bootloader https://youtu.be/6htjeFKx5Wo?is=Yi9R9PAuOM8itAnM

Plissken is a live-performance instrument and sequencer prototype for the M-VAVE FM-1. It is built on `fm1-base`; the `base/`
directory is the build system and platform submodule. The keyboard offers Juno, piano, Pro-1, guitar, bass, LM-1
drums, CS-50/CS-80 and CHORD sounds, along with a live phrase recorder and a visual five-track sequencer with
saved-song playback.

Current version allows you play a variety of instruments, write your own songs and edit versions of The Fog, Eacape from New York and Halloween themes. 
Future versions will include midi and export of songs, hopefully. 

It's a crappy little firmware, not meant to be serious, just wanted to prove I could do it with vibecoding.

ALWAYS FOLLOW INSTALL PROCEDURE LAID OUT IN https://thefirmwarehub.com/kb/  !!!
Use a bootloader! Never install on the 47 key without converting because you will damage your hardware calibration irreversibly!

Contact: raggadus@gmail.com

Please do what you like with the firmware, but give credit.

Content below this line is human edited

**HUMAN CONTENT END**

## Project overview

Plissken is a live-performance sound instrument and visual sequencer for the M-VAVE FM-1. The main screen is a
playable keyboard with several synthesized and sampled instruments, live phrase recording, and a five-track
sequencer. The saved-song screen plays and edits named sequences.

The original seven-part backing arrangement is disabled because its mix distorted. **PLAY** opens the saved-song
screen; it does not start that disabled arrangement. The preloaded **Escape From Firmware**, **Halloween**, and
**The FMog** sequences are independent, editable tracks. Dense sequencer arrangements can be created and saved;
editor preview and saved-song playback cap audio at five simultaneous voices using prioritized voice stealing.
Drums have the highest priority and Juno the lowest; a lower-priority incoming note can be skipped.

Build 026 experiments with **voice stealing** during Playback of the three untouched factory songs: it plays
additional or substituted notes and, when all four voices are busy, can cut an older voice to play a new one.
Drums take priority over piano, Pro-1, guitar and bass; Juno has the lowest priority. Lower-priority new notes can still be
skipped, and abrupt cuts may be audible. This does not create extra voices.

Builds 027 and 028 replace untouched factory Escape's Playback arrangement with the same denser MIDI-derived
arrangement, streamed from flash, using guitar, electric bass and rebalanced piano and synth parts. Build 027 tests
four voices; build 028 tests five for Escape only. Both worked and sounded good on hardware; the user preferred five.
The current source selects five via `ESCAPE_EXPERIMENT_VOICES`; set it to four to reproduce the 027 voice budget.
Build 034 tests a five-voice budget for Halloween and The FMog as well. In default host renders this eliminates
10 and six voice steals respectively; the user reports the songs sound great on hardware.
Escape's Playback screen shows the maximum sampled, smoothed CPU percentage and detected late-render count since
playback started. These indicators do not detect every timing fault.
Build 030 changes only this Escape Playback overlay's bass from Electric to NoMore; the saved editable arrangement
and other songs are unchanged. Build 031 changes it to 808 and reduces its level only in the Escape overlay;
build 032 uses Acoustic bass. These Playback bass parts follow the selected bass Sustain/Note mode.
Build 033 removes the MIDI bass part from Escape's factory Playback entirely, freeing the five-voice budget for
the other parts; the editable saved arrangement and other songs remain unchanged.
Build 036 is a listening comparison that removes 361 lower notes from matched piano octaves in that overlay;
the other 2,027 events retain their order. The user preferred it on hardware.

The factory-song Playback overlays do not change saved arrangements or their editor display. User-created songs and
edited built-ins play their saved arrangements with the same five-voice stealing budget. Chord events consume three
or four voices; when an event needs several voices, multiple older voices may be cut to make room. Individual
instrument pools can be smaller than five, so five notes of the same sound are not guaranteed. Older four-voice
firmware may reject denser songs; back them up before downgrading. The user reports the build 029 sequencer hardware
test passed.

The revised arrangements use piano for additional melodic parts, and electric bass in Halloween and The FMog.
Piano parts follow the currently selected sampled or additive piano engine. On upgrade, untouched original built-in
songs are updated automatically; edited built-ins are preserved. To restore a revised factory arrangement, delete
that built-in in Playback and reboot. Deletion discards its edits.

Build 036 removes the empty opening bar from untouched factory Escape and The FMog in both Playback and the editor.
Untouched v0.30 factory slots upgrade automatically; edited slots and user songs are preserved. Halloween's drum
intro is unchanged. Older firmware may no longer recognize the trimmed built-ins as factory songs after downgrade;
deleting a built-in to restore the older factory version discards edits.
Build 038 trimmed additional full empty opening bars from stored Escape songs when loading them. Build 044 removes
that load-time trim so edited Escape songs retain their intended opening silence; it cannot restore silence that
was already removed and saved.
Build 043 removes the first 30 sixteenth-note steps from the previously trimmed, untouched factory Escape, including
its opening Juno note, so the first LM-1 hit starts immediately in both the editor arrangement and factory Playback.
Untouched older factory versions upgrade automatically; edited Escape saves are not migrated. The shorter song is
padded to whole bars. Older firmware may not recognize the new factory arrangement after downgrade.
Build 044 also rejects note lengths that would exceed the 128-bar song limit and includes live editor keys in the
five-voice playback budget. Live notes may be skipped or cut according to the same priorities as sequenced notes.

## Known issues

- Pressing two piano keys in CHORD mode with JUNO or PRO-1 selected can cause distortion. In an earlier build 035
  test, no audible distortion was reported, but CPU and XRUN readings remained high. These indicators do not
  detect every timing fault.

## Instrument sounds

- **Piano:** the six-zone Kawai sample engine is the startup default. With PIANO selected, turn ALGO to switch
  between the sampled and additive engines.
- **Guitar:** the modeled instrument remains available alongside Electric, Nylon, and Acoustic samples. PRESET or
  ALGO cycles Model Sustain/One-shot, Electric Sustain/One-shot, Nylon Sustain/One-shot, and Acoustic
  Sustain/One-shot. SEL advances through the same cycle. Electric Sustain is the startup mode.
- **Bass:** cycles Model, Electric, Acoustic, 808, and NoMore, each with Sustain and Note modes. Electric Sustain
  is the startup mode. Sustain responds to key release; Note lets a sample play through its natural tail. The 808
  level is boosted. The former BassPlunk samples are removed.
- **Juno, Pro-1, CS-50/CS-80, and CHORD:** select these from the main instrument list. CS-80-inspired patches and
  chord layouts are available through their on-screen controls. In CHORD SPLIT, all keys play chords; F4–C5
  (keys 12–19, counting from zero) also select the chord type, while C#5–G5 retain the current type. The eight
  types are major, minor, sus2, sus4, dominant 7, major 7, minor 7, and diminished.
- **LM-1:** thirteen drum samples are played from the keys or sequencer. Samples are one-shots and do not transpose.

Sample provenance, licenses, and processing details are in [CREDITS.md](CREDITS.md).

## Release history

Entries below summarize the available Git tags, README history, and build notes. They are intentionally limited to
documented changes.

| Version | Documented changes |
| --- | --- |
| **v0.32** | Dense sequencer editing with five-voice stealing, bar navigation and copy/paste, improved save and Playback controls, a shorter factory Escape opening, and saved-song safety fixes. Based on hardware-tested build 044; the versioned package has not been separately hardware-tested. |
| **v0.31** | Lighter Escape Playback, five-voice Halloween and The FMog Playback, trimmed empty factory-song openings, and optimized JUNO/PRO rendering. The user reports the versioned package passed hardware testing. |
| **v0.30** | Five-voice sequencer and bass-free Escape Playback with voice stealing; prioritized stealing for untouched factory songs; revised piano and bass arrangements; CHORD SPLIT and version help improvements. The user reports the local v0.30 package passed hardware testing. |
| **v0.20-rc1** | Release candidate: sampled piano is the default; guitar and bass gain the new sample timbres and sustain/one-shot modes; 808 is boosted; old guitar riffs and bass plunks are removed; the startup phrase's repeated low pulses are revoiced as E4. Local candidate package: build 023. |
| **v0.15** | Adds The Fog as a preloaded sequence and vertically centers the startup animation. Build 018 was hardware-tested. |
| **v0.14** | Adds Halloween as a preloaded sequence and balances its drum mix below the pitched parts. Build 017 was hardware-tested. |
| **v0.13** | Adds the animated startup splash and Pro-1 startup phrase; build 014 shortens the phrase after its stacked ending distorted. Build 014 was hardware-tested and the startup no longer distorted. |
| **v0.12** | Adds the visual five-track sequencer and saved-song Playback screen, including the Escape From Firmware track and 16 user slots. Build 012 was hardware-tested. |
| **v0.11** | Signed firmware baseline; it predates the visual sequencer and saved-song playback. |
| **v0.1** | Early tag. The tag history records the root MPL-2.0 license; no fuller release notes were found. |

The **v0.20-rc1** tag builds and publishes a GitHub prerelease with its firmware package and build attestation. The
final **v0.20** release tag has not been created. Version tags trigger the workflow to build and attest a package.
Published release packages are named `onlyusefmflashertoinstallplissken.fwsc` to make the required FM-Flasher
installation method explicit. Numbered local hardware-test packages in `builds/` retain their `plissken###.fwsc`
names.

## Numbered internal builds

There are 44 numbered packages in the current sequence, builds 002 through 045. Build 001 is not present in the
builds folder. Hardware status is marked as **tested**, **not tested**, or **not recorded**; missing notes are not
treated as evidence of a hardware test.

| Build | Summary | FM-1 hardware status |
| --- | --- | --- |
| 045 | Adds per-note and bar sound changes, instrument colours, and safe working-song clear. | Tested; user reports it works |
| 044 | Preserves Escape opening silence, prevents invalid note lengths, and budgets live preview voices. | Tested; user reports it works fine |
| 043 | Starts untouched factory Escape at the first drum hit while preserving edited saves. | Tested; hardware test passed |
| 042 | Adds single-track bar copy and paste; fixes reverse track selection. | Tested; result not specified |
| 041 | SAVE updates the active song; EDIT + SAVE creates a user-slot copy. | Tested; hardware test passed |
| 040 | Adds EDIT + ALGO bar navigation in the sequencer. | Tested; hardware test passed |
| 039 | Adds Playback song-delete confirmation and corrects sequencer REC help. | Tested; user reports it was fine |
| 038 | Trims empty opening bars from Escape; EDIT and SEQ open the selected song in the sequencer. | Tested; user reports hardware testing was good |
| 037 | Allows dense sequencer arrangements; prioritized stealing keeps playback within five voices. | Not tested |
| 036 | Removes the empty opening bars in Escape and The FMog; compares a lighter Escape piano arrangement. | Openings passed; user preferred the alternative |
| 035 | Tests reduced JUNO and PRO render work, retaining the CHORD diagnostics. | No audible JUNO/PRO distortion; CPU/XRUN readings remain high |
| 034 | Tests five-voice Halloween and The FMog Playback and CHORD CPU/XRUN diagnostics. | Songs sound great; JUNO test reported XRUNs |
| 033 | Removes the MIDI bass part from Escape's factory Playback. | Tested; hardware test passed |
| 032 | Changes Escape's factory Playback bass to Acoustic. | Not tested |
| 031 | Changes Escape's factory Playback bass to 808. | Not tested |
| 030 | Changes Escape's factory Playback bass to NoMore. | Not tested |
| 029 | Raises the sequencer and saved-song limit to five voices. | Tested; sequencer test passed |
| 028 | Tests the rebuilt Escape arrangement with five voices and stealing. | Tested; user preferred it |
| 027 | Tests the rebuilt Escape arrangement with four voices and stealing. | Tested; user reports good |
| 026 | Tests prioritized voice stealing during Playback of untouched factory songs. | Tested; user reports good |
| 025 | Renames The Fog to The FMog and revises the built-in piano and bass arrangements. | Not tested yet |
| 024 | Makes CHORD SPLIT upper keys playable and shows the build version in main-screen help. | Tested; SPLIT and version display work |
| 023 | v0.20-rc1 release-format candidate package. | Not tested yet |
| 022 | Restores startup gain and replaces repeated low E2/E3 pulses with a single E4 pulse. | Tested; user confirmed it works fine |
| 021 | Lowered startup phrase gain to investigate the buzz. | Tested; buzz remained |
| 020 | Adds Electric/Acoustic bass samples, Sustain/Note modes, and a louder 808; removes BassPlunk samples. | Tested; main sounds were good, startup phrase buzzed |
| 019 | Package exists, but no build notes were found. | Not recorded |
| 018 | Adds The Fog and centers the startup splash vertically. | Tested |
| 017 | Rebalances Halloween's sequenced drums below its pitched parts. | Tested |
| 016 | Tests rendering Halloween's lowered piano lane with Juno. | Not tested |
| 015 | Adds Halloween as an editable preloaded sequence. | Not tested |
| 014 | Shortens the startup phrase and removes its distorted closing chord. | Tested |
| 013 | Adds the startup scanline animation and Pro-1 phrase. | Not tested |
| 012 | Improves the Escape From Firmware sequence's CS-50 onset timing. | Tested |
| 011 | Highlights GLO help labels on the main, playback, and sequencer screens. | Tested |
| 010 | Adds Escape From Firmware and editing a saved song from Playback. | Not recorded |
| 009 | Package exists, but no build notes were found. | Not recorded |
| 008 | Adjusts instrument labels and levels; repositions the SEQ indicator. | Not recorded |
| 007 | Disables the distorted backing arrangement and adds a basic empty-pattern drum beat. | Not recorded |
| 006 | Reduces Juno level in the backing-song mix. | Not recorded |
| 005 | Further reduces Juno level in the backing-song mix. | Not recorded |
| 004 | Restores the built-in song data missing from builds 002 and 003. | Not recorded |
| 003 | Moves sound-variant switching to the knobs and adds the CHORD layout control. | Not recorded |
| 002 | Early guitar/bass samples, drum sequencer, and quick-layer controls. | Not recorded |

For detailed measurements and package hashes, see [builds/BUILD_NOTES.md](builds/BUILD_NOTES.md). The firmware
packages in `builds/` are local test artifacts and are not tracked by Git.

## Using the FM-1

### Main screen

1. Turn **SELECT** to choose an instrument. Play the piano keys to hear it.
2. Use **OCT- / OCT+** to shift a pitched instrument by one octave, within two octaves in either direction.
3. Use **K1-K4** for the selected instrument's sound controls. For LM-1, K1-K3 set chorus, reverb, and compression;
   K4 sets drum bass boost. Guitar and bass have independent drive, chorus, modulation, and delay controls.
4. With **PIANO** selected, turn **ALGO** to switch between the sampled and additive engines. The sample engine
   starts selected after reboot.
5. With **GUITAR** selected, PRESET or ALGO cycles the eight Model/Electric/Nylon/Acoustic Sustain/One-shot modes;
   SEL cycles them as well. With **BASS** selected, PRESET, ALGO, or SEL cycles the five bass timbres and their
   Sustain/Note modes.
6. **EDIT + K1** changes the main tempo from 70 to 140 BPM. **SAVE** keeps that tempo across reboot.
7. **REC** records a live phrase; press it again to stop and loop, then again to stop the loop. **EDIT + REC** clears
   the phrase. The phrase is held in RAM and is lost at reboot.
8. **MASTER** controls output level. **ARP** toggles the arpeggiator. **GLO** opens on-screen help.
9. Press **SEQ** to edit the working sequence or **PLAY** to open saved-song playback.

### SEQ screen

The editor has four pitched tracks and one LM-1 drum track and shows one whole bar at a time; the number of grid
steps depends on the time signature.

- Turn **PRESET** to choose a track; turn **ALGO** to move the step cursor. Hold **EDIT** while turning **ALGO**
  to jump to the beginning of another bar. Moving beyond the end extends the song, up to 128 bars.
- Turn the **SELECT encoder** to change the pitched track's default instrument and variant for new notes; existing
  notes keep their sounds. On an existing note the selected note's sound remains displayed; move to an empty step
  to use the default. LM-1 is only offered on the drum track, where SELECT chooses a drum sample.
- Hold **EDIT** and turn **SELECT** to change only the selected pitched note's instrument or variant. At an empty
  step, this chooses the sound for the next note there; moving away restores that track's default. Note colours show
  their instrument: Juno cyan, piano green, Pro-1 pink, guitar orange, bass red, LM-1 purple, CS-50/CS-80 yellow
  and CHORD white. The track label shows its default; GLO help also shows the colour legend.
- With playback stopped, hold **EDIT** and press **PLAY** to apply the currently selected sound and variant to
  all notes starting in the current pitched track and bar. Press **PLAY** to confirm, or **HOME** or **SEQ** to
  cancel. Other tracks, bars and notes sustained in from a preceding bar are unchanged; drums cannot be changed
  this way.
- Press a piano key to play and insert or replace a note at the selected step. On the drum track, keys select and
  insert individual samples; different drum samples can share a step.
- Press **REC** to remove the selected note. On the drum track it removes only the selected drum sample; it does not
  add a sample.
- Press **SEL** to change the time signature. Turn **MASTER** to set the selected pitched note's length; drum-hit
  length follows the sample and tempo.
- Hold **EDIT** and turn **K1** to change the sequence tempo. This tempo is separate from the main-screen tempo.
- With playback stopped, hold **EDIT** and press **REC** to copy the selected track's current bar. Move to a
  destination bar and press **EDIT + SEL** to paste; if it contains notes, press **SEL** to replace them or
  **HOME** or **SEQ** to cancel. Pitched bars paste to pitched tracks and drums to the drum track, with the same
  time signature. Copy and replacement cover events that start in the bar; notes spanning into it are unchanged.
  Copying an empty bar can clear a destination after confirmation. The clipboard remains available until reboot
  and is not saved with the song.
- Press **PLAY** to start or stop the working sequence. During playback the display follows the play position; moving
  ALGO pauses that follow so you can edit another step. Press PLAY twice quickly to stop and return to the start.
- Press **SAVE** to name and save the sequence; subsequent saves update that song. Hold **EDIT** and press **SAVE**
  to name a copy in a free user slot and continue editing the copy. Built-in copies use the editable arrangement,
  not the separate factory Playback overlay. If all user slots are full, free one in Playback before saving a copy.
  There are 16 user slots plus three built-in songs.
- Hold **EDIT** and press **HOME** to clear the working song; **SEL** confirms and **HOME** or **SEQ** cancels.
  Playback stops when the prompt opens, even if cancelled. Clearing discards unsaved edits and detaches the working
  song from its saved slot, but does not delete or overwrite that slot; the next SAVE creates a new user song.
- Untouched factory songs may use a separate Playback overlay. Editing and saving a built-in can replace that
  Playback sound with its editable arrangement; use **EDIT + SAVE** to preserve the built-in as a separate copy.
- Press **SEQ** or **HOME** to return to the main screen. **GLO** opens sequencer help.

### PLAY screen

1. Press **PLAY** on the main screen to open the saved-song selector.
2. Turn **PRESET** to choose a song, then press **PLAY** to start or stop playback.
3. Press **REC** to request deletion, then **REC** again to confirm, or **HOME** or **SEQ** to cancel.
   Playback stops when the prompt opens. Deleting a built-in track restores it on the next boot.
4. Press **EDIT** or **SEQ** to load the selected song into the SEQ editor. Pressing a piano key also opens the editor;
   that first key only navigates and does not play or insert its note. Use **SAVE** in the editor to write changes back.
5. Press **HOME** to leave Playback. **GLO** opens Playback help.

## Build and test

Build in the VS Code terminal opened in WSL (Ubuntu), with the JieLi toolchain and AC79 SDK configured as described
by `fm1-base`. Run commands from the repository root.

```sh
# Normal FM-1 development package
python3 base/tools/build.py .

# Versioned v0.32 release package
python3 base/tools/build.py . --release 0.32

# Host simulator and scenario tests
python3 base/tools/build.py . --host
for test in tests/*.txt; do
  build/host/plissken_host "$test" build/host
done
```

The release workflow runs on version tags, builds and attests the package, and attaches it to the GitHub release.
Tags containing `-rc` are marked as prereleases.

The browser emulator requires a separate compatibility build:

```sh
python3 base/tools/build.py . --emulator-only
```

It outputs `build/emulator-only/dont-install-on-your-fm1-plissken-emulator-only.fwsc`. **Never install this package
on an FM-1.** It is a real firmware package with emulator-specific clock handling; its warning label is not an
installation lockout. Use the normal hardware-target build for the device.

## License and credits

Plissken's application code is licensed under [MPL-2.0](LICENSE). Third-party code and assets may have different
terms; see [CREDITS.md](CREDITS.md) and [base/LICENSING.md](base/LICENSING.md).

## Future development

- Revisit the sequencer controls: the new controls work but feel clunky to use.
- Investigate and fix distortion when playing two keys in CHORD mode with JUNO or PRO-1, including the timing load.
- Target full song sharing for v0.40 or v0.50, depending on implementation complexity.
- Continue improving the preloaded songs, including renderer choices, voice allocation, mix balance, and FM-1
  listening tests.
- Continue refining songs with the sampled piano and new guitar and bass instruments. Reuse the startup fix's
  anti-buzz approach by simplifying or revoicing stacked low notes instead of lowering the overall gain, then
  verify the result on an FM-1.
- Improve the live sound balance and quality of Juno, guitar, bass, and CHORD while staying within the FM-1's audio
  and voice limits.
- Add MIDI step-sequencer output for built-in and user-created songs, with tracks assignable to external instruments.
- Explore importing and exporting user-created songs to phones or computers, plus a buildable Windows/Linux player
  using the firmware's instruments. Investigate desktop conversion to MIDI or an FL Studio-readable DAW project.
- Optimize CPU, RAM, flash use, and voice allocation before adding more demanding features.
- Consider additional preloaded songs and original graphics. Keep local reference MIDI files out of the repository
  and verify permissions for any new samples, music, or artwork.

## Important firmware-installation warning

**For a real FM-1, follow the [Firmware Hub installation procedure](https://thefirmwarehub.com/kb/) and use the
M-Flasher Bootloader Installer. Confirm the package is intended for your exact FM-1 variant; never install the
emulator-only package. Do not interrupt power during flashing. The 47-key model requires the documented conversion:
an incorrect installation can permanently damage its calibration or brick the device.**
