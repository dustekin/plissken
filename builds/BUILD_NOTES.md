# Test build notes

Each test build is saved as `plissken###.fwsc` with an increasing number. Newest first.
Add an entry here whenever a new build is saved.

## v0.32 release preparation
- Local `0.32` FM-1 package is based on hardware-tested build 044. The versioned package has not been separately
  hardware-tested. Dense song handling, editor voice limits, saved-song safety, and the Escape opening are included.
- All 60 top-level host scenarios, ten audio checks and ten MIDI converter tests passed for the same firmware
  source; the normal versioned FM-1 build passed. A committed-source snapshot produced the same package SHA-256.
  CHORD JUNO/PRO timing load remains under investigation.
- Local device image 468,420 B; RAM .data+.bss 66,148 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981003200`; local build ID `70489439`, snapshot build ID `DE99FCE5`; package SHA-256
  `B8440EE6AC99FDEF855E70B9AD238BC74853F6FE52930390A297D4138CAC6137`.

## 044
- Local `0.32-test044` preserves edited Escape openings instead of trimming them at load, rejects notes extending
  beyond 128 bars and validates songs before saving, and applies the playback voice budget to live editor keys.
  Reused guitar and bass sample voices no longer respond to an earlier key's release.
- All 60 top-level host scenarios, ten audio checks and ten MIDI converter tests passed; the normal FM-1 target
  build passed. The user reports it works fine on hardware.
- Device image 468,436 B; RAM .data+.bss 66,148 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981003200`; build ID `653AB456`; package SHA-256
  `2D0EF652AD30337216597F3BFE70A4242445011B0DD490C6BECBD99DA10DB8E4`.

## 043
- Local `0.32-test043` starts untouched factory Escape at its first LM-1 drum hit in both the editor and Playback.
  Older untouched factory versions migrate automatically; edited Escape saves do not receive the new cut. At that
  build, full-empty-bar normalization still applied when loading Escape.
- All 57 top-level host scenarios, ten audio checks and ten MIDI converter tests passed; the normal FM-1 target
  build passed. The user reports the hardware test passed.
- Device image 468,484 B; RAM .data+.bss 66,148 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981003200`; build ID `092F331C`; package SHA-256
  `2ABE075E8C12810BB604435DDF5FBDB10DEAFFF01194079B41AEC505A1954FBA`.

## 042
- Local `0.32-test042` adds single-track bar copy and paste using EDIT + REC and EDIT + SEL. Occupied destination
  bars require confirmation; compatible tracks and matching meters are required. Fixes reverse track selection.
- All 54 top-level host scenarios passed, including pitched and layered drum bars, cancellation, limits, 5/4,
  clipboard persistence and saved-song behavior. The normal FM-1 target build passed. The user reports hardware
  testing; the result was not specified.
- Device image 468,244 B; RAM .data+.bss 66,148 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981003200`; build ID `C0013778`; package SHA-256
  `9E45CE4730A5ADAC32B6C4A16D5093BC5BFD12CCB1D92BB5864CC395B53D4B6D`.

## 041
- Local `0.32-test041` makes subsequent saves update the active song, and EDIT + SAVE creates a named user-slot
  copy that becomes active. Built-in copies use the editable arrangement. Full user slots produce a warning.
- Twelve focused host scenarios passed, including copy persistence, cancelled and failed saves, full slots,
  built-in preservation and existing saved-song behavior. The normal FM-1 target build passed; the user reports the
  hardware test passed.
- Device image 466,880 B; RAM .data+.bss 62,324 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981003200`; build ID `9FA2E108`; package SHA-256
  `C818490022AF340244597B09AF7D207F54074CF901DF2E01E8B3259EAC7637A9`.

## 040
- Local `0.32-test040` adds EDIT + ALGO bar navigation in the sequencer. Jumps land at the target bar's start
  and extend the song when necessary, up to 128 bars. During playback, jumping pauses display follow.
- Five focused host scenarios passed, covering both directions, multiple meters, extension, limits, and playback;
  the normal FM-1 target build passed. The user reports the hardware test passed.
- Device image 466,804 B; RAM .data+.bss 62,324 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981003200`; build ID `AB58368D`; package SHA-256
  `D8301E794A068C0135C266E2906B4EE738481DF7A6CB37ABA48C4A759CD4E6DD`.

