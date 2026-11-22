# Credits

From author: Record each code, sample, and asset source here when it is first added to Plissken, including its creator, source,
license or permission, and any changes. Update package metadata in the same change; do not defer attribution until
release.

## Code and firmware platform

- Plissken's application code is maintained in this project.
- Built on **fm1-base** by Charles Vestal and **felucca-bsp**, from **Felucca** by Leo Kuroshita
  ([@kurogedelic](https://github.com/kurogedelic), Hügelton Instruments). See [fm1-base credits](./base/CREDITS.md)
  and [licensing notes](./base/LICENSING.md) for additional contributors, provenance, and license details.
- **SLOOP** is a workflow and sample-configuration reference only; no SLOOP code has been imported into Plissken.
- **Felucca** (hugelton/Felucca, GPL-3.0-only) is the design reference for the hold-a-button quick layers (GLO mutes and
  tap tempo, FX momentary effects, SEQ tools, LFO momentary changes). Only the behaviour was copied, in a smaller form;
  the code in `app.c` was written for Plissken and no Felucca source was imported.
- **ChoralRoot FM-1** ([Quixotic7/ChoralRootFM1](https://github.com/Quixotic7/ChoralRootFM1)) is an Orchid-style
  chord-workflow reference only. License review (2026-11-09): it is Felucca-derived and `GPL-3.0-only` (Copyright
  (C) 2026 Leo Kuroshita; some ported parts keep BSD-3/MIT/Apache-2.0/GPL-3.0-or-later terms). Its code is not
  imported: doing so would require distributing all of Plissken under GPL-3.0-only, which conflicts with
  Plissken's MPL-2.0 app files and the Sampling+ drum samples. Plissken's chord mode is an independent
  implementation of general chord ideas (quality, voicing, inversion, spread), which are not copyrightable. If
  code is ever adopted from a GPL project, record it here first and relicense the distribution accordingly.
- The separate, reduced CS-80-inspired renderer and its ten patch recipes are original Plissken implementations.
  Their broad dual-channel/expressive-synth inspiration was informed by the public **CS-1** project by
  twochannels ([GPL-3.0-only](https://github.com/twochannels/CS-1)); no CS-1 code or preset data was copied.
  The patch names are user-requested timbral targets, not claims of exact preset recreation. This is not a full
  CS-80 emulation.
- **JAZZY** ([JazzyRhythmer/Jazzy](https://github.com/JazzyRhythmer/Jazzy)) was reviewed on 2026-11-11 as a design
  reference only. Its firmware is `GPL-3.0-only` (Felucca fork with ported GPL, MIT and MPL-2.0 parts), and its LXR
  preset bank and Amen/Think break recordings state that redistribution rights are not cleared. Nothing was copied.
  Its Juno-106-style four-pole VCF design (`fm106_vcf.h`) informed Plissken's change to a four-pole Juno filter,
  which is an original implementation of a standard cascaded one-pole ladder.
- **J6** ([JackieAlacrity2373/fm1-j6](https://github.com/JackieAlacrity2373/fm1-j6), `MPL-2.0`, JUNO-60-style synth
  on fm1-base) was reviewed on 2026-11-11. Its zero-delay four-pole filter design (trapezoidal stages with the
  resonance solved in-sample and a rational tanh) informed Plissken's `ladder_*` filter used by the Juno and Pro-1
  voices. That code is an original implementation; no J6 source was copied. The owner may later adopt J6 code
  directly. If so, the copied code stays under `MPL-2.0` with its notices kept, and credit is recorded in the same
  change. Its `LICENSES/Apache-2.0.txt` has not been checked for which files it covers.

## Samples

- **Sampled piano prototype:** six Kawai Grand Piano sustain recordings by Simon Dalzell (Ivy Audio), from the
  [Versilian Community Sample Library](https://github.com/sgossner/VCSL/tree/c1ea7bcc3c7309650ab0da9d15c9cd1fbc4a4c7e/Chordophones/Zithers/Grand%20Piano%2C%20Kawai/Sustains),
  licensed under [CC0 1.0](https://github.com/sgossner/VCSL/blob/c1ea7bcc3c7309650ab0da9d15c9cd1fbc4a4c7e/LICENSE).
  The C2, F#2, C3, F#3, C4 and F#4 v4/round-robin-1 recordings are downmixed to mono, trimmed to 1.0 second
  from just before the attack, resampled from 44.1 kHz to 22.05 kHz, peak-normalized, and given a 100 ms fade.
  The processed 16-bit WAVs are in [`assets/piano-cc0/`](assets/piano-cc0/); firmware builds encode them as 4-bit
  IMA ADPCM. Credit is retained although CC0 does not require it.
- **Original bass recordings:** [`assets/NoMoreBass.wav`](assets/NoMoreBass.wav) and
  [`assets/Bass808.wav`](assets/Bass808.wav) are the project owner's recordings; the owner holds the rights and
  confirmed they may be used freely. They are packed as 8-bit mono. The four BassPlunk recordings were removed
  from the repository and firmware at the owner's request.
- **Sampled bass timbres:** Electric uses “Bass-guitar Pluck 01” by
  [TheEndOfACycle](https://freesound.org/people/TheEndOfACycle/sounds/660532/), and Acoustic uses the C2 pizzicato
  note from “Double bass” by [elzozo](https://freesound.org/people/elzozo/sounds/390140/). Both are marked
  [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/). The original WAV downloads require FreeSound
  authentication, so the processed samples use the public MP3 previews. The Electric A#3 excerpt is trimmed to
  0.35 seconds; the Acoustic C2 excerpt is trimmed from 0.46 to 0.86 seconds. Both are downmixed to mono, resampled
  to 22.05 kHz, peak-normalized, and given a short tail fade. The processed WAVs in [`assets/bass-cc0/`](assets/bass-cc0/)
  are packed as 4-bit IMA ADPCM.
- **Sampled guitar timbres:** the Electric recording, “Pacifica low E string -G.wav,” is by
  [tribaldarkness](https://freesound.org/people/tribaldarkness/sounds/136598/) from the [Pacifica Electric guitar
  pack](https://freesound.org/people/tribaldarkness/packs/8526/); the Nylon recording, “Clean_G_str_pick.wav,” is
  by [SpeedY](https://freesound.org/people/SpeedY/sounds/8402/) from the [Nylon Guitar Single notes pack
  469](https://freesound.org/people/SpeedY/packs/469/); and the Acoustic low-E recording, “E String 02.wav,” is
  by [johnnypanic](https://freesound.org/people/johnnypanic/sounds/248177/) from the [E strings
  pack](https://freesound.org/people/johnnypanic/packs/15283/). All three recordings are marked
  [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/). The processed mono 22.05 kHz samples use 1.2-second
  excerpts from the public previews, normalized with a short tail fade, and are packed as 4-bit IMA ADPCM at build time.
  The earlier project-owner-provided guitar riffs have been removed; the modeled guitar remains available.
- **Splice (permission noted, none used yet):** Splice has told the project owner that Splice sounds may be used in
  the firmware with artist credits. When any Splice sound is added, list its name, artist and pack here at that time.
- All 13 samples packed into Plissken builds are from **Rossf's**
  [Linn LM1 (LM-1) Drum Set pack 4919](https://freesound.org/people/rossf/packs/4919/), licensed under
  [Creative Commons Sampling+ 1.0](https://creativecommons.org/licenses/sampling+/1.0/). They are converted from
  mono 16-bit PCM to 8-bit mono at build time and are available through the LM-1 keyboard mapping; originals
  are preserved in [`references/rossf-lm1-drum-set/`](references/rossf-lm1-drum-set/). See the
  [source pack's attribution and license notes](references/rossf-lm1-drum-set/_readme_and_license.txt).
## Music

- **Escape from New York test MIDI:** user-provided local file `escape_from_ny.mid`; the project owner confirmed
  authorization to embed it in Plissken firmware. The generated `dsp/escape_sequence.h` and
  `dsp/escape_rebuild_sequence.h` contain playback data derived from it; the local `dsp/song.h` is not distributed.
  Original composer/arranger attribution was not included with the local file.
- **Halloween sequence:** derived from project-owner-supplied local `Synth.mid`, `Piano.mid`, `Bass.mid`, and
  `Drums.mid` reference stems. The MIDI files remain outside the repository and are not distributed with the
  firmware; `dsp/halloween_sequence.h` contains the reduced, editable 5/4 sequencer transcription. The synth stem
  uses Plissken's Juno-6-style renderer. The project owner confirmed authorization to distribute the transcription.
- **The FMog sequence:** derived from the project-owner-supplied local cover MIDI `the fog main theme (cover).mid`.
  The project owner confirmed authorization to reproduce and distribute the transcription in firmware. The MIDI
  file remains outside the repository; `dsp/fog_sequence.h` contains the reduced, editable sequencer arrangement.

## Graphics

- **Color startup splash:** AI-generated adaptation by Copilot based on the project owner's supplied skyline sketch (cough cough).
  The resulting image is reduced to 240×144 and packed using a four-color RGB565 palette for the FM-1 display.
- **User-provided portrait:** `assets/jc-face.png`, supplied for the instrument screen. The build scales it to
  48 pixels high, converts it to a four-color grayscale palette with a transparent background, and RLE-packs it
  into the generated firmware header.
- **User-provided knob artwork:** `assets/snake-sma.png` and `assets/snake-sma2.png` through
  `assets/snake-sma4.png`, supplied for the instrument screen. The build converts the ordered 50×50 grayscale
  images to transparent, tintable shades and RLE-packs them into firmware.

## New contributions

Before incorporating future code, samples, or artwork, verify the right to use them and record the attribution and
license here in the same change.

Contact: raggadus@gmail.com