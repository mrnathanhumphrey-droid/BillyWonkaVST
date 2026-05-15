# Known Bugs — Working List

Reproducible bugs found in BW BASS v3.0.6 on macOS (Apple Silicon, macOS 14+).
Listed in suspected order of effort to fix.

## 1. Pluck driver outputs only clicks

**Severity:** high — breaks one of the three voicing modes.

**Symptom:** With `Bass Mode = Pluck` (and Drive Mode = Auto, which routes to
the Transient Waveshaper / envelope-gated CP3 driver), pressing notes
produces only short transient clicks, not a musical pluck attack with
a sustained body.

**Additional Ableton-specific symptom:** the Ableton track level meter
DOES register signal on each note (so audio is reaching the host),
but that signal does not produce anything audible in the headphones.
This strongly suggests the click is either sub-millisecond (below
headphone reproduction floor), DC-offset only (meters average energy
but no AC content), or out-of-band frequency content. Could also mean
the transient is so short it shows on meters but never lasts long
enough to drive a speaker excursion.

**Repro:** load any Pluck-family preset (e.g. `Pluck_Funk`, `Pluck_LoFi`,
`Pluck_Warm_RnB`). Hold any note. Output = click, no sustain. In
Ableton, the BW BASS track level meter pulses on each note but
headphones stay silent.

**Suspected location:**
- `DSP/TransientWaveshaper.cpp` — the envelope-gated CP3 polynomial
- `DSP/OutputStage.cpp` — driver routing in `setDriveMode(Auto)` for
  `BassMode::Pluck`
- `DSP/SynthEngine.cpp` `applyBassMode(Pluck)` — the parameter preset
  for pluck mode (filter cutoff, env amounts, etc.)

**Hypotheses (in order of likelihood):**
1. The amp envelope decay+release pair set by `applyBassMode(Pluck)` is
   too short, so the body of the note dies before the wave is audible.
   Check `env_a_decay` / `env_a_release` values applied for pluck mode.
2. The Transient Waveshaper's envelope-follow gate has the wrong
   release time — it gates audio off after the transient and never
   re-opens for the sustained portion.
3. Filter cutoff for pluck mode is too low and the body is filtered
   out, leaving only the transient click of the filter envelope opening.
4. The driver's wet/dry mix is fully wet on the transient-only output.

**Diagnostic next steps:**
1. Bypass the driver (Drive Mode = `Auto` → switch to `Tape` or even no
   drive) and check if the pluck body returns. If yes, the driver is
   the cause. If no, the envelope/filter settings are the cause.
2. Print or scope the `ampEnvelope.tick()` values over a held note in
   pluck mode — does the level drop to zero too fast?
3. Compare to v3.0.3 release. Was the pluck driver working there? If
   yes, regression introduced between v3.0.3 → v3.0.6. If no,
   pre-existing.
4. **Audibility test in Ableton:** route the track through Ableton's
   Spectrum analyser device (or any analyser). If the click shows
   energy only at DC (0 Hz) or above 20 kHz, that explains the
   "meters move but no sound" symptom. If energy is in the audible
   band but extremely short, headphone reproduction limit is the
   cause — try lowpassing post-driver and increasing release.
5. Compare `OscillatorBank::tick()` direct output (pre-driver, scope
   in code or via debug print) to the driver output — is the source
   signal already a click, or does the driver collapse a sustained
   waveform into a click?

---

## 2. Preset XMLs scanned by editor but never appear in the preset browser

**Severity:** medium — presets work as state files, but UX is broken.

**Symptom:** With 20+ `*.xml` preset files in
`~/Library/Application Support/BW BASS/Presets/Factory/`, all with the
correct `<GrooveEngineRnBState>` root tag, the BW BASS plugin shows
`-- Init --` in the preset name slot and clicking `>` (next preset
arrow) in the header does NOT change the displayed name.

Same behavior in BW BASS standalone AND in Ableton VST3 host.