## 039
- Local `0.32-test039` requires confirmation before deleting a song from Playback; HOME or SEQ cancels.
  Playback stops when the prompt opens. Sequencer REC help now correctly says it deletes notes.
- Four focused host scenarios passed, including cancellation, failed deletion and retry, user and built-in songs,
  and restoration of deleted built-ins. The normal FM-1 target build passed. The user reports hardware testing was fine.
- Device image 466,596 B; RAM .data+.bss 62,324 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981003200`; build ID `B5A11A61`; package SHA-256
  `6EDD858B5FF531F5660CFBA35163C4B587FE5A9D2BCD2F4D563F28C22BB1D05B`.

## 038
- Local `0.32-test038` trims additional full leading empty bars from stored Escape songs when loaded in the editor or Playback.
  EDIT and SEQ now open the selected Playback song in the editor; the first piano key only navigates and inserts no note.
- All 46 host scenarios, ten audio checks, and ten MIDI converter tests passed; the normal FM-1 target build passed.
  The user reports hardware testing was good.
- Device image 466,284 B; RAM .data+.bss 62,324 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981003200`; build ID `897D82DD`; package SHA-256
  `B0B87E6BAAB5DBB434D0AD19C9A64A90AD871FAA6F1D9E8135783BA9E3AE314A`.

## 037
- Local `0.32-test037` allows dense sequencer arrangements to be edited and saved; playback, including editor preview,
  uses prioritized stealing to stay within five voices. Chord events may steal multiple lower-priority voices.
- All 45 host scenarios, ten audio checks and MIDI converter tests passed; the normal FM-1 target build passed.
  Not hardware-tested.
- Device image 465,984 B; RAM .data+.bss 62,324 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981003200`; build ID `63DA4305`; package SHA-256
  `499D1EA701120EC2B4D5451E54F63CEB4C7C3318EC6AD2CFF39DF8C594DE7525`.

## v0.31 release preparation
- Local normal FM-1 package `build/plissken-0.31.fwsc` retains hardware-tested build 036's preferred Escape
  arrangement and trimmed openings, but removes the temporary CHORD CPU/XRUN display. The user reports that the
  versioned package passed all requested hardware checks, with no issues for now. Earlier measured CHORD XRUNs
  remain an unresolved timing risk.
- All 45 top-level host scenarios, ten audio checks, converter tests and the normal FM-1 target build passed.
  A source snapshot excluding ignored local files produced the same package SHA-256 as the local build.
- Device image 466,392 B; RAM .data+.bss 62,324 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981003100`; local build ID `E0CBC664`, clean-source build ID `909FB528`; package SHA-256
  `B5CC4BD43A1F26E97C3F9CC2125DD8D0A505D1D906871038E583C0025528241E`.

## 036
- Local `0.31-test036` removes one empty opening bar from untouched factory Escape and The FMog in Playback
  and the editor; their v0.30 factory slots are migrated on boot. Edited built-ins and user songs are preserved.
  Halloween starts with a hi-hat and retains its drum intro. Older firmware may treat trimmed built-ins as edited.
- Escape Playback is an optional listening comparison: 361 lower notes in matched piano octave pairs are omitted,
  retaining the other 2,027 bass-free MIDI-derived events and their order. The editable arrangement is not thinned.
  Default host render: 415 steals, 82 dropped onsets, pre-tanh peak 1.242, 62 frames at or above 1.0, no clipped
  output samples (build 035: 497, 146, 1.242, 189). First audible samples occur about 0.009 s after start for
  Escape and 0.0003 s for The FMog, versus about 3.009 s and 1.952 s in 034. These are host observations.
- All 45 top-level host scenarios, ten audio checks, converter tests and the normal FM-1 target build passed;
  migration, edited-song preservation, write-failure retry and legacy upgrades were tested. On hardware, the user
  preferred this Escape arrangement, confirmed the openings and reported no other issues.
