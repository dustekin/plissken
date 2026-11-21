# Plissken

**HUMAN CONTENT**

WARNING - One user has reported bricking after installing Plissken. I have hardware tested Plissken multiple times, including here https://youtube.com/shorts/GOW90MxLPbM?feature=share. Please always use a bootloader!

Plissken is a live-performance instrument and sequencer prototype for the M-VAVE FM-1. It is built on `fm1-base`; the `base/`
directory is the build system and platform submodule. The keyboard offers Juno, piano, Pro-1, guitar, bass, LM-1
drums, CS-50/CS-80 and CHORD sounds, along with a live phrase recorder and a visual five-track sequencer with
saved-song playback.

**v0.15** adds The Fog as a preloaded track and centers the startup animation vertically. The older published,
signed v0.11 release does not include the visual sequencer and saved-song playback features described below.

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
**The Fog** sequences are independent, editable tracks. Saved-song playback is limited to four simultaneous sound
voices, so dense arrangements may omit events when that budget is reached.

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
  chord layouts are available through their on-screen controls.
- **LM-1:** thirteen drum samples are played from the keys or sequencer. Samples are one-shots and do not transpose.

Sample provenance, licenses, and processing details are in [CREDITS.md](CREDITS.md).

## Release history

Entries below summarize the available Git tags, README history, and build notes. They are intentionally limited to
documented changes.

| Version | Documented changes |
| --- | --- |
| **v0.20-rc1** | Release candidate: sampled piano is the default; guitar and bass gain the new sample timbres and sustain/one-shot modes; 808 is boosted; old guitar riffs and bass plunks are removed; the startup phrase's repeated low pulses are revoiced as E4. Local candidate package: build 023. |
| **v0.15** | Adds The Fog as a preloaded sequence and vertically centers the startup animation. Build 018 was hardware-tested. |
| **v0.14** | Adds Halloween as a preloaded sequence and balances its drum mix below the pitched parts. Build 017 was hardware-tested. |
| **v0.13** | Adds the animated startup splash and Pro-1 startup phrase; build 014 shortens the phrase after its stacked ending distorted. Build 014 was hardware-tested and the startup no longer distorted. |
| **v0.12** | Adds the visual five-track sequencer and saved-song Playback screen, including the Escape From Firmware track and 16 user slots. Build 012 was hardware-tested. |
| **v0.11** | Signed firmware baseline; it predates the visual sequencer and saved-song playback. |
| **v0.1** | Early tag. The tag history records the root MPL-2.0 license; no fuller release notes were found. |

The **v0.20-rc1** tag builds and publishes a GitHub prerelease with its firmware package and build attestation. The
final **v0.20** release tag has not been created.

## Numbered internal builds

There are 22 numbered packages in the current sequence, builds 002 through 023. Build 001 is not present in the
builds folder. Hardware status is marked as **tested**, **not tested**, or **not recorded**; missing notes are not
treated as evidence of a hardware test.

| Build | Summary | FM-1 hardware status |
| --- | --- | --- |
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

The editor has four pitched tracks and one LM-1 drum track.

- Turn **PRESET** to choose a track; turn **ALGO** to move the step cursor.
- Turn **SELECT** to choose the track's instrument or drum sample. The LM-1 sound is only available on the drum
  track.
- Press a piano key to play and insert or replace a note at the selected step. On the drum track, keys select and
  insert individual samples; different drum samples can share a step.
- Press **REC** to remove the selected note. On the drum track it removes only the selected drum sample; it does not
  add a sample.
- Press **SEL** to change the time signature. Turn **MASTER** to set the selected pitched note's length; drum-hit
  length follows the sample and tempo.
- Hold **EDIT** and turn **K1** to change the sequence tempo. This tempo is separate from the main-screen tempo.
- Press **PLAY** to start or stop the working sequence. During playback the display follows the play position; moving
  ALGO pauses that follow so you can edit another step.
- Press **SAVE** to name and save the sequence. When editing a saved song, SAVE updates that song. There are up to
  16 user songs plus the built-in Escape From Firmware, Halloween, and The Fog tracks.
- Press **SEQ** or **HOME** to return to the main screen. **GLO** opens sequencer help.

### PLAY screen

1. Press **PLAY** on the main screen to open the saved-song selector.
2. Turn **PRESET** to choose a song, then press **PLAY** to start or stop playback.
3. Press **REC** to delete the selected song. Deleting a built-in track restores it on the next boot.
4. Press any piano key to load the selected song into the SEQ editor. That first key only opens the editor and is not
   recorded. Use **SAVE** in the editor to write changes back to the selected song.
5. Press **HOME** or **SEQ** to leave Playback. **GLO** opens Playback help.

## Build and test

Build in the VS Code terminal opened in WSL (Ubuntu), with the JieLi toolchain and AC79 SDK configured as described
by `fm1-base`. Run commands from the repository root.

```sh
# Normal FM-1 development package
python3 base/tools/build.py .

# Versioned release-candidate package
python3 base/tools/build.py . --release 0.20-rc1

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

- Continue improving the preloaded songs, including renderer choices, voice allocation, mix balance, and FM-1
  listening tests.
- In CHORD SPLIT mode, make the second octave of keys playable in a future release. Its current non-playing behavior
  is intentional, not a bug.
- Rename the preloaded **The Fog** song to **The FMog** in a future release; the current firmware still calls it
  The Fog.
- Rebuild future songs using the sampled piano and the new guitar and bass instruments. Reuse the startup fix's
  anti-buzz approach by simplifying or revoicing stacked low notes instead of lowering the overall gain, then
  verify the result on an FM-1.
- Improve the live sound balance and quality of Juno, guitar, bass, and CHORD while staying within the FM-1's audio
  and voice limits.
- Add quality-of-life improvements to the sequencer and saved-song workflow.
- Optimize CPU, RAM, flash use, and voice allocation before adding more demanding features.
- Consider additional preloaded songs and original graphics. Keep local reference MIDI files out of the repository
  and verify permissions for any new samples, music, or artwork.

## Important firmware-installation warning

**For a real FM-1, follow the [Firmware Hub installation procedure](https://thefirmwarehub.com/kb/) and use the
M-Flasher Bootloader Installer. Confirm the package is intended for your exact FM-1 variant; never install the
emulator-only package. Do not interrupt power during flashing. The 47-key model requires the documented conversion:
an incorrect installation can permanently damage its calibration or brick the device.**
