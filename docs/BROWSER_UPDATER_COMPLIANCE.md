# Plissken browser updater compliance

Audited 2026-11-27 against current local fm1-base/felucca-bsp source, current public Firmware Hub rules, and the
FM1 Open repository/release information.

## Scope distinction

Plissken is an **fm1-base/felucca-bsp firmware**. It follows the open `.fwsc`, USB-MIDI update handshake, loader, and
`firmware.json` path used by The Firmware Hub and fm1-base's browser installer.

M-VAVE's **FM1 Open Workbench** is a separate official Open SDK distribution path for FM1 Open applications. Plissken
is not an FM1 Open binary and should not be expected to load through Workbench merely because it is a valid `.fwsc`.
The relevant browser compliance target for Plissken is the Firmware Hub/fm1-base updater.

## Result

**Compliant with the verifiable fm1-base/Firmware Hub browser updater requirements.**

No blocking package or protocol defect was found. Remaining caveats are hardware/browser-path testing and metadata
freshness, not malformed firmware.

## Actual build 054 checks

Package: `builds\plissken054.fwsc`

| Check | Result |
| --- | --- |
| `.fwsc` size | PASS: 614,938 bytes |
| UFW header CRC | PASS |
| UFW entry-list CRC | PASS |
| Part CRC checks | PASS |
| Embedded identity | PASS: `FM-1_1981003400` |
| App part type 0 | PASS: offset 1,024, length 602,112 |
| Update loader type 100 | PASS: offset 603,136, length 11,782 |
| Package SHA-256 | `B403DDB991205D400731F7E75CAE5E162168EAAB3175C468947973BF9CD57BB1` |
| Logical browser image size | PASS: 614,918 bytes after marker removal |
| Identity format | PASS: `FM-1_` + digits; 15 characters |
| Flash support in generated manifest | PASS: `856014` and `B36014` |
| Long identity OTA handshake implementation | PASS in `base/bsp/tests/ota_test.c` |
| Update-loader OTA handshake implementation | PASS in `base/bsp/tests/ota_test.c` |

## Package/loader compliance

- The package carries both app and update-loader entries.
- The loader follows the documented "device asks / browser answers" logical-image update protocol.
- Plissken answers the plain M-UPGRADE identity query with the required shortened form and the full `01` query with
  the complete identity.
- The loader enters as `ota-FM-1_900`, as expected by fm1-base browser and M-UPGRADE discovery.
- The loader writes only permitted app/update-record ranges and not the boot head or Plissken's user-data region.
- The browser preflight checks the same header, list, and checked-part CRCs.

## Manifest and Hub compliance

Generated `build/firmware.json`:

- Required release keys are present: author, MPL-2.0 licence, tagline, description, tags, install URL, source URL,
  screenshot.
- Tags `synth` and `sequencer` are valid.
- Both current flash chips are listed.
- The screenshot exists and is 240 x 240, valid and below size limits.
- `fmhub.json` contains `what`, `kinds`, `image`, and `sounds`; `docs/card.png` is 240 x 240 and 173,118 bytes.
- `base/tests/firmware_json_test.py` passed all available manifest tests.

## Release workflow

- Version tags trigger the shared `zvenson/fm1-build` v1 workflow.
- The required release/attestation permissions are present.
- Renaming the built file to `onlyusefmflashertoinstallplissken.fwsc` happens **inside** the attested build script.
- The `packages:` glob matches the renamed asset.
- A fixed release asset name is allowed; browser/Hub rules require a valid `.fwsc`, not a versioned filename.
- Do not rename or upload a replacement package after the workflow attestation, because SHA-256 pinning and
  attestation verification would no longer match.

## Warnings

1. **M-UPGRADE's shortened identity collision.**
   Plissken's full identity `FM-1_1981003400` is unique, but the plain/legacy reply exposes only `FM-1_198`.
   Another firmware prefix beginning `FM-1_198` can therefore look identical to old M-UPGRADE software. The full
   query used by current browser tools resolves this. This is not a package-format error.

2. **The renamed asset is human safety guidance, not a technical installer lock.**
   Nothing in the `.fwsc` prevents the Firmware Hub, fm1-base browser installer, or another conforming tool from
   installing it. If browser installation is not to be advertised, that remains a policy/documentation choice.

3. **End-to-end browser hardware validation is not complete.**
   The protocol/package tests pass, but a real WebMIDI browser test should still be performed for the exact
   release file. Previous reported browser-installer failures mean this evidence should not be treated as a
   physical installation guarantee.

4. **Metadata should be refreshed for Groovebox.**
   `firmware.json` is valid but its description/tags still present only synth/sequencer functionality. A future
   release should consider `groovebox` and optionally `experimental` tags, plus a description of the new mode.

5. **Sample licensing remains a Hub review item.**
   `fmhub.json` declares the licensed Rossf LM-1 pack and own bass recordings. The package format is compliant,
   but asset redistribution terms remain separate from updater compliance and receive manual Hub review.

## Recommended release hardware checks

For each release candidate, using the exact attested `.fwsc`:

1. Confirm the browser discovers the Plissken MIDI port.
2. Install from official V15.
3. Install Plissken over another custom firmware.
4. With Plissken running, install another conforming firmware and confirm handoff to its loader.
5. Interrupt an update once, reconnect, and confirm resume/restart behavior.
6. Confirm identity and full flash JEDEC ID are shown correctly.
7. Test Chrome and Edge, direct USB connection without hubs.
8. Confirm saved calibration/data behavior; back up before every destructive test.
9. Test at least one `856014` and one `B36014` unit when available.
10. Verify the published asset SHA-256 and attestation.

## Conclusion

Plissken's package, identity, loader, flash metadata, manifest, release workflow, and Hub metadata satisfy the
verifiable browser-updater requirements. The only unresolved matter is real-device browser end-to-end behavior,
plus the legacy three-digit identity ambiguity.