- Device image 466,640 B; RAM .data+.bss 62,324 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981003100`; build ID `27759D58`; SHA-256
  `C4A817FE18722C36E552341D6778AC178A66B0C9B99D785D3E6062EC6D4811BA`.

## 035
- Local `0.31-test035` retains build 034's five-voice Halloween/The FMog Playback and CHORD diagnostics. It avoids
  redundant JUNO oscillator correction, zero-resonance ladder feedback, and duplicate PRO vibrato evaluation.
  The user reports no audible distortion with two-key JUNO or PRO and considers the issue corrected for now.
  Reported readings were JUNO 125% CPU / 115 XRUNs and PRO 90% CPU / 90 XRUNs; timing load remains a concern.
- Three two-key CHORD host renders (JUNO, PRO and PRO with vibrato) match 034 sample-for-sample. All 43 top-level
  host scenarios, ten audio checks and MIDI converter tests passed, as did the normal FM-1 target build.
  Hardware-tested as described above.
- Device image 467,916 B; RAM .data+.bss 62,324 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981003100`; build ID `0CA77458`; SHA-256
  `DB8AA065A3AFD81D089E9926923997191CCF7C46031302D5EC7A993FB28196C9`.

## 034
- Local `0.31-test034` tests five-voice factory Playback for Halloween and The FMog. In default host renders,
  Halloween's steals fall from 10 to zero and The FMog's from six to zero, with zero dropped onsets in both.
  Their events and saved arrangements are unchanged. Escape remains unchanged.
- Temporary CHORD diagnostic displays peak sampled, smoothed CPU and detected XRUNs since selecting CHORD.
  Select another instrument and return to CHORD to reset them. The diagnostic redraws every 250 ms and cannot
  detect every timing fault; host output does not establish the cause of the reported hardware distortion.
- All 43 top-level host scenarios, ten audio checks, MIDI converter unit tests and the normal FM-1 target build
  passed. The three full default host renders have no clipped samples. Hardware: the user reports both songs
  sound great; two-key CHORD JUNO reported 105% CPU and 354 XRUNs. A PRO observation was 10% CPU and 315 XRUNs;
  its key count/reset state was not clear.
- Device image 468,040 B; RAM .data+.bss 62,324 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981003100`; build ID `537D23A7`; SHA-256
  `3B317B422E3C9A7E278ED3C8E37D536A716A770D2A8B5720128C7BBD058D8370`.

## v0.30 release preparation
- Local normal FM-1 release-format package `build/plissken-0.30.fwsc`, based on hardware-tested build 033.
  The user reports the v0.30 hardware test passed; the release version display was not specifically confirmed.
- All 43 current host scenarios, ten audio checks and the MIDI converter unit tests passed. The target build passed;
  its identity is `FM-1_1981003000`, and `0.30` is present in the device image.
- Device image 467,940 B; RAM .data+.bss 62,340 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  build ID `423E273E`; SHA-256
  `55659DADF3E28977936DFDAAA19921A7E82533DDAA7BBEB0CDFEF79FA78F0D82`.
- The package was built locally before the work was committed. Five-voice songs cannot be loaded by older
  four-voice firmware. The earlier CHORD PIANO report was corrected: the user reports two-key CHORD JUNO and PRO
  distortion in SPLIT mode; CHORD PIANO is fine. The cause remains uninvestigated.

## 033
- Local `0.20-test033` rebuilds untouched factory Escape's Playback without the MIDI bass part, retaining five-voice
  prioritized stealing. All 2,388 remaining events and their ordering are unchanged from 032; 105 bass events were
  removed. The editable saved arrangement and other songs are unchanged.
- Default host render: 497 steals, 146 dropped onsets, pre-tanh peak 1.242, 189 stereo frames at or above 1.0;
  output WAV has no clipped samples. The full-song no-bass, alternate piano, sequencer and other factory-song
  regressions passed. Normal FM-1 target build passed; the user reports the hardware test passed.
- Device image 499,848 B; RAM .data+.bss 62,324 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981002000`; build ID `3EA356D3`; SHA-256
  `568B399122747394E78A13B38097EB913BE8F9395E41757EF0F2A5A478E91B2F`.

## 032
- Local `0.20-test032` changes untouched factory Escape's Playback bass from 808 to Acoustic. The Acoustic sample
  follows the selected Sustain/Note mode; event pitches/timing, five-voice stealing, saved arrangements and other
  songs are unchanged.
