# MANIFOLD

**Generative just-intonation firmware for the M-VAVE FM-1.** Nothing is sequenced.

![The Perform page](docs/img/manual/perform.png)

MANIFOLD turns the FM-1 into a self-playing instrument. Two layers of moving points wander over a map of a
just-intonation scale (a Tonnetz: neighbours are fifths, thirds and sevenths), and every note they pass near sounds,
as loud as the points near it. The rhythm is one continuous function of the bar, the drums ride the synth's own notes,
and every partial is a whole-number multiple of its note. Four performance knobs steer it; the keys play into it.

- **Play it in the browser:** [stets43.github.io/Manifold/emu](https://stets43.github.io/Manifold/emu/), the whole
  firmware in WebAssembly on a drawn FM-1 (the build is in [`emu/`](emu/)).
- **Install:** the web installer on the [MANIFOLD site](https://stets43.github.io/Manifold/) (Chrome or Edge), or
  [`firmware/manifold-1.0.5.fwsc`](firmware/) with fm1-base's `tools/fm1_install.py`.
- **User guide:** [docs/MANUAL.md](docs/MANUAL.md), also [on the site](https://stets43.github.io/Manifold/manual/).
  It covers every page and setting, and ends with the mathematics of each generator for developers.

## What is in it

- **41 scales** in just intonation: the harmonic series, handpan and tongue-drum tunings (the 1-3-5-9 hexany, Golden
  Gate, Celtic minor, Kurd…), pentatonics, modes. Temper moves them continuously to 12-tone equal temperament.
- **Twelve motions** for the points: Orbit (Lissajous), Ladder, Swarm (Kuramoto), Lorenz, Motif, Drift, Billiard
  (Sinai), Gravity, Stride, Loops (coprime tape loops), Erode, Chords. **Eight envelopes**: Pulse, Swell, Speed, Pass,
  Height, Hold, Stream, Drums. Either layer can move, add to or gate the other instead of playing.
- **Voices** of up to 16 exact harmonics a note, shaped by one continuous spectral function (tone, tilt, centre, band,
  stride, phase law, bloom, detune, width).
- **A rhythm without a grid**: a stream of smooth peaks with continuous swing, three-against-four, fineness, rotation
  and long form; a threshold or a coupled-oscillator engine; four drums that put envelopes and colour on notes the synth
  already plays, never changing pitch.
- **A 24-route modulation matrix** with six LFOs, the knobs, the generators' own motion and more as sources, and the
  performance knobs as destinations.
- **Effects**: fuzz, morphing filter, tempo delay, an 8-line reverb that can be tuned to the scale or frozen, bus
  compressor, look-ahead limiter.
- Rolls in eight styles, locks, undo, 16 patches with four scenes each, MIDI over USB and Bluetooth, Bluetooth audio
  out to headphones and speakers.

## Releases

- **1.0.5** (2026-11-17): drum **patterns** beside linear drumming: a library trained on about 100 house, techno,
  breakbeat, drum'n'bass and hip-hop patterns, played without a grid (swing bends them, Density thins them, fills,
  ghosts, rotation, polymetric loop lengths, mutes), rolled by a decision tree over the corpus. New styles **House**
  (replacing Drum circle) and **Hip-hop** (replacing Tongue drum); **Breakbeat** rebuilt (mostly 124–140 BPM, a third
  drum'n'bass). Wider tempos in every style; Ambient, Cosmic and Wild down to sparse, slow, evolving; a deeper,
  richer matrix in every roll (no self-feedback routes); Machine modulates its own rhythm. **Keys pull the points**
  (Layer 1 → Keys): off, a key plays just its note. The keys play both layers by default. **Harder drums**: a hit's
  Bright and Snap act at full strength however loud the note is (no longer by the hit's share of it), Snap past Wave's
  top with a length Settle sets, and rolls give the kick's duck more depth. Older patches load with the
  patterns off; 1.0.4 cannot read patches saved by 1.0.5.
- **1.0.4** (2026-11-13): 64 patch slots in four banks (A–D) of 16. A tap on SAVE shows the Patches page (the
  banks, the active bank's slots); SAVE + key and HOME + key use the active bank; the settings are kept with
  SAVE + OCT+ or K4 on that page, no longer with a SAVE tap. 1.0's 16 slots become bank A. After 1.0.4, do not go
  back to an earlier version (it would miss some patches and the Bluetooth speakers).
- **1.0.3** (2026-11-12): FM-1s with the newer UC25HQ80 flash (JEDEC `B36014`, units sold since autumn 2026) are
  recognised alongside the Puya P25Q80 (`856014`). Before, those units kept nothing between power-ons (patches,
  settings, Bluetooth devices). Nothing else changed.
- **1.0.2** (2026-11-12): Bluetooth audio to speakers that set up the stream themselves after pairing (1.0 refused
  them); the set-up's steps on the crash report's log; the SBC encoder set up from each connection's agreement.
  The sound engine is unchanged.
- **1.0** (2026-11-10): first release.

## Building

MANIFOLD is an app on [fm1-base](https://github.com/charlesvestal/fm1-base). It needs a few changes to fm1-base
(a larger RAM code window, and the battery read in the key scan's pot slot); they are in [`patches/`](patches/).

```
git clone --recursive https://github.com/charlesvestal/fm1-base
cd fm1-base
git checkout 2d8ed74 && (cd bsp && git checkout 773b0b9)
git apply ../Manifold/patches/fm1-base.patch
(cd bsp && git apply ../../Manifold/patches/felucca-bsp.patch)

python3 tools/build.py ../Manifold --host              # the host simulator (any Linux or macOS)
../Manifold/build/host/manifold_host ../Manifold/tests/basic.txt ../Manifold/build/host

tools/get_toolchain.sh                                     # JieLi's clang for the FM-1
export JIELI_TOOLCHAIN=~/.jieli/toolchain AC79_SDK=/path/to/fw-AC79_AIoT_SDK
python3 tools/build.py ../Manifold --release 1.0.5     # -> build/manifold-1.0.5.fwsc
```

The device build needs three files from JieLi's AC79 SDK V1.2.1 (`cpu/wl82/tools/uboot.boot`, `cfg_tool.bin`,
`cfg/eq_cfg_hw.bin`; Apache-2.0), checked by SHA-256. `tests/basic.txt` runs 144 checks in the simulator;
`tests/screens.txt` and `tests/guide.txt` make the screenshots. `tools/site.py` builds the website (the front page in
`site/`, the manual, fm1-base's web installer and the firmware).

## Layout

```
app.c, app.h, fm1.toml    the app: pages, performance keys, scenes, patches, MIDI, Bluetooth audio
dsp/gen.c                 the twelve motions, the eight envelopes, routes, keys, the drive
dsp/rhythm.c              the stream, events, drum choice, the Lock engine
dsp/drums.c               drum triggers, envelopes and character, on the synth's voices
dsp/voice.c               the oscillator banks and their spectral function
dsp/engine.c, scales.c    the surfaces, the Tonnetz layout, tuning, the scale library
dsp/mod.c                 LFOs, the matrix, the knobs' paths
dsp/fx.c                  effects; dsp/params.c parameters, rolls, styles; dsp/synth.c the top level
docs/MANUAL.md            the user guide;  site/  the website's front page
firmware/                 the release and the licence texts that ship with it
emu/                      the firmware compiled for the browser (tools/emcc-zig: no Emscripten needed)
```

## Licence

MANIFOLD is [MPL-2.0](LICENSE). It is built on fm1-base by Charles Vestal (MPL-2.0), which carries code by Leo
Kuroshita (Felucca), Chris Farrell and others (see fm1-base's LICENSING.md). The firmware uses fm1-base's own Bluetooth
stack, which includes Apache-2.0 and BSD-2-Clause code: their texts are in [`firmware/`](firmware/). The screen font is
Barlow Semi Condensed (SIL OFL 1.1). M-VAVE and FM-1 are trademarks of their owners; this project is not affiliated
with M-VAVE.