Same behavior whether presets are placed at:
- `~/Library/Application Support/BW BASS/Presets/Factory/` (path #1, install.sh target)
- `<dir with .app>/Presets/Factory/` (path #3, exe-relative)

**What's been ruled out:**
- File location — confirmed both expected paths populated.
- File format — confirmed root tag `<GrooveEngineRnBState>` matches
  what `PluginEditor.cpp::loadPresetByIndex` expects.
- Plugin version — confirmed running v3.0.6 (CFBundleShortVersionString).
- Quarantine — `xattr -cr` was run on the .app.

**Suspected location:**
- `Source/PluginEditor.cpp` `scanPresets()` — paths or `findChildFiles`
  call.
- `Source/PluginEditor.cpp` editor constructor — is `scanPresets()`
  actually invoked, and is the result wired into the headerBar?
- `UI/HeaderBar.cpp` `prevBtn` / `nextBtn` `onClick` callbacks — wired
  to `onPresetNav` but does the editor's handler call
  `loadPresetByIndex`?

**Hypotheses:**
1. `scanPresets()` is being called but the `presetFiles` / `presetNames`
   members are members of the editor and may be reset somewhere after
   the scan (a constructor reorder, a `repaint()` triggering a reset).
2. Apple Silicon sandboxing / TCC quietly blocking the .app from
   reading `~/Library/Application Support/`. Would also explain why
   the exe-relative path didn't help if it has a similar permission
   issue, though that's less likely.
3. `juce::File::getSpecialLocation(userApplicationDataDirectory)` in
   JUCE 8 returns a different path on macOS Sonoma+ than expected —
   perhaps with a sandbox-mapped path under
   `~/Library/Containers/<bundle-id>/...`.

**Diagnostic next steps:**
1. Build a debug version locally. In `scanPresets()`, after every
   `searchPaths.add(...)`, log the resolved path and whether
   `.isDirectory()` returns true. Run the standalone; check which path
   it actually finds (if any).
2. Try `dtruss` or `fs_usage` to watch the running standalone:
   ```bash
   sudo fs_usage -f filesystem | grep -i "Presets\|BW BASS"
   ```
   then launch standalone. See which path it opens (if any).
3. Test in a freshly-built local debug binary (skip the
   download/extract path entirely) to rule out a release-artifact issue.
4. Print `juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getFullPathName()`
   to a log file at startup — confirm it's actually
   `~/Library/Application Support`.

---

## 3. Double-click on .app blocked by launchd despite xattr -cr

**Severity:** low — workaround exists (Terminal-launch works fine).

**Symptom:** After `xattr -cr "BW BASS.app"` (clears quarantine),
double-clicking the .app in Finder shows no window, no error visible
to the user. Console.app logs the familiar:

```
Error Domain=RBSRequestErrorDomain Code=5 "Launch failed."
NSPOSIXErrorDomain Code=111 "Launchd job spawn failed"
```

Running the inner binary from Terminal works:
```bash
"/path/to/BW BASS.app/Contents/MacOS/BW BASS"
```

Loading the VST3 inside Ableton also works.

So launchd is the only path that rejects it.

**Suspected cause:** the bundle's `_CodeSignature/CodeResources` hash
manifest doesn't fully match the actual bundle contents. Terminal
exec'ing the inner binary bypasses Gatekeeper's signature integrity
check; launchd enforces it; Ableton's plugin loader is permissive
enough to load anyway.

**Evidence:** earlier v3.0.5 build had
`Sealed Resources files=1` on `codesign -dv` — the manifest only
covered one file when it should cover hundreds. The v3.0.6 zip-on-
runner restructure was supposed to fix this. Worth re-running
`codesign -dv --verbose=4` on the v3.0.6 .app to confirm whether
sealed-resources count is in the expected range now.

**Hypotheses:**
1. `ditto -c -k --sequesterRsrc --keepParent` from the build runner
   into the user-facing .zip does not preserve the
   `_CodeSignature/CodeResources` xattr / extended metadata correctly
   on Apple Silicon strict-signing macOS.
2. The CI does the ad-hoc sign BEFORE the package step's `ditto`
   into `dist/`. The signature in the dist'd bundle may be invalid
   because the file paths changed. Need to re-sign AFTER the package
   ditto, before the zip-for-release ditto.

**Suggested fix:** add a final re-sign step in CI immediately before
the zip-for-release step:

```yaml
- name: Re-sign after ditto-copy (macOS)
  if: runner.os == 'macOS'
  shell: bash
  run: |
    DIST="dist/${{ matrix.artifact_name }}"
    find "$DIST" -maxdepth 4 -name "*.vst3"      -type d -exec codesign --force --deep --sign - {} \;
    find "$DIST" -maxdepth 4 -name "*.component" -type d -exec codesign --force --deep --sign - {} \;
    find "$DIST" -maxdepth 4 -name "*.app"       -type d -exec codesign --force --deep --sign - {} \;
```

then the existing zip-for-release ditto runs.

---

## 4. (Backlog) macOS standalone preset path doesn't fall through to bundled location

**Severity:** trivial — UX improvement.

When the user downloads the macOS release zip and just runs the .app
directly without running `install.sh`, the bundled `Presets/Factory/`
folder is right next to the .app but the standalone's exe-relative
scan path (#3) lands on `<dir with .app>/Presets/Factory` which is
the same place. So this SHOULD already work. If it doesn't (and
testing confirms it doesn't in v3.0.6), then bug #2 is the root and
fixing #2 covers this.

---

---

## 5. ADSR sustain not held while key is held

**Severity:** high — root cause of bugs #1 and #6 (click-only output).

**Symptom:** User holds a key. Envelope rises, decays, then immediately
silences instead of holding at the sustain level. Output = click then nothing.
Affects both Pluck and 808 modes.

**Suspected location:**
- `Source/PluginProcessor.cpp` `updateEngineParameters()` — APVTS values
  (loaded from saved DAW state) override the bass-mode preset values
  every block. If a previous session had `env_a_sustain = 0` saved, that
  value sticks across every mode switch and across plugin reloads.
- `DSP/SynthEngine.cpp` `applyBassMode()` — writes sustain values to the
  engine but never to APVTS, so the writes don't persist.

**Fix in progress:** make bass-mode change write the mode's envelope +
osc-waveform values into APVTS using `setValueNotifyingHost`, so the
mode's character actually persists and survives the read-back. Header
change landed in `Source/PluginProcessor.h`; `.cpp` half not yet written.

---

## 6. Drive happens after the filter; should be before

**Severity:** high — affects perceived loudness, drive audibility, and
likely contributes to the "drivers are just clicks" symptom (because the
filter strips the harmonics before the driver can color them).

**Current chain:** `mixer → Moog filter → ampEnv × → drive → bass driver → compressor → EQ → reverb → master`

**Wanted:** `mixer → drive → bass driver → Moog filter → ampEnv × → compressor → EQ → reverb → master`

i.e. drive + driver move *before* the filter; everything else (comp, EQ,
reverb, master) stays after. Matches the classic Moog signal flow —
oscillators get saturated, then the filter shapes the saturated tone.

**Fix path:** split `OutputStage::process` into `processPreFilter`
(drive + bass driver) and `processPostFilter` (comp + EQ + reverb +
master vol). `SynthEngine::process` invokes them on opposite sides of
`filter.process(...)`.

---

## 7. Reverb wets the sub band; should be ≥ 180 Hz only

**Severity:** medium — muddies the low end.

**Symptom:** BassReverb processes the full-band signal, including the
fundamental and sub-harmonics, which makes the low end smear.

**Fix:** add a high-pass at ~180 Hz to the reverb send only. Keep the dry
signal full-band. Implement either:
- Inside `BassReverb::processSample` — HPF the input before the reverb
  network, leave the dry path untouched.
- Or as a tilt in `OutputStage::process` — split signal at 180 Hz,
  route only the high band through reverb, sum after.

Simpler is the first option (HPF on the reverb input only). 2nd-order
biquad HPF at 180 Hz, Q ≈ 0.7, applied before the reverb tank.

---

## 8. LFO sync button doesn't change rate from Hz to beat increments

**Severity:** medium — sync mode currently has no visible effect on the
rate; user expects beat-relative LFO rate when sync is on.

**Symptom:** Toggling the LFO sync button does not switch the LFO rate
parameter from Hz units to host-tempo-relative beat divisions
(1/1, 1/2, 1/4, 1/8, 1/8T, 1/16, etc.).

**Suspected location:**
- `DSP/LFOEngine.cpp` `setTempoSync` / `setRate` — likely sets a flag
  but doesn't re-map the rate parameter range.
- UI side — the rate knob's value display doesn't switch to beat-relative
  notation.

**Fix path:** when sync is on, interpret the rate parameter as an index
into a beat-divisions table:
`{ 4/1, 2/1, 1/1, 1/2, 1/2T, 1/4, 1/4T, 1/8, 1/8T, 1/16, 1/16T, 1/32 }`
and compute Hz from tempo: `lfoHz = bpm / 60 * (beatsPerBar / division)`.

---

## Notes

- Pluck-driver bug (#1) is independent of the macOS pipeline bugs and
  exists on Windows too — worth confirming on the Win build to narrow
  whether it's a recent regression or always-broken.
- Preset-scan bug (#2) is the highest-priority UX blocker.
- Double-click bug (#3) has a clean workaround for now (Terminal /
  install.sh / open via Ableton).