- Default host render: 559 steals, 166 dropped onsets, pre-tanh peak 1.211, 186 stereo frames at or above 1.0;
  output WAV has no clipped samples. The Acoustic trigger, alternate bass/piano modes, sequencer and other
  factory-song regressions passed. Normal FM-1 target build passed; not hardware-tested.
- Device image 500,268 B; RAM .data+.bss 62,324 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981002000`; build ID `F6BBAC7C`; SHA-256
  `B0450FCC790ED981DF3D96A15929F2765E831C958EF6DB54248061FB152E7DF7`.

## 031
- Local `0.20-test031` changes untouched factory Escape's Playback bass from NoMore to 808. The boosted 808 is
  scaled to 22% of its usual gain only in the Escape overlay to reduce mix saturation; live instruments and other
  songs are unchanged. Event pitches/timings and the five-voice limit remain unchanged.
- Default host render: 569 steals, 172 dropped onsets, pre-tanh peak 1.211, 229 stereo frames at or above 1.0;
  output WAV has no clipped samples. The 808 trigger, alternate bass/piano modes, sequencer and other factory-song
  regressions passed. Normal FM-1 target build passed; not hardware-tested.
- Device image 500,308 B; RAM .data+.bss 62,324 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981002000`; build ID `E20EBE18`; SHA-256
  `B77A7F46A1F35B2FA80CFC47AF6BD3C50E61AF57CE512770222A3D569C7BEC9C`.

## 030
- Local `0.20-test030` changes the bass in untouched factory Escape's Playback overlay from Electric to NoMore.
  The bass follows the selected Sustain/Note mode; event pitches and timing, five-voice stealing, saved songs and
  other factory arrangements are unchanged. NoMore uses the project owner's existing sample.
- Full default-mode host render: 554 steals, 162 dropped onsets, pre-tanh peak 1.211, 144 stereo frames at or
  above 1.0; output WAV has no clipped samples. The NoMore trigger, alternate bass/piano modes, five-voice
  sequencer and factory-song regressions passed. Normal FM-1 target build passed; not hardware-tested.
- Device image 500,268 B; RAM .data+.bss 62,324 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981002000`; build ID `3E2746E7`; SHA-256
  `9ED4AFBBAA2DB8B159D2E8355B3B3DB1900025B7B54425387B59FD746680C6C4`.

## 029
- Local `0.20-test029` raises the sequencer editing and saved-song validation budget from four to five simultaneous
  voices. A sixth overlapping voice is rejected, including when editing chords or reserving drum tails. Voice
  stealing remains absent for user songs and edited built-ins. Per-instrument pools can be smaller than five.
- Escape retains build 028's five-voice Playback; Halloween and The FMog keep their four-voice factory overlays.
  Five-voice songs cannot be loaded by older four-voice firmware; do not delete them if downgrading.
- Host tests verified five simultaneous drum voices and four pianos plus one drum, saving/rebooting/reloading,
  sixth-voice rejection, drum tails, edits, factory songs and sequencer regressions. Normal FM-1 target build
  passed. Hardware-tested: the user reports the sequencer test passed.
- Device image 500,268 B; RAM .data+.bss 62,324 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981002000`; build ID `0CEFE240`; SHA-256
  `82F9B9BCAEC35680A0CE06B95F3803219EE6530C76F2F45CFF5C6C6F4A7E59E9`.

## 028
- Local `0.20-test028` uses the same dense Escape Playback arrangement as 027, with an experimental five-voice
  budget only for untouched factory Escape. Other songs and the editor remain at four.
- Default-mode host render: 585 steals, 178 dropped onsets, peak five voices; pre-tanh peak 1.211 with 207
  stereo frames at or above 1.0; output WAV has no clipped samples. Alternate piano/bass mode and other-song,
  user sequencer and edited-built-in checks passed. Target build passed. Hardware-tested: user reports it works,
  sounds good, and sounds better than 027.
- Device image 500,264 B; RAM .data+.bss 62,324 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981002000`; build ID `FAE3E536`; SHA-256
  `CE96524F4BFC62B2A00FAFB0B60586186C864A970A92CFB78CD75D9FE2FA579D`.

## 027
- Local `0.20-test027` streams a 2,493-event MIDI-derived Escape arrangement from flash during untouched factory
  Playback, with guitar, electric bass, piano, Juno, Pro-1 and drums. Its budget is four voices with stealing.
  Saved arrangements and editor views are unchanged; edited built-ins bypass the overlay.
- Default-mode host render: 732 steals, 215 dropped onsets, peak four voices; pre-tanh peak 1.184 with 132
  stereo frames at or above 1.0; output WAV has no clipped samples. These peaks are not a hardware safety proof.
  Host song, edit/persistence, migration and sequencer checks and target build passed. Hardware-tested: user reports
  it works and sounds good.
- Escape Playback displays maximum sampled smoothed CPU and detected late renders since playback began; neither
  proves the absence of all timing faults. The two-key CHORD PIANO distortion remains unresolved.
- Device image 500,272 B; RAM .data+.bss 62,340 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981002000`; build ID `779F3811`; SHA-256
  `0011010ACFD11BDA1C61A9D4F0FC5B92769B70D181B3793EA2D11BE9DA0DFFC6`.

## 026
- Local `0.20-test026` build experiments with prioritized voice stealing during Playback of all three untouched
  factory songs. It swaps 44 events in Escape and Halloween and adds 38 in The FMog. Drums are prioritized above
  piano/Pro-1/bass, then Juno; older lower-priority voices may be cut and lower-priority new notes can be skipped.
- Playback overlays are in RAM only; saved arrangements, edited built-ins and user songs remain unchanged. The
  sequencer editor does not support voice stealing. Escape's full host render exactly matches the prior experiment.
- Focused full-song, four-voice, sampled/additive piano, sustain/one-shot bass, edited-song persistence, sequencer,
  migration and audio checks passed. Full host renders had no clipped output samples. Normal FM-1 target build
  passed. Hardware-tested: the user reports the test is good. Device CPU/load has not been measured.
  The previously reported two-key CHORD PIANO distortion is not fixed.
- Device image 493,740 B; RAM .data+.bss 62,340 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981002000`; build ID `29B78060`; SHA-256
  `B833A602A356C704F10E8F75101E2AACA222A7385FE390A5D5C1D9065CA8DEE5`.

## 025
- Local `0.20-test025` test build: The Fog is renamed The FMog. Escape and The FMog replace their CS-50 part
  with piano; Halloween replaces a Juno part with piano. Halloween and The FMog use electric bass.
- Untouched original built-ins upgrade automatically; edited built-ins are retained. The four-voice sequencer
  limit and event timings remain unchanged; fast voice switching was not used. Piano follows the selected engine.
- Focused built-in playback, migration/preservation/failure, SPLIT, phrase, sequencer and sampled audio host
  tests passed. Full host song renders had no clipped output samples. Normal FM-1 target build passed.
  The new arrangements and device CPU/load have not been auditioned or hardware-tested; host peaks do not prove
  hardware audio safety. CHORD PIANO two-key distortion reported on build 024 remains uninvestigated.
- Device image 481,556 B; RAM .data+.bss 62,340 / 98,304 B; pool 216,676 / 344,064 B; package 614,938 B;
  identity `FM-1_1981002000`; build ID `9D89EB19`; SHA-256
  `63D5918661E759BE599E4FB1623D8CA420A1406732B499321DD4BECE3A85272B`.

## 024
- Local `0.20-test024` test build: CHORD SPLIT upper keys play chords, and keys 12–19 also select chord type.
  Main-screen GLO help displays the build version on its first page.
- Focused chord, phrase recorder, sequencer and help host scenarios and the Pro-1 chord audio check passed.
  The normal FM-1 target build passed. Hardware-tested: the user confirmed SPLIT and the version display work.
  This is not a published GitHub release.
- The user reported distortion when pressing two CHORD PIANO keys simultaneously. The cause and specific chord
  types, pitches and velocities are not yet established; investigate before changing levels or voice allocation.
- Device image 481,104 B; package 614,938 B; identity `FM-1_1981002000`; build ID `D914E066`; SHA-256
  `8AED9F694E52040D088D99B138C19A04BC235119BA7861537CA61555601FF75F`.

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
