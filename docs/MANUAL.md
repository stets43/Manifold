# MANIFOLD — User Guide

**Generative just-intonation firmware for the M-VAVE FM-1**

![The Perform page](img/manual/perform.png)

MANIFOLD turns the FM-1 into a self-playing instrument. Two **layers** of moving points wander over a map of a
scale, and every note they pass near sounds, as loud as the points near it. Nothing is sequenced: there are no steps,
no patterns and no note triggers. The motion is continuous, the loudness is continuous, and the rhythm is a
continuous landscape the motion is shaded by. You steer it with four performance knobs, play into it with the keys,
roll new patches with one button, and dig into every part of it from the page buttons.

Everything is in tune in a strict sense. Every note is a whole-number ratio of the scale's root (just intonation),
and every partial of every note is a whole-number multiple of that note, so chords fuse instead of beating.

This guide covers installing the firmware, playing it, and every page and setting. The last part,
[For developers](#for-developers), gives the mathematics of each generator and how the code is organised.

---

## Contents

1. Quick start
2. Install
3. How MANIFOLD thinks
4. The screen
5. Controls at a glance
6. The Perform page
7. Playing the keys
8. Rolls, styles, locks and undo
9. Patches and scenes
10. Scale (SEL)
11. Voice 1 and Voice 2 (ENV and GLO)
12. Layer 1 and Layer 2 (EDIT and ARP)
13. Drums and rhythm (SEQ)
14. Modulation (LFO)
15. Effects (FX)
16. Setup (HOME twice)
17. MIDI
18. Bluetooth audio
19. USB audio
20. Recipes
21. Troubleshooting
22. For developers

---

## Quick start

### Make music in a minute

1. **Press PLAY.** The patch plays itself: two layers of points move over the scale and every note they pass sounds.
   PLAY again stops it.
2. **Turn PRESET** to pick a style (Wild, Hip-hop, Groove, Ambient, Cosmic, House, Machine, Breakbeat, Funk, Latin,
   Indie). Once the knob
   rests it rolls a new patch in that style.
3. **Tap REC** until you hear something you like. Each tap is a new sound over the same scale.
4. **Turn K1–K4** to steer it while it plays.
5. **Play the keys** over it. A key sounds its note and pulls the moving points toward it, so the music bends around
   what you play.
6. **Hold SAVE and press a key (1–16)** to keep the patch. **Hold HOME and press the key** to load it again.

### How the rolls work

A roll makes a whole patch from a random **seed**, shaped by the **style**. The same seed and style always give the
same patch, and a roll glides into the new patch over about 1.5 s instead of jumping.

| Do | Rolls |
| --- | --- |
| REC (tap) | the sound: both voices, both layers, the rhythm and the drums. The scale, the effects, the matrix and the knobs stay. Two rolls in five are **Dry** (the effects set to nothing, the Space knob becomes Shape) |
| REC (double tap) | everything except the scale |
| REC (hold) | a mutation: every setting nudged a little from where it is |
| REC + a page button | only that page. **REC + SEL** is the only roll that changes the scale |
| PRESET / ALGO | a new patch in the chosen style / the next or previous seed |

**SAVE + a page button** locks that page so rolls leave it alone: keep a scale or a voice you like and roll the rest.
**Hold PLAY and press key 13** to undo the last roll.

### The performance knobs

On the Perform page, K1–K4 are **Timbre** (the voices' tone), **Groove** (the rhythm and the drums), **Space** (the
effects; **Shape** in a Dry patch: the drums' envelopes as a kit) and **Motion** (how the layers move). The centre is
the patch as rolled. Each knob is not a single setting but a **path** through a dozen or more: every setting on it
goes its own way, so a full turn travels through several sounds. **REC + HOME** rolls new paths; **SAVE + HOME** locks
them. Hold PLAY for the performance keys: scenes 1–4 (tap to recall, hold to store), tap tempo, fill, freeze, latch,
mutes, undo and panic.

### Play it as a synth

The keys always play, with or without PLAY. **Stopped**, nothing moves and nothing plays by itself: MANIFOLD is a
polyphonic synth, every key a note of the scale (key 1 low, each key up the next note), always in tune with the
patch. OCT − and OCT + shift by a scale period. Each layer's **Contour** (EDIT and ARP, *Envelope* tab, K3) shapes its
notes: high is struck (a fast rise and fall), low is bowed (a slow rise, long hold and slow release). The voices (ENV
and GLO) set the tone and the effects (FX) are as they are in the patch. **Setup → Keys** chooses which layer the keys
play (or the drums), and **Latch** (PLAY held, key 8) holds notes for a drone. MIDI notes in (USB, Bluetooth or the TRS
jack) play the scale note nearest their pitch, with velocity. Press PLAY to let the patch play around you again.

### Tuning: just or equal

MANIFOLD tunes in **just intonation**: every note is a whole-number ratio of the scale's root (3/2, 5/4, 7/4…), and
every partial of every note a whole-number multiple of it, so chords fuse instead of beating, and intervals outside
the piano's, such as the natural seventh (7/4), are notes of their own. Most other instruments use **12-tone equal temperament**, where every semitone is the
same size and every interval but the octave is slightly out of tune. **Temper** (SEL, the *Temper & surface* tab, K1)
moves every note continuously from its just ratio (Just) to the nearest equal-tempered pitch (Equal (12-tone)). The
root is always an equal-tempered pitch at A = 440 Hz, so even in Just the root agrees with other instruments.¹

¹ **To play with other synths or a band in 12-tone equal temperament:** SEL, *Temper & surface* tab, turn K1
fully right (the screen shows **Equal (12-tone)**); on the *Tuning* tab set **Root** (K3) to the song's key; for MIDI
out to other hardware choose Setup → MIDI out → **Tuning: Standard** (MPE sends the just pitches as pitch bends).
Temper is never rolled or put on a knob, so it stays while you roll; it is stored with a patch, and a fresh power-on
patch starts at Just. Scales built on high harmonics move furthest (the 7th harmonic by about a third of a semitone,
the 11th by half) and lose their character; a mode or the Chromatic scale gives a conventional keyboard.

---

## Install

MANIFOLD is a single firmware file, `manifold-VERSION.fwsc`. Installing it replaces the FM-1's current firmware;
you can go back to the stock firmware or any other FM-1 firmware the same way.

**From the web installer** (Chrome or Edge, desktop):

1. Connect the FM-1 to the computer with a USB data cable and switch it on.
2. Open the installer page on the MANIFOLD site and press **Connect**. Pick the FM-1 when the browser asks.
3. Press **Install** and wait for it to finish. The FM-1 restarts into MANIFOLD.

**From the command line** (with [fm1-base](https://github.com/charlesvestal/fm1-base) checked out):

```
python3 tools/fm1_install.py manifold-VERSION.fwsc
```

**Try it first in the browser.** The MANIFOLD site has the whole firmware running in WebAssembly on a drawn FM-1
(the *emu* page): click the buttons, drag the knobs, play the keys with the mouse or the computer keyboard. It
sounds the same as the FM-1, and patches you store there are kept in the browser.

**After installing**, the FM-1 starts on a fresh patch every time: a Groove-style roll on the 1-3-5-9 hexany with
its root on A1. It is silent until you press PLAY (the keys play at once).

---

## How MANIFOLD thinks

### The scale is a map

The notes of the scale are laid out like a tongue drum: the root (the **ding**) and the scale's notes over one to
six octaves. Each note has a place on a **Tonnetz**, a map where neighbours are consonant: fifths run along one
axis, major thirds at 60°, natural sevenths at −60°. On the Perform page this map is the square on the left; each
small square is a note.

### Two layers of moving points

Each **layer** is a **generator**: one to eight **points** that move across the map along a **motion** (a
continuous function of time: an orbit, a chaotic flow, a run up the scale, a chord progression…). Each point also
carries an **envelope**, a loudness that changes continuously (a pulse, a swell, its own speed, the rhythm…).

A note sounds as loud as the points near it, weighted by their envelopes. A point sitting on a note is that note; a
point between two notes sounds both, crossfaded; a point crossing the map glides from note to note. **Reach** sets
how far a point's influence spreads.

Layer 1 is drawn as crosses, layer 2 as dots. The layers can also act on each other instead of both playing (see
[Route](#route-one-layer-shaping-the-other)).

### Each layer has a voice

Every sounding note is a bank of up to 16 oscillators at exact harmonics of the note. A layer's **voice** (Voice 1,
Voice 2) is one smooth function that sets all their levels and phases from a few controls: where the spectrum
peaks, its slope, its stride, its phase pattern. Anything can move these while notes ring and the tone follows
without a seam.

### The rhythm is a landscape, not a grid

There is no step sequencer. The rhythm is a **stream**: a sum of smooth peaks at the bar, the half bar, the beat and
faster divisions (in twos, in threes, or both). It shades every envelope (Grip) and its peaks above a threshold are
**events** that the **drums** play. Swing, three-against-four, rotation and long-form variation are all continuous
controls on that one function.

### The drums ride the synth

The four drums (kick, snare, hat, tom) make no sound of their own and never change a note's pitch. A hit puts an
envelope and a change of colour (brighter, snappier) on a note that the synth is already playing. So the drums are
always in tune and always part of the same sound.

---

## The screen

![A typical page](img/manual/voice1-1.png)

Every page has the same three parts.

**Header.** The page's name and the part of it you are on. On the right: the **tempo** in BPM and the **transport**
(a triangle while it runs, a square when stopped), plus **BT** while Bluetooth audio streams. On the Perform page, a
**battery** gauge sits left of the tempo: 0–3 bars, amber at one bar or none, or **USB** when a computer powers the
FM-1. When you change something, its name and new value show in the header for a moment.

**Middle.** Four rows, one for each knob, K1 at the top. Each row shows the setting's name, its value in real units
(Hz, ms, BPM, notes, names) and a bar. A dash means the knob does nothing on this tab.

**Footer.** The page's tabs; **SELECT** moves between them (the underlined one is shown). Below them, a reminder of
what the buttons do on this page.

**Color themes.** Nine: Glacier, Ember, Moss, Rose, Violet, Sunset, Lagoon, Gold and Mono (Setup → General → K1).

---

## Controls at a glance

### Knobs

| Control | On the Perform page | On any other page |
| --- | --- | --- |
| K1–K4 | the performance knobs: Timbre, Groove, Space, Motion | the four rows of the tab shown |
| SELECT | tempo (one BPM a detent) | the tabs |
| PRESET | choose a style and roll a patch in it (once the knob rests) | same |
| ALGO | the next / previous seed: a new patch, the same one every time for a given seed | same |

### Page buttons

| Button | Opens | What is there |
| --- | --- | --- |
| HOME | Perform (from Perform: Setup) | the map, the knobs, what is playing |
| SEL | Scale | scale, rotation, root, octaves; temper, surface |
| ENV | Voice 1 | layer 1's sound |
| EDIT | Layer 1 | layer 1's motion, points, envelope, path view |
| SEQ | Drums | the rhythm and the four drums |
| ARP | Layer 2 | layer 2's motion, points, envelope |
| LFO | Modulation | the modulation matrix, LFO 1–6 |
| FX | Effects | fuzz, filter, delay, reverb, compressor, output |
| GLO | Voice 2 | layer 2's sound |

### Buttons and gestures

| Gesture | Does |
| --- | --- |
| PLAY (tap) | start / stop the generators and the rhythm (the keys always play) |
| PLAY (hold) | the performance keys appear on the white keys (see below) |
| REC (tap) | roll the sound: both voices, both layers, the rhythm and the drums (never the scale, effects or knobs; two times in five it makes the patch Dry, which sets the effects to nothing; from a Dry patch to a wet one it rolls the effects too, unless they are locked) |
| REC (double tap) | roll everything except the scale |
| REC (hold 0.7 s) | mutate: a small nudge to every continuous setting |
| REC + a page button | roll only that page (REC + FX or REC + LFO: the matrix, the LFOs and the effects together) |
| REC + HOME | roll the performance knobs' paths |
| REC + SEL | roll the scale (the only way a roll changes it) |
| SAVE (tap) | the Patches page: the banks and their slots (tap again: back to the page you were on) |
| SAVE + a page button | lock / unlock that page: rolls leave a locked page alone |
| SAVE + HOME | lock / unlock the performance knobs |
| SAVE + key 1–16 | store the patch in that slot of the active bank |
| HOME + key 1–16 | load the patch in that slot of the active bank |
| SAVE + OCT+ | keep the settings (theme, octave, MIDI, Bluetooth, load, keys); SAVE + OCT− does nothing |
| OCT − / OCT + | move the keys down / up a scale period (−3 to +3); on the Patches page, choose the bank |

---

## The Perform page

![Perform](img/manual/perform.png)

**The map** (left). Every note of the scale where it sits on the surface. A note lights up as it sounds (brighter is
louder; a held key is white and larger). Layer 1's points are crosses, layer 2's are dots; each is brighter while
its envelope is up.

**What is playing** (right, top to bottom):

- the **scale**;
- the rhythm's **engine** (Threshold or Lock) and the **surface** (Plane, Torus, Sphere);
- the **layers**: "Orbit / Drift" when both play; when one layer shapes the other, the carrier and how it is
  shaped: "Lorenz x Pass" (multiplies), "Lorenz + Pass" (adds), "Ladder on Orbit" (moves);
- the **style** and the **seed** (#34334), or the **patch** number if one is loaded.

**Chips.** LATCH, FREEZE and FILL light while those performance keys are on. L1, L2, GEN and DRM are lit while
layer 1, layer 2, the generators and the drums are on (DRM also flashes on each hit).

**The performance knobs** (bottom). Four bipolar bars. The centre is the patch as rolled.

![Turning a knob](img/manual/perform-knob.png)

### The performance knobs

Each knob rides a **path** through most of the settings of its part of the instrument. Turning it does not just
scale one value: every setting on the path goes its own way, and the half-way point can lean somewhere different
from the end, so a full turn is a journey through several different sounds. The header shows how many settings ride
the knob as you turn it.

| Knob | Part of the instrument | Always on its path |
| --- | --- | --- |
| **Timbre** | both voices (their Colour too), both layers' Balance, the drums' Bright | Tone, Center, Band, Tilt and Depth of the voices; Balance |
| **Groove** | the rhythm and the drums (the hits' Pluck and Odd / even too), the drums' levels (±4 dB at most) | Density, Swing, the drums' levels; in a genre style, each pattern (to its busier or sparser relatives, a few lines along the library) |
| **Space** | the effects | filter cutoff and resonance, reverb mix, size, decay and damp, delay send and feedback |
| **Motion** | both layers, the path view and the LFOs, the patterns' rotations and fills, the layers' levels (±3 dB at most, one up as the other goes down: which layer leads) | layer 1's Rate and Reach, the layers' levels; in a genre style, often the hat's and the fourth part's rotation (by 8ths) |

**A Dry patch** (two rolls in five; a quarter before 1.0.7) starts with its effects doing nothing: no fuzz, the filter wide open, nothing
sent to the delay or the reverb. An effect set so is not run at all, which leaves the FM-1 more headroom (the delay and
the reverb stop once their tails have rung out; the filter glides out over about 30 ms). They are still there: turn one
up on its FX page and it runs as in any patch. The Space knob becomes **Shape** (FX → Output, K4, **Space knob**:
Shape or Effects): the room the instrument makes itself instead of an effect's.

| Knob (Dry) | Part of the instrument | Always on its path |
| --- | --- | --- |
| **Timbre** | as above, but for the voices' Width, Spin and Pluck | as above |
| **Groove** | as above, but for the drums' envelopes, Snap, Pluck and the duck; with the compressor's **Pump** | Density, Swing, Pump, the drums' levels |
| **Shape** | each drum's Attack, Hold, Decay, Curve, Settle, Snap and Pluck; the kick's duck; the layers' Contour; the voices' Pluck, Width and Spin; the compressor's attack | the drums' envelopes as a kit: one way the kick and the tom bloom long and open while the snare and the hat stay tight and snappy, the other way it all pulls in close (which way, and how far, is rolled); the layers' Contour; the voices' Width |
| **Motion** | as above, but for the layers' Contour | as above |

A Dry roll also sets the kit's roles: the kick and the tom long, the snare and the hat short and snappy. Its routes
in the matrix go nowhere near the effects.

The master, the drum bus level and Temper are never on a knob. **REC + HOME** rolls new paths; **SAVE +
HOME** locks them so rolls keep them. The knobs can also be turned by the modulation matrix: see
[Knobs as destinations](#knobs-as-destinations). A white tick on a knob's bar shows where the matrix has it.

### The performance keys

![PLAY held](img/manual/perform-keys.png)

Hold **PLAY**: the white keys become the performance keys and the screen shows the map. Let go of PLAY to return.

| White key | Does |
| --- | --- |
| 1–4 | **Scenes 1–4**: tap to recall (a 1.2 s morph); hold a second to store the current sound in it |
| 5 | **Tap** tempo: from the second tap, averaged over the last four; a pause of over 2 s starts again (so taps reach down to 30 BPM; SELECT goes to 10) |
| 6 | **Fill**: both layers' energy up while held |
| 7 | **Freeze**: the reverb holds what it has, forever, until pressed again |
| 8 | **Latch**: keys stay on after you let go (press a latched key again to release it) |
| 9 | mute / unmute **Layer 1** |
| 10 | mute / unmute **Layer 2** |
| 11 | mute / unmute the **Generators** (the keys still play) |
| 12 | **Drums** on / off |
| 13 | **Undo** the last roll, mutation or seed |
| 14 | **Panic**: everything off |
| 15 | **Drum keys**: the keys play the drums, and back |

---

## Playing the keys

![A held key](img/manual/perform-key.png)

**The keys play the scale, not the chromatic keyboard.** Key 1 is a low note of the scale and each key up is the next
note of the scale in pitch order, so every key is in tune with the patch. OCT − and OCT + shift the keys by a scale
period (−3 to +3).

**A key is an input into the system, not a note-on.** Pressing a key sounds its note directly (with the key's
velocity) and **pulls the layer's points toward that note** over about 0.15 s, so the motion bends around what you
play; it lets go when you release. The patch's own envelope shapes the key's note: a sharp Contour gives a struck
note (fast rise and fall), a soft one a bowed note (slow rise, long hold, slow release). While the generators run, a
held key also breathes with the layer's envelope.

**Which layer the keys play** is set in Setup → Keys: Layer 1, Layer 2, Both layers (the default since 1.0.5), or
Drums. **Whether a key pulls the moving points** toward its note is the patch's own setting (Layer 1 → Keys): off,
a key plays exactly its note, for melodic lines.

**Drum keys** (performance key 15, or Keys play → Drums): each key plays one of the four drums (kick, snare, hat,
tom, repeating up the keys) at its velocity. The hit rides a note the drum layer is playing, as rhythm hits do.

**Latch** (performance key 8) holds every key you press until you press it again.

---

## Rolls, styles, locks and undo

A **roll** makes a new patch from a random **seed**, shaped by the current **style**. The same seed and style always
give the same patch.

| Style | Character |
| --- | --- |
| **Wild** | everything anywhere: the whole range of every setting (45–165 BPM); a quarter of the rolls take genre patterns too |
| **Hip-hop** | dusty, mellow keys through a filtered sampler, a heavy low bass, a touch of saturation; minor pentatonic, Dorian, blues; boom-bap, lo-fi, West Coast, trap and Southern [patterns](#patterns), heavily swung (about 62–100 BPM); a long deep kick, short hats |
| **Groove** | rhythmic and bright, runs and loops on the beat, drums on most of the time (80–132 BPM) |
| **Ambient** | slow, soft, wide; swells and drifts, down to a single slow note; drums rarely (40–90 BPM) |
| **Cosmic** | strange scales and combs, passing motions, anything from sparse and slow to dense (60–135 BPM) |
| **House** | warm, round chords and stabs over a deep bass, the compressor pumping; minor and Dorian; classic, deep, tech, techno, garage and disco patterns: the kick on every beat, the clap on 2 and 4, off-beat and 16th hats (112–142 BPM) |
| **Machine** | glassy, metallic, hollow combs and twangs; the harmonic series, whole tone, the hexany; an IDM rhythm, drums always on, the matrix moving the rhythm's own controls firmly (80–170 BPM) |
| **Breakbeat** | bright and edgy rave stabs, a growling bass, drive often; minor, Phrygian, Hijaz; the classic breaks, big beat and nu-skool (mostly 96–140 BPM), a third of the rolls jungle and drum'n'bass at 160–178 |
| **Funk** | clavinet and slap-bass tones, bright, buzzy and plucked, loud notes snapping open; Dorian, Mixolydian, minor pentatonic, blues; six eras equally: James Brown, New Orleans second line, P-Funk, go-go, Minneapolis, modern funk and neo-soul (82–130 BPM); a tight kit, ghost notes everywhere |
| **Latin** | struck wood and tuned bars (marimba, vibes), a montuno's keys, a round tumbao bass, a warm room; major, harmonic minor, Phrygian dominant; seventeen styles equally: son montuno, salsa, mambo, cha-cha-chá, songo, rumba guaguancó, bembé (6/8), samba, bossa nova, baião, partido alto, samba-reggae, reggaeton, merengue, bachata, cumbia, tango and habanera. The kick is the bass drum (bombo, surdo, tumbao), the snare's part the rim or clave, the hat a shaker, cáscara or güira, the fourth the congas, tamborim or bongos |
| **Indie** | guitars, jangly to fuzzed, strummed and ringing, drive on more often than not, a big room; major, Mixolydian, Aeolian; seven styles equally: straight 8ths, motorik, dance-punk, post-punk, garage rock, dream pop and shoegaze, punk (66–180 BPM) |

The genres (Hip-hop, House, Breakbeat, Funk, Latin, Indie) build their drums from a **pattern library trained on 678
genre patterns**, over a hundred for each (see [Patterns](#patterns)). Each roll walks a decision tree through its
genre's corpus: a root pattern (an era or sub-style, all of a genre's equally likely in Funk, Latin and Indie), a kick,
then a snare that goes with that kick, then hats that go with both, then the swing those hats are played with. The
parts mostly keep to the root's style (three times in four or more), and now and then borrow from another of the
genre's. Most rolls are combinations the corpus never has, but they keep its rules. The 6/8 feels (bembé) are played
with triplet swing: each beat's two 8ths land on the first and third triplet.

**The matrix in rolls.** Every curated style rolls 3–10 routes (Machine up to 12: two to four more on the rhythm's own controls) to a wide list of musical places
(the voices, the motions, the path, the rhythm, the drums, the effects), deep enough to be heard, each limited so
nothing shuts or runs away. A route that would feed a layer back into itself (its own envelope moving its own path)
is never rolled. The genre styles sometimes route a slow LFO or the bar's random to a pattern, so it walks among its
relatives.

**What a roll changes:**

- **REC (tap)** rolls the sound: both voices, both layers, the rhythm and the drums, and Dry or not (two in five Dry).
  Its knobs' paths stay as they were; a double tap (or REC + HOME) gives a Dry patch Shape's kit paths too.
- **REC (double tap)** rolls everything but the scale: the sound, the matrix, the LFOs, the effects and the knobs.
- **REC + a page button** rolls only that page. **REC + SEL** is the only roll that changes the scale.
- **PRESET** rolls everything in the chosen style; **ALGO** steps through seeds.
- **REC (hold)** mutates: every continuous setting moves a little from where it is.

**Locks.** Hold **SAVE** and press a page button to lock that page: rolls and mutations leave it alone. The header
shows LOCKED on a locked page. Lock the scale, a voice you like, or the matrix (SAVE + LFO) and keep rolling the
rest.

**Undo.** Performance key 13 undoes the last roll, mutation or seed change.

**Morphs.** A roll glides into the new patch (about 1.5 s; 0.3 s in Wild) instead of jumping. Settings that rebuild
the map (scale, rotation, root, octaves, surface) cause a short silence of about 15 ms.

---

## Patches and scenes

![Patches](img/manual/patches.png)

**Patches.** 64 slots: four banks (A–D) of 16, one slot per key. **SAVE + key** stores everything in that slot of
the active bank: every setting, the matrix, the knobs' paths and positions, the locks and the four scenes. **HOME +
key** loads it. While SAVE or HOME is held, the keys that hold a patch in the active bank light up. The Perform page
shows "patch B5" while a loaded patch is unchanged by rolls. If a patch was the last thing you loaded, it comes back
at power-on, with its bank active; otherwise the FM-1 starts on a fresh roll.

**The Patches page.** A tap on **SAVE** opens it; another tap goes back to the page you were on (HOME and the page
buttons leave it too). It shows the four banks with how many patches each holds, and the active bank's 16 slots:
a stored patch lit, the one playing bright.

| On the Patches page | |
|---|---|
| K1, or OCT − / OCT + | choose the bank (A–D) |
| SAVE + key 1–16 | store in that slot of the bank shown |
| HOME + key 1–16 | load from it |
| K4 (turn right) | keep the settings; the row says "unsaved" when there is something to keep |

The bank stays active when you leave the page, so SAVE + key and HOME + key work on it from any page. Storing over a
slot that holds a patch replaces it, without asking.

**Patches and firmware updates.** Updates keep the patches and the settings. 1.0.4 moved the patches into a shared
area of the flash to make room for 64: the 16 slots of earlier versions become bank A by themselves at the first
power-on. Do not go back to a version before 1.0.4 afterwards: it would miss some patches and forget the Bluetooth
speakers, and storing a patch there could overwrite others.

**Patches from before 1.0.5** load: the patterns' settings are new and arrive off (every drum Linear), with the keys'
pull on, and each layer's Balance at Balanced. They may sound different: the voices themselves changed in 1.0.5. A patch saved by 1.0.5 is in a newer format that 1.0.4 cannot read (it shows those slots as
Empty), so stay on 1.0.5 or later once you have saved with it.

**Patches from 1.0.5** load in 1.0.6 as they were (wet, Drum notes at Follow). A patch saved by 1.0.6 is in a newer
format again, which 1.0.5 shows as Empty. **1.0.7** keeps 1.0.6's format: patches move between them both ways. **1.0.8**'s pattern library is larger, so its
patches are in a new format again: 1.0.5–1.0.7 patches load with exactly the drum patterns they had (each moved to the
same line in the new library), but a patch saved by 1.0.8 shows as Empty in 1.0.7 and earlier.

**Scenes.** Four snapshots inside the current patch, on performance keys 1–4. **Hold** a scene key for a second to
store the current sound; **tap** it to recall, with a 1.2 s morph. Scenes are saved with the patch.

**Settings** (theme, key octave, MIDI channel and output, Bluetooth, load caps, keys, the style) are kept with
**SAVE + OCT+** (hold SAVE first), or K4 on the Patches page. The SAVE button lights while there are changes not kept
yet; without keeping them, the next power-on brings back the settings as they were. Storing a patch keeps them too.

---

## Scale (SEL)

![Scale: Tuning](img/manual/scale-1.png) ![Scale: Temper & surface](img/manual/scale-2.png)

| Tab | Knob | Setting | Range | What it does |
| --- | --- | --- | --- | --- |
| Tuning | K1 | **Scale** | 41 scales | the set of notes (below) |
| | K2 | **Rotation (the ding)** | each note of the scale | which note is the root: the same notes, another mode (named "/2", "/3"…) |
| | K3 | **Root** | A0–A3 | the ding's pitch |
| | K4 | **Octaves** | 1–6 | how far the scale extends above the ding (at most 32 notes: a 7-note scale reaches 4 octaves, a 5-note one 6). Rolls give 4–6, six most often |
| Temper & surface | K1 | **Temper** | Just … Equal (12-tone) | moves every note continuously from its just ratio to the nearest equal-tempered pitch, to play along with other instruments |
| | K2 | **Surface** | Plane, Torus, Sphere | the shape of the map: the plane reflects at its edges, the torus wraps around, the sphere has no edge (a path over a pole comes down the other side) |

Changes to Scale, Rotation, Root, Octaves and Surface take effect when the knob rests (about 0.3 s) and rebuild the
map (a moment of silence). Temper retunes without a rebuild.

**The 41 scales**, in five groups:

- **Harmonic**: Harmonic series (the ding's partials 1, 2, 3…, not folded into an octave), Odd harmonics,
  Harmonics 4-8, 6-12, 8-16, Harmonic pentatonic, Undertones 8-16.
- **Tongue drum and handpan**: Hexany 1-3-5-9, Golden Gate, Celtic minor (Amara), Kurd, Integral, Pygmy, Akebono,
  Sabye, Aegean, Hijaz.
- **Pentatonic and smaller**: Fifths, Suspended, Major pentatonic, Minor pentatonic, Egyptian, Septimal pentatonic,
  In, Kumoi.
- **Modes**: Major, Dorian, Phrygian, Lydian, Mixolydian, Minor, Harmonic minor, Melodic minor, Acoustic (overtone),
  Phrygian dominant, Double harmonic.
- **Other**: Blues, Major hexatonic, Whole tone, Diminished, Chromatic.

Every note is a whole-number ratio of the ding. Notes that hold the key more strongly (the ding, its fifth, its
third) have more **tonal weight**: the motions lean toward them. Each layer's **Balance** (Layer → Notes) decides how
its loudness spreads over the notes' range: evenly, toward the bass, or toward the middle.

---

## Voice 1 and Voice 2 (ENV and GLO)

![Voice: Tone](img/manual/voice1-1.png) ![Voice: Spectrum](img/manual/voice1-2.png) ![Voice: Voice](img/manual/voice1-3.png)

Voice 1 is layer 1's sound, Voice 2 is layer 2's. Both pages are the same. Every note is up to 16 oscillators at the
note's exact harmonics; these settings shape all of them at once, live, and the sound is normalised so they change
the colour, not the volume.

| Tab | Knob | Setting | Range | What it does |
| --- | --- | --- | --- | --- |
| Tone | K1 | **Tone** | 250 Hz – 16 kHz | where the partials stop: above it they fall 24 dB an octave. Not a filter: each partial's level is set by its own frequency, so nothing rings |
| | K2 | **Tilt** | 0–100 % | the spectrum's slope: dark (k^−1.6) through a saw's slope (k^−1) to bright (k^−0.4) |
| | K3 | **Detune** | 0–100 % | doubles each note's first three partials with a copy up to 14 cents away (each note its own amount and side): slow beating and stereo movement. 0 = no copy |
| | K4 | **Wave** | 0–100 % | each oscillator from a sine to a richer wave (adds its own 2nd and 3rd harmonics) |
| Spectrum | K1 | **Center** | partial 1–12 | where a formant-like lift sits in the series (up to +9.5 dB once above the 5th partial) |
| | K2 | **Band** | 0–100 % | how wide that lift is |
| | K3 | **Stride** | every partial … every 4th | thins the spectrum: 2 = odd partials only (hollow, clarinet-like), 3 = every third… continuous between |
| | K4 | **Phase** | 0–100 % | the phase pattern across the partials: aligned (a sharp, buzzy pulse), Schroeder at 50 % (smooth, glassy), then dispersed (a twangy, stiff-string quality) |
| Voice | K1 | **Bloom** | 0–100 % | louder is brighter, note by note: quiet notes lose their upper partials |
| | K2 | **Width** | 0–100 % | stereo: each note is panned by its place on the map |
| | K3 | **Register** | Octave down, As tuned, Octave up | the whole layer an octave down or up |
| | K4 | **Layer level** | 0–100 % | the layer's volume |
| Colour | K1 | **Odd / even** | 0–100 % | the partials above the fundamental blended toward a square's (odd, falling 1/k: hollow, reedy) under 50 %, toward the even ones (octaves: nasal, organ-like) over it. 50 % is the bank as it is; the fundamental is never changed |
| | K2 | **Pluck** | Off … 100 % | after a note's level rises (or a drum hits it), its upper partials fade faster than the lower: the top first, as a plucked or struck string. Higher is faster (1.5 s … 25 ms). It also lightens the load: fewer partials sound. The undertones do not fade |
| | K3 | **Depth** | 0–100 % | how much of each note reaches below it: its undertones an octave, two and three down (in tune). At 100 % the first is as loud as the fundamental. The low end's weight; on the Timbre knob |
| | K4 | **Clarity** | Open … 100 % | how many partials, of all the layer's notes, each octave may hold: the low octaves the fewest. Up, the low end belongs to the lowest notes and the chords above stay clear; Open, everything reaches down. On no knob: it switches partials, so set it by hand (a turn can step) |

![Colour](img/manual/voice1-4.png)

Rolls set the Colour per layer: Odd / even is left as it is in about half of them (Ambient gently either way), Pluck
in about half the rhythmic rolls (Machine's fast, Hip-hop's slower, a slow one sometimes in Ambient). Wild takes
anything.

A layer plays up to 8 notes at once (Setup → Load). When more want to sound, the quietest fade out over 8 ms; no
note is ever cut.

---

## Layer 1 and Layer 2 (EDIT and ARP)

![Layer: Motion](img/manual/layer1-1.png) ![Layer: Points](img/manual/layer1-2.png)
![Layer: Envelope](img/manual/layer1-3.png) ![Layer: Path view](img/manual/layer1-4.png)

The two layers are the same, except that **Path view** is on Layer 1's page and moves both.

| Tab | Knob | Setting | What it does |
| --- | --- | --- | --- |
| Motion | K1 | **Motion** | the function the points follow (12, below) |
| | K2 | **A** | the motion's first control (its name changes with the motion) |
| | K3 | **B** | the motion's second control |
| | K4 | **Rate** | how fast: cycles per bar, from 1 per 64 bars to 4 a bar, snapped to whole-number ratios so a repeating motion lines up with the rhythm |
| Points | K1 | **Points** | 1–8 points: how many notes the layer can sound at once |
| | K2 | **Spread** | how far apart the points are along the motion (0 = all together) |
| | K3 | **Reach** | how far a point's sound spreads to nearby notes: 0 = mostly the note it is on, 100 % = its neighbours at about half |
| | K4 | **Energy** | the layer's drive (also the depth when it shapes the other layer) |
| Envelope | K1 | **Envelope** | how each point's loudness moves (8, below) |
| | K2 | **Envelope rate** | for Pulse and Swell: 1 per 16 bars to 4 a beat (exact ratios to the bar). For Drums: which drum |
| | K3 | **Contour** | the envelope's shape: soft and broad … sharp and narrow |
| | K4 | **Route** | Plays, or shapes the other layer (below) |
| Path view | K1 | **Path scale** | how much of the map the motions cover (1.0 = all the notes) |
| | K2 | **Path rotation** | turns every motion on the map: a different set of neighbours along the same path |
| | K3, K4 | **Path x, Path y** | slides the motions across the map |
| Notes | K1 | **Balance** | **Dark** … **Balanced** … **Bright**. How the layer's loudness spreads over its notes. A motion visits some notes more than others (some the frame's edge, some its centre, some the low notes); Balanced divides that out, so over a long run every note of the range sounds about as much. Dark leans toward the bass (at 100 %, about 4 dB an octave), Bright toward the middle of the range (its ends down). The layer's loudness is kept. On the Timbre knob |
| Notes (Layer 1) | K2 | **Keys pull the points** | **On**: a key sounds its note and pulls the nearest point toward it (the music bends round what you play). **Off**: a key plays just its note, the points carry on around it: for melodic lines |

![Layer 1: Notes](img/manual/layer1-5.png)

**Keys pull the points** is part of the patch, and rolls switch it off in a share of patches (half in the beat
styles). It applies to both layers.

When a motion's discrete choice changes (a ratio, a figure, the motion itself, the number of points), each point
glides from where it is onto the new path over about 0.3 s. A point never jumps.

### The twelve motions

| Motion | What you hear | A | B |
| --- | --- | --- | --- |
| **Orbit** | a closed loop (a Lissajous figure): an ostinato that returns every cycle | **Ratio** (1:1, 2:1, 3:2, 4:3, 5:4, 5:3, 3:1, 5:2): the figure | **Phase**: opens it from a line to a loop |
| **Ladder** | runs through the notes in pitch order, resting on each: scales and arpeggios by step | **Span**: 3–12 notes | **Contour**: up, up and down, down |
| **Swarm** | the points are coupled oscillators: they gather into a chord, then scatter | **Coupling**: how strongly they pull together | **Spread**: how different their speeds are |
| **Lorenz** | chaos with two lobes: circles one group of notes, then flips to the other | **Chaos** (ρ 20–50): how often it flips | **Zoom**: how much of the attractor fills the map |
| **Motif** | a small figure stated at the ding, again, answered away, then home (A A B A) | **Figure**: the figure's shape (a ratio) | **Answer**: at the fifth, at the third, or alternating |
| **Drift** | a smooth random walk: slow, aimless, never repeating | **Detail**: a finer wander on top | **Density**: how many turns a cycle |
| **Billiard** | straight runs that bounce off the edges and a central bumper: arpeggios that fold back | **Angle**: the heading (0–180°) | **Bumper**: its size; with it, the runs never repeat |
| **Gravity** | the point orbits the key's own notes, falling toward the ding, swinging round and away | **Wells**: the ding; ding and fifth; ding, fifth and third | **Energy**: low = tight orbits, high = wide swings |
| **Stride** | moves only as the rhythm's stream peaks and turns while it rests: a melody that steps with the beat | **Turn**: how much and which way it turns | **Stride**: how far each step goes |
| **Loops** | each point is a tape loop on its own note, of lengths 5, 7, 8, 11, 13, 17, 19, 23: they never line up twice | **Notes**: how many of the key's notes they share (2–6) | **Swell**: how long each note sounds in its loop |
| **Erode** | one six-note figure repeated, wearing away note by note (weakest first), then renewed | **Life**: passes until renewal (4–64) | **Wear**: how much of it is worn away |
| **Chords** | the points are a chord and the chord changes: harmonic rhythm, four chords a cycle | **Progression**: I IV V I, I VI IV V, I III IV II, I V VI IV, I I IV I, I VII VI V | **Voicing**: thirds, fourths, fifths |

### The eight envelopes

| Envelope | Loudness follows… | Contour |
| --- | --- | --- |
| **Pulse** | a periodic peak at Envelope rate, locked to the bar | soft swell … sharp attack and narrow peak (the steepest rise is still a few ms, never a click) |
| **Swell** | a periodic raised-cosine swell at Envelope rate | how narrow the swell is |
| **Speed** | the point's speed: loud while it moves, quiet while it rests | how steep |
| **Pass** | nearness to a note: it sounds as it passes through notes, silent between | how narrowly |
| **Height** | the motion's third dimension (a flow's height, the distance from the centre, the speed round a well) | how steep |
| **Hold** | steady (the motion alone shapes the music) | — |
| **Stream** | the rhythm's stream itself: it sounds where the drum events are | only the strongest events … all of them |
| **Drums** | a drum's envelope (Envelope rate picks kick, snare, hat, tom, or all) | how sharp |

Every envelope is also accented by the rhythm's stream, by the amount set with **Grip** (Drums → Lock & motion).

### Route: one layer shaping the other

With **Route** set to anything but Plays, a layer falls silent and acts on the other one instead. Its **Energy** is
the depth. Point *j* of the played layer takes point *j* of the shaping layer (wrapping around).

| Route | What it does | Example |
| --- | --- | --- |
| **Plays** | both layers sound | — |
| **Moves** | its path is added to the other's path | a Lissajous loop carried along a Lorenz orbit |
| **Adds to** | its envelope is added to the other's | Stream accents on a chaotic swell |
| **Multiplies** | its envelope gates the other's | a chaotic flow gated by a pulse |

If both layers are set to route, layer 2 shapes and layer 1 plays ("Plays (2 routes)").

---

## Drums and rhythm (SEQ)

The Drums page has 24 tabs. The first ones are the ones you reach for most.

![Groove](img/manual/drums-1.png) ![Patterns](img/manual/drums-patterns.png) ![Drums](img/manual/drums-4.png) ![Mix](img/manual/drums-6.png)

### How the rhythm works

The rhythm is a **stream**: a smooth curve over the bar that peaks at the downbeat, the half bar and the beats, and
(depending on the Groove settings) at eighths, sixteenths, triplets or both. It never stops changing smoothly; there
is no grid.

- In the **Threshold** engine, every peak of the stream above a threshold is an **event** with a strength. **Density**
  sets the threshold. **Drum choice** decides which drum plays each event.
- In the **Lock** engine, each drum is its own oscillator at a whole-number rate (kick 2 a bar, snare 2 offset by
  half, hat 8, tom 3), slightly detuned, and **Pull** couples them back into step: strongly pulled they lock to the
  grid; loosely, they slip and catch again.

The stream also shapes the synth: **Grip** accents every envelope with it, and the **Stream** envelope and **Stride**
motion follow it.

### Patterns

Each drum can either take part in the choice above (**Linear**: one drum per event, MANIFOLD's own drumming) or
play a **pattern**: a line from a library trained on 678 house, techno, breakbeat, drum'n'bass, hip-hop, funk, Latin
and indie patterns (108 kick lines, 77 snare, 25 hat, 62 percussion, each one or two bars of 16ths). The two live side by side,
a drum at a time: a patterned kick and snare with a linear hat and tom is the stream's own drumming over a genre
skeleton.

![Patterns](img/manual/drums-patterns.png) ![Pattern feel](img/manual/drums-feel.png)
![Rotate](img/manual/drums-rotate.png) ![Mutes](img/manual/drums-mutes.png)

**It is not a step sequencer.** A pattern says where in the metre a drum plays; when is still the stream's:

- **Swing** bends the steps as it bends the stream (at the 16ths or the 8ths, **Swing on**): the second of each
  pair moves from half way to two thirds (triplet swing) to three quarters, continuously.
- **Density** thins a pattern from its weakest steps: a step sounds only if the stream without swing is strong enough
  at its place in the bar (the 16ths go first, then the off-beats; the downbeat last). Ghost notes need more.
- **Fills**: on the second half of every fourth bar, a drum may play a busier relative of its line instead.
- **Ghosts**: the patterned drums also take part in the linear choice, quietly: MANIFOLD's own fills between the
  pattern's hits.
- **Rotate** delays a line by 16ths (the hats moved by two are off-beat hats); **Length** loops it over its first
  2–32 steps against the bar (a 12-step percussion line over a 16-step bar drifts round it: polymeter).
- **Mutes**: a drum that plays mutes another on the same instant: the kick clearing the hat, an open hat (the fourth
  part) choking the closed one, the snare clearing the linear drums under it.

The library is ordered from sparse to busy, with similar lines next to each other, so turning a pattern (by hand, with
the Groove knob or from the matrix) moves to a close relative, never a random jump. A pattern moved by a knob or a
route never falls back to Linear; only setting it to Linear by hand does.

### All the drum settings

| Tab | Knob | Setting | What it does |
| --- | --- | --- | --- |
| **Groove** | K1 | **Density** | how many events: the threshold the stream must cross |
| | K2 | **Swing** | the off-beat's timing: straight (0 %), triplet swing (about 67 %), hard (100 %) |
| | K3 | **Threes** | twos (eighths, sixteenths) … 3 against 4 (50 %) … threes (triplets) |
| | K4 | **Fine** | how far the subdivisions go: beats only … sixteenths and thirty-seconds |
| **Patterns** | K1–K4 | **Kick, Snare, Hat, Tom** | Linear, or a line of the library (1 … its count): the strip under it shows the steps, the ghosts dimmer, the step playing now white |
| **Pattern feel** | K1 | **Ghosts** | how much the patterned drums also take the linear layer's events (quietly) |
| | K2 | **Fills** | how often the second half of a fourth bar plays a busier relative |
| | K3 | **Swing on** | the level Swing bends for the patterns: 8ths or 16ths |
| **Drums** | K1–K4 | **Kick, Snare, Hat, Tom** density | each drum's share of the events (0 = that drum off) |
| **Levels** | K1–K4 | **Kick, Snare, Hat, Tom** level | each drum's level (Off … 0 dB). A light by the name flashes on each hit |
| **Mix** | K1 | **Drums** | on / off |
| | K2 | **Drum layer** | which layer's notes the drums ride: Layer 1, Layer 2, or **Split** (kick and tom on the lower-sounding layer, snare and hat on the higher) |
| | K3 | **Drum level** | all the drums |
| | K4 | **Accent** | how much an event's strength sets the hit's velocity |
| **Kick / Snare / Hat / Tom** | K1 | **Voice** | which of the drum layer's sounding notes the hit rides: lowest note … highest note |
| | K2 | **Attack** | 1–20 ms |
| | K3 | **Decay** | 20 ms – 3 s (to −35 dB) |
| | K4 | **Curve** | the decay's shape: exponential … straight |
| **Rhythm** | K1 | **Tempo** | 10–180 BPM |
| | K2 | **Rhythm** | the engine: Threshold or Lock |
| | K3 | **Form** | long-form variation: slow functions move the threshold, the sixteenths, the three-feel and the drum choice, so the pattern evolves over many bars (with all of it, it repeats only after 6,720 bars) |
| | K4 | **Turn** | shifts the threes against the twos by up to a third of a bar: every rotation is a different rhythm |
| **Drum choice** | K1 | **Drum choice** | **Select**: each drum bids for each event by its strength (Place) and the selector; **Count**: hat on every event, kick every n-th, snare between kicks, tom on strong leftovers |
| | K2 | **Selector** | the selector's speed (Select): 1 per 2 bars … 3 a bar; it makes kick and snare answer each other |
| | K3 | **Feedback** | (Select) how much a drum that just played steps back for the others |
| | K4 | **Kick every** | (Count) the kick every 2–8 events |
| **Lock & motion** | K1 | **Pull** | (Lock) how strongly the drum oscillators lock to the grid |
| | K2 | **Motion** | layer 1's path moves the threshold: the melody makes the rhythm busier or sparser |
| | K3 | **Evolve** | a slow random wander of both layers' A and B (and looser Lock timing) |
| | K4 | **Grip** | how strongly the stream accents every synth envelope |
| **Hold** | K1–K4 | per drum | how long the envelope holds at full level after the attack (0–50 ms) |
| **Place** | K1–K4 | per drum | which events it prefers: Strongest … Weakest (kick strongest, hat weakest). When a drum follows a layer: which point it follows |
| **Shape, duck, notes** | K1 | **Shape by** | None, Layer 1, Layer 2: every drum is multiplied by that layer's envelope, and its decays lengthen in a swell and shorten in a trough |
| | K2 | **Shape depth** | how much |
| | K3 | **Kick duck** | sidechain: the kick ducks the synth |
| | K4 | **Drum notes** | **Follow** … **Fixed**: from each hit riding one of the notes the motion is sounding (Voice: lowest .. highest) to a kit, each drum on its own pitch (see below). On the Groove knob |
| **Follow** | K1–K4 | per drum | **The stream** (the rhythm decides), or **Layer 1 / Layer 2**: the drum fires when one point's envelope of that layer rises past 1 − its density. A silent, shaping layer works too. A drum with a pattern plays its pattern instead |
| **Rotate** | K1–K4 | per drum | its pattern delayed by 0–15 16ths |
| **Length** | K1–K4 | per drum | its pattern looped over its own length, or its first 2–32 steps |
| **Mutes** | K1–K4 | per drum | what it mutes on the instants it plays: Nothing, the kick, the snare, the hat, the tom, or the linear drums |
| **Kick / Snare / Hat / Tom sound** | K1 | **Bright** | Dark … As is … Bright. Bright: the hit tilts the note's partials up at its start (up to +3 dB an octave); Dark: down toward its undertones, bringing them in (an octave down at up to the fundamental's level): a hit that starts low and settles up, the fall a pitch sweep gives a kick or a tom. The note's energy is kept and its fundamental never drops more than 6 dB. On the Timbre knob |
| | K2 | **Settle** | how fast that brightness settles (300 ms … 9 ms) and how far |
| | K3 | **Snap** | a push on the voice's Wave at the hit, past Wave's own top: a bite on the attack. It lasts as Settle says: 71 ms at 0, 25 ms in the middle, 9 ms at full |
| | K4 | **Pluck** | the hit plucks the note it rides: its upper partials fade first after the hit, as the layer's own Pluck does (1.5 s … 25 ms). Off: the layer's own. The note stays plucked until it rises by itself (or, 2 s without a plucking hit, lets go over a second) |
| **Odd / even** | K1–K4 | per drum | the hit pushes its note's colour toward hollow or even (as the layer's Odd / even), settling as its Bright does. None: the note's own |

**Drum notes** (1.0.6) runs from **Follow** to **Fixed**. Each drum has a pitch and a band, after the TR-808's: the
**kick** 45–76 Hz (the 808's is tuned near 56), the **tom** 90–185 Hz (its three toms), the **snare** 200–320 Hz (its
body near 238), and the **hat** the top octave of the layout (up to 4 kHz); each drum's **Voice** sets its pitch in its
band. At **Fixed**, a hit takes a note the motion is sounding inside its band (the one nearest its pitch), or else the
scale's nearest note to its pitch, sounding or not: always in tune, always in its range. Turned toward **Follow**, the
band widens (an octave each way at the middle), so the drums ride more of what the motion plays; at Follow they ride
it as before. Half way, a kick locks to the bass notes near it and falls back to its own pitch when there are none.
Toward Fixed each drum also takes its own spectrum while its hit settles: the kick and the tom a round **body** (the
fundamental, a little of the 2nd and 3rd, an undertone under the kick), the snare a **crack** (its middle partials,
3rd to 9th), the hat **sizzle** (only its partials above 4.5 kHz, the 7th, 11th and 13th included, which clash like the
808's six square waves). Odd / even still colours them, and a hit is no louder for its spectrum. Rolls: Fixed about a
quarter of the time in House, Hip-hop, Machine, Breakbeat, Funk and Indie (a third in Latin), between (0.3–0.8) about 45 %, seldom either in Ambient
and Cosmic. The kit's levels: the snare about 8 dB under the kick, the tom 6, the hat 13; the hat's decay a closed
hat's (20–60 ms, longer only where a style opens it).

![Kick](img/manual/drums-7.png) ![Rhythm](img/manual/drums-11.png) ![Kick sound](img/manual/drums-21.png) ![Odd / even](img/manual/drums-25.png)

**The drums never change pitch.** A hit swells a note that is already sounding and colours it, its undertones (Depth)
with it, so a kick on any note has weight. If the drum
layer is silent, the hit lands on the note under its loudest point.

With the drums on, the synth sits about 4 dB lower to make room.

---

## Modulation (LFO)

### The matrix

![Matrix](img/manual/mod-1.png) ![A route to a knob](img/manual/matrix-route.png)

The first tab of the LFO page. **24 routes**, each a **source** onto a **destination** at a **depth**.

| Knob | Does |
| --- | --- |
| K1 **Route** | which route (1–24) you are editing ("empty" if unused) |
| K2 **Source** | what modulates. **None** clears the route |
| K3 **Destination** | what is modulated |
| K4 **Depth** | −100 % … +100 % of the destination's range (on a knob: of its full turn) |

A new route starts at +25 % onto the Timbre knob.

**Sources**

| Source | Range | What it is |
| --- | --- | --- |
| LFO 1–6 | −1 … +1 | the six LFOs |
| Timbre / Groove / Space / Motion knob | −1 … +1 | where your hand has the knob |
| Output level | 0 … 1 | the instrument's loudness (an envelope follower) |
| Layer 1 onsets | 0 … 1 | a pulse each time layer 1's notes rise |
| Random (a bar) | −1 … +1 | a new random value each bar, reached smoothly across the bar |
| Path X, Path Y | −1 … +1 | where layer 1's main point is on the map |
| Velocity | 0 … 1 | the last key's velocity |
| Layer 1 motion, Layer 2 motion | 0 … 1 | that layer's loudest point's envelope: the generators as modulators |

Sources marked 0 … 1 push one way only from where the destination is set: use a negative depth to pull it down.

**Destinations**, in this order: the four **performance knobs**, the **LFO rates**, **Layer 1** (with the path view),
**Layer 2**, **Voice 1**, **Voice 2**, the **drums**, the **rhythm**, and the **effects**. Names say where each one is:
"Kick decay", "LFO 3 rate", "Layer 2 rate", "Voice 1 tilt". Every continuous setting is there except the master level.

### Knobs as destinations

A route onto a knob turns that knob for you, so every setting on its path moves together, exactly as if a hand were
turning it. A white tick on the Perform page shows where the matrix has the knob. Your hand still sets the centre
the route moves around. Knob sources always read your hand, never the routed position, so knob → knob routes cannot
loop. Rolls never route onto the knobs.

### Generators as sources

**Layer 1 motion** and **Layer 2 motion** let the generators modulate anything: the drums' envelopes, a voice, an
effect, a knob. Rolls with the drums on often add one or two Motion → drum routes. Two things to know:

- A layer's motion routed onto **its own** generator (its rate, energy, reach…) or onto the Motion knob is a feedback
  loop. It is bounded and stable, but it can settle at an extreme (rate pinned high, say).
- A route onto a knob moves every setting on that knob's path at once.

**Rolls and the matrix.** Rolling the LFO page (or REC + FX, or a double tap) rewrites the matrix, and mutation nudges
its depths. **SAVE + LFO** locks it so your routes survive both.

### LFO 1–6

![LFO](img/manual/mod-2.png)

| Knob | Setting | Range | What it does |
| --- | --- | --- | --- |
| K1 | **Rate** | 0.01–12 Hz | speed |
| K2 | **Shape** | Sine, Triangle, Saw, Square, Step random, Smooth random, Chaos, Rhythm, Shape 9 | Chaos is a logistic map (a new value each cycle); Rhythm a fixed 8-step pattern; **Shape 9** follows the output level |
| K3 | **Sync** | Free, Tempo | Tempo snaps the rate to the nearest of 1/8 … 4 cycles a beat (just ratios: 1/3, 2/3, 3/4, 4/5, 5/4, 4/3, 3/2…) |
| K4 | **LFO coupling** | 0–2 (shared by all six) | the LFOs pull on each other's phases (Kuramoto coupling): at higher values they drift into step |

---

## Effects (FX)

![Fuzz](img/manual/fx-1.png) ![Reverb](img/manual/fx-5.png)

The chain: **fuzz → filter → delay and reverb sends → compressor → master → limiter**.

| Tab | Knob | Setting | What it does |
| --- | --- | --- | --- |
| **Fuzz** | K1 | **Fuzz mix** | wet/dry (0 = bypassed) |
| | K2 | **Fuzz sustain** | drive (24–52 dB): a two-stage, Big-Muff-style fuzz on the whole mix. On a just scale its sum and difference tones land back on the harmonic series |
| | K3 | **Fuzz tone** | dark … scooped … bright |
| **Filter** | K1 | **Filter cutoff** | 60 Hz – 18 kHz |
| | K2 | **Filter resonance** | 0–0.95 |
| | K3 | **Filter type** | Low-pass → band-pass → high-pass, continuously |
| **Delay** | K1 | **Delay send** | |
| | K2 | **Delay time** | in ten tempo divisions: 0 % = 1/16, then 1/8 triplet, dotted 1/16, 1/8, 1/4 triplet, dotted 1/8, 1/4, 1/2 triplet, dotted 1/4, 100 % = 1/2 (at most 3 s) |
| | K3 | **Delay fb** | feedback (above 100 % it grows) |
| | K4 | **Delay tone** | the repeats' brightness (a tape-like, darkening delay) |
| **Delay 2** | K1 | **Delay tap** | a second tap at a fraction of the time: 1 of 5 = the same time, then 3/4, 2/3, 1/2, 4/5 |
| | K2 | **Delay into reverb** | how much of the delay feeds the reverb |
| **Reverb** | K1 | **Reverb mix** | |
| | K2 | **Reverb size** | room size |
| | K3 | **Reverb decay** | 0.3–60 s |
| | K4 | **Reverb damp** | how long the top of the tail rings against the rest: as long at 0 (a plate's), a quarter as long at full (a soft room's). The screen says how long ("Top 55 % as long"); on the Space knob |
| **Space** | K1 | **Predelay** | 0–100 ms |
| | K2 | **Reverb mod** | slow movement in the tail (chorus-like) |
| | K3 | **Reverb tuned** | On: the reverb's delay lines snap to periods of the scale's notes, so the tail rings in key |
| | K4 | **Reverb freeze** | On: the reverb holds what it has, indefinitely (also performance key 7) |
| **Compressor** | K1–K4 | **Threshold, Ratio, Attack, Release** | the bus compressor (−36–0 dB, 1–20:1, 0.5–80 ms, 20–800 ms) |
| **Output** | K1 | **Comp mix** | parallel compression: dry … fully compressed |
| | K2 | **Pump** | the compressor ducks on layer 1's onsets |
| | K3 | **Master** | the output level (0–150 %) |
| | K4 | **Space knob** | **Effects**: the third performance knob is Space; **Shape**: it is Shape (a Dry patch's). See [The performance knobs](#the-performance-knobs) |

After the master, a look-ahead **limiter** holds peaks at −0.5 dBFS, and a soft clip catches anything that still gets
past. Patches come out loud (about −14 dBFS on average) without distorting.

---

## Setup (HOME twice)

From the Perform page, press **HOME** again for Setup. **SAVE + OCT+** keeps these settings.

![General](img/manual/setup-1.png) ![Bluetooth audio](img/manual/setup-2.png) ![Load](img/manual/setup-3.png)

| Tab | Knob | Setting | What it does |
| --- | --- | --- | --- |
| **General** | K1 | **Color theme** | nine themes |
| | K2 | **Key octave** | −3 … +3 (also OCT − / OCT +) |
| | K3 | **MIDI channel** | Omni or 1–16, for MIDI in (and the knobs' CCs out, in Standard) |
| **Bluetooth audio** | K1–K4 | | see [Bluetooth audio](#bluetooth-audio) |
| **Load** | K1 | **Layer 1 voices** | 4–8 notes at once |
| | K2 | **Layer 2 voices** | 4–8 |
| | K3 | **Partials** | 4–16 oscillators a note |
| **Load cuts 2** | K1 | **Second core** | a status, not a control: **Layer 2 on core 2** (always, since 1.0.5), or **Given up: one core** if the second core ever failed (fm1-base then renders both layers on the first core until power-off, which crackles: power off and on) |
| | K2 | **Peak** | the worst block since reset; turn to reset |
| **Keys** | K1 | **Keys play** | Layer 1, Layer 2, Both layers, Drums |
| **MIDI out** | K1–K4 | | see [MIDI out](#midi-out) |
| **MIDI out notes** | K1–K3 | | |
| **MIDI out channels** | K1–K4 | | |
| **MIDI out drum notes** | K1–K4 | | |

### The load page

The Load tabs show a live profiler: the share of each audio block spent on **Control** (smoothing, LFOs, the matrix,
tables; "of it, gens" is the generators' part), **Layer 1**, **Layer 2** (on core 2) and how long core 1 waited for
it, **Drums + FX**, and the **Total**. **Peak** is the worst block, **Platform** the USB and Bluetooth work,
**Xruns** the blocks that ran out of time, **over 90 %** the blocks close to it, and **voices** the current voice
cap.

If the audio ever runs out of time, MANIFOLD drops a voice at once (a thinner chord, never a click) and gives voices
back one every 4 s while it keeps up. "OVER n" in the Perform page's header counts these moments.

---

## MIDI

MANIFOLD is a USB MIDI device, and a Bluetooth MIDI device.

**In** (on the MIDI channel set in Setup, or any channel at Omni):

| Message | Does |
| --- | --- |
| Note on / off | plays the scale note nearest the incoming pitch, as a key does (velocity included) |
| CC 1 | the Timbre knob |
| CC 21, 22, 23, 24 | the Timbre, Groove, Space and Motion knobs |
| CC 120, CC 123 | everything off |

MANIFOLD does not follow incoming MIDI clock.

### MIDI out

What MANIFOLD plays can go out over USB and Bluetooth MIDI: the layers' notes, the keys, the drums, the knobs and the
clock. **It sends only while something listens**: a computer (or another USB host) that has set up USB MIDI, or a
Bluetooth MIDI link. A USB charger or power bank does not count. With nothing listening, MIDI out is idle. When a
link arrives it is given the MPE set-up (in MPE), the knobs' CCs, a Start at the next bar (if MANIFOLD is playing)
and then the notes as they sound; when it goes, it is forgotten. (The TRS jack is MIDI in only: the FM-1 has no
output on it. For 5-pin DIN out, go through a USB host MIDI box.)

![MIDI out](img/manual/setup-midi.png)

| Tab | Knob | Setting | What it does |
| --- | --- | --- | --- |
| **MIDI out** | K1 | **Notes** | **Keys + layers** (the default), **Keys**, or **Off** |
| | K2 | **Tuning** | **Standard**: each note as its nearest equal-tempered pitch. **MPE (just)**: each note on its own channel with a pitch bend that puts it at its exact just pitch |
| | K3 | **Drums** | **Kit** (the default: General MIDI notes), **Pitched** (each hit as the pitch of the note it lands on), or **Off** |
| | K4 | **Knobs and clock** | **Knobs + clock** (the default), **Knobs**, **Clock**, or **Off** |
| **MIDI out notes** | K1 | **Note floor** | −12 … −36 dB (default −18): how far under the layer's loudest note a note must rise to start |
| | K2 | **Notes a second** | the most new notes a layer starts in any second: 6, 12, **24** (the default: 32nd notes at 180 BPM), 48, 96, or no cap |
| | K3 | **Level** | **Off** (the default) or **On**: each note's level as it moves, as channel pressure (MPE) or polyphonic aftertouch (Standard), 20 times a second |
| **MIDI out channels** | K1–K4 | **Layer 1, Layer 2, Keys, Drums** | Standard: 1, 2, 3 and 10 by default. In MPE the notes take channels 2–15 and the kit 16 |
| **MIDI out drum notes** | K1–K4 | **Kick, Snare, Hat, Tom** | the kit's notes: 36, 38, 42 and 45 by default |

**The layers' notes.** A layer's notes swell and fade without beginning or end, so MIDI out decides: a note starts
when it rises above the **Note floor** under the layer's peak (its loudest note, held and let down over about a
second and a half), with a velocity from its level (30 at the floor, 127 at the peak), and stops 12 dB under the
floor (so it does not flicker at the edge). A note lasts 30 ms at least. **Notes a second** caps how many new notes
a layer starts in any second; a note the cap holds back waits its turn, the loudest first, so fast motions come out
thinned rather than as a flood. Stopped (PLAY), the layers send nothing; the keys still do.

**MPE.** One zone: channel 1 is its master, channels 2–15 its 14 notes (shared by the layers, the keys and pitched
drums). Each note's pitch bend, sent just before its note-on, is set for a bend range of ±48 semitones, MPE's
standard (0.6 cents a step). MANIFOLD sends the MPE configuration message when a link arrives and when you choose MPE,
and most MPE instruments set themselves up from it; on one that does not, set its pitch-bend range to 48. With 14
notes sounding, a key, or a layer note louder than the quietest one, takes the quietest one's channel. The kit goes
on channel 16, outside the zone.

**The drums** go out when the hit sounds (timed to the audio within a millisecond), each as a 20 ms note.

**The knobs** you turn go out as CC 21–24 (Timbre, Groove, Space, Motion: the numbers MANIFOLD takes in), on the MIDI
channel (channel 1 at Omni, and in MPE), 30 times a second at most. A knob moved by a scene, a roll or incoming MIDI
is not sent, so a DAW playing back what it recorded does not fight itself. Nothing MANIFOLD receives is sent back.

**The clock:** 24 pulses a quarter note at MANIFOLD's tempo, Start when it starts (at the beginning of a bar) and
Stop when it stops. Swing is not in the clock. Over Bluetooth a computer receives MIDI in steps of about 7.5 ms, so
for tight sync use USB.

**No stuck notes.** Every note sent is kept until its note-off; a link takes a note-on only while its queue has room
for every note-off it may owe. Releasing a latched key (a second press, or Latch off) sends its note-off, and
**Panic** lets every note go and sends All Notes Off on each channel used. A roll, a patch load or a change of these
settings lets the notes go first.

---

## Bluetooth audio

MANIFOLD can **send** its audio to Bluetooth headphones and speakers. It cannot receive audio.

![Bluetooth audio](img/manual/setup-2.png)

1. Press **HOME** twice for Setup, then **SELECT** to *Bluetooth audio*.
2. **K1** turns Bluetooth on.
3. Put the speaker or headphones in pairing mode and press **REC** to scan (about 10 s).
4. **K2** picks the device.
5. Turn **K3** right to connect (left to hang up).
6. **K4** sets whether the last device is called again at power-on.

Four devices are remembered. While it streams, the FM-1's own output is muted and the header shows **BT**. Bluetooth
audio arrives 150–300 ms late (the receiver's buffer), so it suits listening more than playing in time.

The FM-1 sends SBC at 44.1 kHz, joint stereo. It works both with devices that wait for the source to set up the
stream and with those that set it up themselves after pairing (most speakers do; 1.0 refused those). If a device
will not connect, put it back in pairing mode, connect again, then open the web installer's **Read the crash report**
straight away: it lists each step of the Bluetooth set-up and whether the FM-1 or the device turned it down.

**A device that once paired with the FM-1** may keep that pairing when the FM-1 no longer has it (a set-up that
failed half way, or the device forgotten on the FM-1). Such a device can stop part way through pairing again; since
1.0.6 the FM-1 gives up after 8 s and the crash report says so. Remove the FM-1 from the device's paired devices (for
headphones and speakers, usually a reset of their pairings), then connect again. Since 1.0.6 the FM-1 also asks each
of a device's audio inputs in turn for SBC, so a device that offers AAC or its own codec first is no longer refused.

---

## USB audio

The USB cable carries audio both ways as well as MIDI, with no driver to install: the FM-1 is a class-compliant USB
audio device (UAC1) named **MANIFOLD FM-1**, for macOS, Windows and Linux, and most tablets and phones that take USB
audio.

**Record the FM-1 on a computer.** Connect it with a USB data cable and choose **MANIFOLD FM-1** as the input in your
recording software (or in the system's sound settings). It sends the FM-1's stereo output, 16-bit, at 44.1 kHz (the only
rate it offers: a session at 48 kHz is converted by the computer). What is recorded is
exactly what the speaker plays, after the effects and the limiter, at the **MASTER** knob's level. Turn MASTER most of
the way up while recording: with 16 bits, a quiet setting records a quiet, noisier signal. The recording never
contains the computer's own audio (below), so a computer playing back what it records makes no loop.

**Play the computer through the FM-1.** Choose **MANIFOLD FM-1** as the output too: the computer's audio is mixed
into the FM-1's speaker and headphones (44.1 kHz too), also at MASTER's level, and goes out over Bluetooth audio if that is
streaming. Use it to play along with a track, or to monitor a recording through the FM-1's headphones.

Notes:

- Both directions work alongside USB MIDI in and out, Bluetooth MIDI and Bluetooth audio. A USB charger or power bank
  carries no audio: it takes a computer (or another USB audio host).
- While Bluetooth audio streams, the FM-1's own speaker and headphones are muted (see [Bluetooth audio](#bluetooth-audio))
  but the USB recording continues.
- The USB recording is the lowest-latency way to hear the FM-1 on a computer; Bluetooth audio arrives 150–300 ms late.

---

## Recipes

**A slow ambient bed.** PRESET → Ambient. On Layer 1: Motion Drift or Gravity, Envelope Swell at 1 per 4 bars,
Contour low. Drums off (performance key 12). FX: Reverb decay 20 s, Reverb tuned On. Turn Space up.

**A pulse that plays the scale.** Layer 1: Motion Ladder (Span 8, up and down), Rate 1 a bar, Envelope Pulse at
2 a beat, Contour high. Layer 2: Route Multiplies, Envelope Stream: the run is gated by the rhythm.

**Drums from the melody.** Drums → Follow: set the hat to Layer 2, Place to point 1. The hat now fires when layer 2's
first point swells. Set layer 2's Route to Adds so it shapes layer 1 silently while it drives the hat.

**The melody makes the groove.** Drums → Lock & motion: Motion up. As layer 1 moves across the map, the rhythm gets
busier on one side and sparser on the other.

**A generator turns a knob.** LFO page, Matrix: Source Layer 2 motion, Destination Space knob, Depth +40 %. Layer 2's
swells now open the reverb and delay. Lock the matrix with SAVE + LFO.

**A house groove from scratch.** PRESET → House. Drums → Patterns: if the kick is not four on the floor, turn K1
to a neighbour that is. Pattern feel: Ghosts 0 for a clean groove, 30 % for MANIFOLD's own fills between. Mutes: the
tom (an open hat) mutes the hat.

**A boom-bap loop you can play over.** PRESET → Hip-hop. Layer 1 → Keys: Keys pull the points Off, so the keys play
exactly their notes over the beat. Turn Groove to move the patterns to busier or sparser relatives.

**Long-form evolution.** Drums → Rhythm: Form high. Drums → Lock & motion: Evolve around 30 %. Leave it running.

**Play along with other instruments.** Scale → Temper: Equal (12-tone). Root to match the song.

---

## Troubleshooting

**Nothing records over USB.** Choose MANIFOLD FM-1 as the recording input, check the cable carries data (charging-only
cables do not), and turn MASTER up: the recording follows it.

**No sound.** Press PLAY (the generators start stopped). Check Master (FX → Output) and the layer levels. Check the
performance-key mutes (hold PLAY: L1, L2, GEN on the Perform page should be lit). If BT shows in the header, the
sound is going to Bluetooth.

**A key does nothing.** Setup → Keys: is it set to Drums, or to a muted layer? Latch may be holding the key: press it
again, or performance key 14 (Panic).

**It stopped changing when I roll.** A page may be locked: LOCKED shows in its header. SAVE + its button unlocks it.

**"OVER" in the header.** The audio ran out of time and a voice was dropped. Lower Setup → Load (voices, partials) for
very dense patches.

**The scale did not change on a roll.** Only REC + SEL rolls the scale.

**A Bluetooth speaker says connected but plays nothing, or the screen says REFUSED.** Update to 1.0.2 or later (1.0
refused speakers that set up the stream themselves). Then see [Bluetooth audio](#bluetooth-audio) for reading the
set-up's log.

**Saved patches or settings are gone after power-off.** FM-1s sold since autumn 2026 carry a different flash chip
(UC25HQ80). MANIFOLD before 1.0.3 did not recognise it and kept everything in memory only. Install 1.0.3 or later.

**The battery gauge.** It reads the battery once at power-on and then every 10 s (in a slot of the key scan that no
key uses, so playing is never affected), averaged over about 40 s. On USB power it shows USB.

---

## For developers

MANIFOLD is open source (MPL-2.0) and builds on [fm1-base](https://github.com/charlesvestal/fm1-base). This part
describes the engine's mathematics and the code's organisation.

### Building

```
git clone https://github.com/stets43/Manifold
git clone --recursive https://github.com/charlesvestal/fm1-base
cd fm1-base
git checkout 2d8ed74 && (cd bsp && git checkout 773b0b9)
git apply ../Manifold/patches/fm1-base.patch
(cd bsp && git apply ../../Manifold/patches/felucca-bsp.patch)
python3 tools/build.py ../Manifold --host        # the host simulator: any Linux or macOS
../Manifold/build/host/manifold_host ../Manifold/tests/basic.txt ../Manifold/build/host
tools/get_toolchain.sh                           # JieLi's clang for pi32v2
export JIELI_TOOLCHAIN=~/.jieli/toolchain AC79_SDK=/path/to/fw-AC79_AIoT_SDK
python3 tools/build.py ../Manifold --release 1.0.7 # -> build/manifold-1.0.7.fwsc
```

MANIFOLD needs a few changes to fm1-base (a larger RAM code window for its control tick, the battery read in the key
scan's pot slot, Bluetooth audio's acceptor role, both flash chips, and the store's `APP_STORE_POOL_FOREIGN`); they are
in `patches/`, against the fm1-base and felucca-bsp commits above.

**The store.** The 64 patch slots and the Bluetooth record share fm1-base's pool (`APP_STORE_POOL`, app.h): 69
sectors from 0x97000, one copy per sector, the newest valid copy of an object winning, so a power cut during a save
leaves the previous copy. 1.0's 16 slot pairs sat at the pool's first 32 sectors and are read as pool copies as they
are; the Bluetooth record's pair at 0xDE000 moves in at the first boot. A slot's current copy that is not a MANIFOLD
patch (another firmware's record of the same format) is erased at boot. A patch is 3052 bytes.

The device build needs three files from JieLi's AC79 SDK (`cpu/wl82/tools/uboot.boot`, `cfg_tool.bin`,
`cfg/eq_cfg_hw.bin`; Apache-2.0). `tests/basic.txt` is the host test suite (144 checks: every page, rolls, locks,
the performance keys, scenes, undo, patches, MIDI, the matrix, the profiler). `hosttest/render.c` is a desktop
render harness (single patches, sweeps, a tour).

### Architecture

The audio runs at 44.1 kHz in blocks; everything that is not per-sample runs in a **control tick** every 64 samples
(1.45 ms): parameter smoothing, the LFOs, the matrix, the rhythm, both generators, the drums' envelopes and the
voices' spectra. Layer 2's voices run on the FM-1's second core while core 1 renders layer 1. The UI talks to the
engine only through a lock-free command queue drained at the start of each audio block.

| File | Contents |
| --- | --- |
| `app.c`, `app.h`, `fm1.toml` | the app on fm1-base: UI, pages, performance keys, scenes, patches, MIDI, Bluetooth audio |
| `dsp/gen.c` | the twelve motions, the eight envelopes, routes, keys, the drive |
| `dsp/rhythm.c` | the stream, events, drum choice, the patterns, mutes, the Lock engine |
| `dsp/patterns.c` | the pattern library and corpus entries (generated by `tools/patterns/train.py` from `tools/patterns/corpus.py`) |
| `dsp/drums.c` | drum triggers, envelopes and character (on the synth's voices) |
| `dsp/voice.c` | the oscillator banks and their spectral function |
| `dsp/engine.c` | the surfaces, the Tonnetz layout, tuning |
| `dsp/scales.c` | the scale library, tonal weights |
| `dsp/mod.c` | LFOs, the matrix, the knobs' paths |
| `dsp/fx.c` | fuzz, filter, delay, reverb, compressor, limiter |
| `dsp/params.c` | the parameter table, rolls and styles, the patterns' decision tree |
| `dsp/synth.c` | the top level: the block, the tick |

Parameters are stored normalised (0…1) in `target` (what is set), `base` (smoothed toward it) and `mod` (base plus
the matrix and the knobs' paths, recomputed each tick). Structural parameters (scale, rotation, root, octaves,
surface) rebuild the engine in the main loop. Code that runs in the audio interrupt is placed in RAM (`DSP_CTL`,
`DSP_INL`); the rest runs from flash (`DSP_FAR` calls). The audio path never allocates and never calls into flash.

### The drive: from points to loudness

Each layer has points *j* = 1…*n* at positions **q**ⱼ(*t*) with envelopes *e*ⱼ(*t*). Each note *i* sits at **p**ᵢ on
the surface with tonal weight *w*ᵢ. The note's drive is

```
u_i(t) = w_i · b_i · Σ_j  A · e_j(t) · g(t) · φ(|q_j(t) − p_i|)

φ(d) = (1 − (d/R)²)²  for d < R,  else 0          R = s · (0.6 + 1.2 · Reach)
```

where *s* is the layout's mean spacing between notes of different pitch classes, *A* = 0.35 + 0.65·Energy and
*g*(*t*) the stream's accent: g = (1 − γ) + γ · clamp(0.25 + 1.5 R̂(τ), 0, 1), γ = 0.7·Grip. φ is a smooth bump with
a continuous slope that is exactly zero beyond *R*, so the drive is C¹ in the points' positions. Distances are on the
surface: Euclidean on the plane, wrapped on the torus, chordal on the sphere. The keys add their own input:
*u*ᵢ += *k*ᵢ(*t*). Loudness is the drive through a 3 ms symmetric smoother.

Point *j*'s phase along the motion is *t*·Rate + Spread·*j*/*n*, with Rate snapped to the nearest of 1/64 … 4 cycles a
bar (whole-number ratios). Motions output (*x*, *y*) in [−0.45, 0.45]² and a third coordinate *h* ∈ [0, 1]. The
**path view** maps them onto the surface:

```
X = c_x + o_x + k · (x cos θ − y sin θ),    Y = c_y + o_y + k · (x sin θ + y cos θ),    k = (f / 0.45) · scale
```

with (*c*ₓ, *c*ᵧ) and *f* the centre and half-extent of the notes' frame (so scale 1 covers every note), θ the path
rotation and *o* the path offset. The plane folds at its edges, the torus wraps, and on the sphere crossing a pole
flips longitude by half a turn, so every path stays continuous.

When a motion's discrete choice changes, the point's offset from the new path decays with a 0.1 s time constant: the
position is always continuous.

### The motions

Below, *t* is the point's phase in cycles, sn(*z*) = sin 2π*z*, cs(*z*) = cos 2π*z*, and A, B ∈ [0, 1] the two controls.

**Orbit** (Lissajous). With ρ chosen from {1, 2, 3/2, 4/3, 5/4, 5/3, 3, 5/2} by A:

```
x = 0.45 sn(t),   y = 0.45 sn(ρt + B/4),   h = ½ + ½ sn(ρt/2)
```

**Ladder.** The notes sorted by pitch; a window of M = 3 + round(9A) notes, each point starting two notes higher.
The position along the run is u = φM (up), (1 − |2φ − 1|)(M − 1) (up and down) or (1 − φ)M (down), φ = t mod 1. Between
note *i* and *i*+1 the point rests and then moves: f = smoothstep((frac(u) − 0.55)/0.45).

**Swarm** (Kuramoto). Each point *j* has a phase θⱼ on a circle of radius 0.2 + 0.25 j/(n − 1):

```
dθ_j/dt = ω_j + K (Im(Z) cos 2πθ_j − Re(Z) sin 2πθ_j),   Z = (1/n) Σ_q e^{2πiθ_q}
K = 3A,   ω_j = 1 + B/2 · ξ_j  (ξ_j ∈ [−1, 1] fixed per point),   h = |Z|
```

Above a critical coupling the phases lock (the points gather into a chord); below it they scatter.

**Lorenz.** Integrated with Heun's method (RK2), steps ≤ 0.004:

```
ẋ = 10(y − x),   ẏ = x(ρ − z) − y,   ż = xy − (8/3)z,   ρ = 20 + 30A
position = (x, z − ρ + 1) / (35 + 30(1 − B)),   h = z / 1.6ρ
```

**Motif.** A small Lissajous figure of ratio *r* (from A) around a centre that moves each quarter cycle through
home, home, answer, home (A A B A). "Home" is the ding, the "answer" the next strongest note of the key (its fifth),
the third, or alternating (B). The centre glides over the last 15 % of each quarter.

**Drift** (value noise). Two octaves of hashed random knots joined by the quintic w = 6f⁵ − 15f⁴ + 10f³, at 1 + 3B
knots a cycle, the second octave three times denser at amplitude 0.6A:

```
x = 0.9/(1 + 0.6A) · Σ_o a_o [ (1 − w) N(n) + w N(n + 1) − ½ ]
```

**Billiard** (Sinai). A point moves in straight lines at constant speed in the box |x|, |y| ≤ 0.45, reflecting off
the walls and off a central circular bumper of radius 0.28B (mirroring the velocity in the bumper's tangent). The
heading turns with changes of A. Without the bumper the paths close into figures; with it the billiard is chaotic.

**Gravity.** Up to three wells at the key's strongest notes (the ding, then its fifth, then its third: W = 1…3 from
A), each of mass GM = 0.25/W, softened by ε² = 0.0064:

```
a = −Σ_k GM (q − w_k) / (|q − w_k|² + ε²)^{3/2}
```

integrated with symplectic Euler (steps ≤ 0.01). To keep the orbit bounded without damping it, the speed is eased
toward the value the target energy E = −GM·W·(0.8 + 3(1 − B)) asks for at the current potential:
v² → 2(E − Φ), by at most 10 % a step. h is the speed.

**Stride.** A heading θ and a speed *v* = R̂^{3/2}·(2 + 8B), with R̂ the rhythm's normalised stream: the point moves only
as the stream peaks, and turns while it rests: dθ = (A − ½)·3·(1 − R̂)·dt. Walls reflect.

**Loops.** Point *j* sits on its own note of the key (the strongest 2 + round(4A), each in its own octave) and swells
once per loop of length Lⱼ ∈ {5, 7, 8, 11, 13, 17, 19, 23} quarter cycles:

```
ψ_j = frac(4t / L_j + 0.37 j),   e_j = ½ − ½ cos(2π ψ_j / w)  for ψ_j < w,  w = 0.12 + 0.5B
```

The lengths are pairwise coprime, so the whole pattern returns only after their product.

**Erode.** A six-note figure (scale steps 0, 2, 4, 3, 5, 1 from a low note) repeated every cycle. Over a life of
4·2^{4A} cycles a wear level rises to 0.25 + 0.75B and then renews over the last tenth. Each note *i* fades when the
wear passes its threshold (0.95, 0.35, 0.6, 0.15, 0.8, 0.5), so the weakest notes go first.

**Chords.** Four chords a cycle from a progression (A): I IV V I, I VI IV V, I III IV II, I V VI IV, I I IV I,
I VII VI V. Point *j* holds the chord's root + *j* × 2, 3 or 4 scale steps (B: thirds, fourths, fifths), from an octave
and a half above the ding, gliding to the next chord over the last 20 % of each quarter.

### The envelopes

Each point's envelope eⱼ(*t*) ∈ [0, 1] (Adds may reach 1.2), with Contour C ∈ [0, 1]:

| Envelope | Formula |
| --- | --- |
| Pulse | e = sin(π ψ′)^p,  ψ′ = ψ / (ψ + k(1 − ψ)),  p = 1 + 7C²,  k = 1 − 0.88C |
| Swell | e = (½ − ½ cos 2πψ)^{1 + 2C} |
| Speed | e = ½ + c·(S((v/v̄ − 1)(0.8 + 2.5C) + ½) − ½), v̄ the 2 s running mean speed |
| Pass | e = (exp(−4 d²/s²))^{0.6 + 1.6C}, d the distance to the nearest note |
| Height | e = ½ + c·(S((h − h̄)/σ_h · (0.35 + 0.6C) + ½) − ½) |
| Hold | e = 1 |
| Stream | e = S((R̂(τ − Spread/rate) − (0.04 + 0.5C)) / 0.16) |
| Drums | e = (drum envelope)^{0.4 + 2C} |

S is smoothstep; *c* ramps from 0 to 1 over the first second (the running averages need data). For Pulse and Swell,
ψ is the envelope's phase at Envelope rate ν (cycles a beat, times 4 beats a bar), plus the point's Spread offset. It
is phase-locked to the bar: dψ/dt = ν/bar + 2·err, err the wrapped difference from ν·(bar position), so after a tempo
change it eases back into step within about half a second.

Routes, with depth d = the shaping layer's Energy and *m* its point *j* mod *n*:
Moves **q**ⱼ += d·**q**ₘ; Adds eⱼ += d·eₘ; Multiplies eⱼ ← eⱼ·(1 − d + d·eₘ).

**Keys.** A key press is a kernel in time: a smoothstep rise over *a* = 2 ms + 1.2(1 − C)³ s, then an exponential
fall with time constant 0.12 + 3(1 − C)² s; a held key adds a sustained level (0.25 + 0.65(1 − C))·velocity that
breathes with the layer's envelope while it runs. Each key also pulls every point of its layer toward its note: the
offset approaches Σ kᵢ Δᵢ / (1 + Σ kᵢ) with a 0.15 s time constant (Δᵢ the way to note *i* on the surface).

### The rhythm

The stream is a sum of von Mises bumps over the bar position τ ∈ [0, 1):

```
R(τ) = (1/W) Σ_k w_k · b(n_k τ − φ_k),     b(z) = exp(5 (cos 2πz − 1)),    W = Σ w_k
```

with harmonics nₖ = 1, 2, 4 (always), 8, 16, 32 (the twos), 3, 6, 12, 24, 18 (the threes) and an off-beat at 4. Threes
crossfades the two families; Fine opens 8/3, 16/6/12, 32/24 in turn; Swing moves the off-beat's phase from ½ through
⅔ (triplet swing) to ¾ while the straight 8, 16, 32 give way to 12 and 24; Turn shifts the threes by up to ⅓ bar.
Form adds slow cosines (periods 3, 4, 5, 8, 16, 64 bars) to the threshold, the sixteenths' weight, the three-feel
and the selector.

**Events** (Threshold engine) are local maxima of R above the threshold θ = 0.03 + 0.6(1 − Density)², refined by a
parabola through three ticks so they land on the right sample. An event's strength is (peak − θ)/(1 − θ).

**Select** scores each drum *d* for an event of strength *s*:
score = 1 − 1.6|s − (1 − Place)| + polarity·selector + (density − ½)·0.8 − Feedback·tired, with the kick taking the
strongest events (s > 0.8) as an anchor. **Count** distributes events by their order.

**Lock**: four phase oscillators θ_d with rates r_d (kick 2, snare 2 offset ½, hat 8, tom 3 a bar, varying with
Density, Fine and Threes) and small detunings δ_d:

```
dθ_d/dx = r_d (1 + δ_d) + K r_d sin 2π(r_d x − θ_d),    K = 0.4 Pull
```

Above its detuning a drum locks to its exact rate; below, it slips and catches. Each hit nudges the others.

### The patterns

**The corpus** (`tools/patterns/corpus.py`, and `families.py` since 1.0.8) holds 678 kit patterns: 132 house and
techno, 72 breakbeat, 28 drum'n'bass, 114 hip-hop, 108 funk, 119 Latin and 105 indie. The first 102 are written out
one by one; the rest come from 45 **families** (an era or sub-style each: James Brown funk, son montuno, motorik…),
each listing, for every part, the lines that style's players reach for, with how typical each is. Any line of a part
goes with any line of the others, so `expand` draws 7–20 distinct combinations from each (a fixed seed per family),
and a family's weight is shared among them: families, not entries, are weighted, so a genre's styles come up equally.
Each entry is four lines (kick, snare, hat, a fourth part, or none: that drum stays linear) of one
or two bars of 16ths, hits and ghosts, with a tempo range, a swing range, the level the swing works at and a weight.
They are skeletons in the manner of each genre and of the well-known breaks, not transcriptions.

**Training** (`tools/patterns/train.py`, run whenever the corpus changes) makes each role's library: its distinct
lines (a two-bar line whose bars are equal is one bar), ordered by busyness (hits + 0.4 ghosts a bar, in steps of half
a hit) and, within a step, as a chain of nearest neighbours by the Hamming distance over two bars (a ghost counting
half). It writes `dsp/patterns.c`: the lines as hit and ghost masks, and every corpus entry as indices into them.

**The decision tree** (params.c `roll_patterns`): a root entry of the genre by weight; then for each role *r* in
order, an entry *e* drawn with probability ∝ wₑ · exp(−Dₑ/4), Dₑ the summed distance of *e*'s earlier roles to the
ones already chosen (an entry of the root's family 25 times likelier, so a roll mostly keeps to one style), and
*e*'s line taken (with probability 0.15–0.2 a close variation instead: another line of the
genre's corpus within three hits, nearer likelier). This samples P(kick), P(snare | kick), P(hat | kick, snare),
P(fourth | kick, snare, hat) as the corpus implies them, smoothed so unseen but rule-keeping combinations appear.
Tempo comes from the root (Breakbeat outside drum'n'bass: 124–140), swing and its level from the hats' entry. Mutes,
rotations, loop lengths, ghosts and fills are rolled per genre.

**Playing them** (rhythm.c `pat_tick`, each control tick): step *n* of a bar sits at its 16th, bent by Swing: within
each pair (two 16ths, or two 8ths) the second moves to *f* = ½ + Swing/4 of the pair. A step whose place falls in the
tick's window (1.5 ticks back, as the stream's events land) is looked up at index (16·bar + *n* − Rotate) mod Length,
bit modulo the line's length. It sounds if R₀ ≥ θ/2 (a ghost: R₀ ≥ θ), R₀ the stream without swing at the step's
straight place, normalised: so Density, Fine and Form thin and vary a pattern as they do the stream. Its velocity is
(1 − 0.55·Accent + 0.55·Accent·s)·(0.42 for a ghost), s = (R₀ − gate)/(1 − gate). Every hit of the tick, the patterns'
and the linear layer's, is collected; then in the order kick, snare, tom, hat each drum that plays removes its Mute
target's hits (or every linear hit) of that tick; then the rest are queued to the drums.

### The drums

A drum's envelope after a hit: a linear rise from its current level over Attack, a hold, then a decay over Decay,

```
env(t) = ex + Curve · (lin − ex),   ex = 2^{−5.77 t/Decay}  (−35 dB at Decay),   lin = max(0, 1 − t/Decay)
```

so Curve blends exponential and straight. It is recomputed each tick from the current
parameters, so modulation reshapes a hit that is still ringing. The hit rides a sounding note on the drum layer
(Voice picks lowest … highest), raising that note's level by the envelope (its own gain, independent of the layer's
level staging) and, at full strength while the hit is within 18 dB of its peak (then fading with it; before 1.0.5,
in proportion to the hit's share of the note's level), tilting the partials (Bright, ±3 dB an octave at the note's energy, the fundamental at most 6 dB down; Dark brings in
the undertones,
settling over 300 … 9 ms) and pushing Wave (Snap, past Wave's own top, over 71 … 25 … 9 ms as Settle goes 0 … ½ … 1).
The kick's duck multiplies the other voices by 1 − 0.75·Duck·env², released over 150 ms. No drum ever changes a frequency.

### The voices

Each note *f* is a bank of oscillators at *k·f*, *k* = 1…16 (and up to 64 for low notes, from shared tail tables).
The level of partial *k* is one function, normalised to unit energy:

```
a_k ∝ k^{−1.6 + 1.2·Tilt} · (1 + g · exp(−(k − c)² / 2b²)) · comb(k) · tone(k f) · bloom(k)
c = 2 + 10·Center^1.6,   b = 0.5 + 7·Band,   g = 2·clamp((c − 2)/3, 0, 1)   (a lift of up to +9.5 dB)
comb(k) = (½ + ½ cos(2π (k − 1) / stride))²,   stride = 1 + 3·Stride
tone(F) = 1 / (1 + (F / f_c)⁴)  (−24 dB/oct),   f_c = 250 · 2^{6·Tone} Hz
bloom(k) = (2^{−0.36·Bloom·(1 − loudness)})^{k − 1}
```

A note has 16 oscillators: its harmonics 1–13 (k above, up to 13), then its undertones an octave, two and three
below (½, ¼, ⅛ of its pitch: octaves, so always in tune), at 1.2·D, 1.2·D², 1.2·D³ of the fundamental before the
whole is set to unit energy (D = Depth). Every partial is a whole multiple of one phase per note that runs at an eighth
of its pitch, so the undertones stay locked to the note. Nothing low is cut; no partial under 20 Hz is ever played.
Clarity then limits, for all of a layer's notes together, how many partials each octave (from 20 Hz: 20–40, 40–80,
…) may hold: 1, 2, 3, 5, 8, 13, 21, … at full, 16 times that at half (1 + 15·(1 − Clarity)²), no limit at 0. Every
note's fundamental plays; from the lowest note up, each note's undertones and harmonics, nearest first, take the
room that is left. A note's place does not depend on whether it sounds, so nothing comes and goes as notes do: the low
notes keep the low octaves. (It lightens the load too: rolled, about a fifth fewer oscillators.) Partials 7, 11 and
13, which belong to no scale but the harmonic series, are held back. Phases follow φₖ = π·D·k², D = Phase/8
(Schroeder's low-crest-factor phases at D = 1/16, Phase = 50 %); the undertones start in phase with the note. Wave
adds each harmonic's own 2nd and 3rd harmonics (0.5 sin 2x + 0.33 sin 3x; not the undertones': a 3rd of one is out of tune),
under Tone where they sound (at 2.5 times the partial's frequency), not where the partial does;
Detune doubles partials 1–3 with copies up to 14 cents away. No partial above 0.45·*f*ₛ is played.

**The oscillators** (1.0.7). A plain partial, a Detune copy, an undertone and the fundamental with Wave are each an
exact recursive sine, the magic circle x ← x − e·y, y ← y + e·x with e = 2 sin(ω/2): two multiplies and two adds a
sample, no table, its amplitude constant to rounding. It is seeded from the note's phase (x = sin(φ − ω), y = −cos(φ −
ω/2)) when a voice starts, when a partial returns, when Phase moves, when the pitch changes, and every 64 blocks
otherwise (staggered across voices), so the phase law and the undertones' lock hold. Wave on the fundamental comes from
the same state: c₂ sin 2θ + c₃ sin 3θ = x·(K₃x² + K₁x + K₂y + K₀). Wave on partials 2–13 reads a 4096-entry table, and
the two bass tails (harmonics 14–32 and 33–64) read theirs with interpolation. Each oscillator adds straight into its
layer's stereo with its own level line through the block: a ramp, or, while a drum hit sounds or the level rises
fast, a step, as before, so the hits keep their attack; a hit landing inside the block bends the line where it lands.
Against a double-precision copy of every oscillator, the error is 70–110 dB under the voices (it was 46–59 dB with
tables throughout, a buzz on low notes). A note's Tone weights and each oscillator's e are kept until its pitch or
Tone moves.

The oscillator budget is 64 per layer. When a layer needs more, a floor rises (from −44 dB relative to each note) so
the quietest partials fade first, over a one-octave knee; whole notes are never cut.

### The map

Each scale degree *n*/*d* is factored into exponents (e₃, e₅, e₇) of 3, 5 and 7 and placed at

```
x = e₃ + 0.5 e₅ + 0.45 e₇,     y = 0.866 e₅ − 0.78 e₇
```

(fifths along *x*, major thirds at 60°, natural sevenths at −60°; 11, 13 and higher primes of the harmonic series each
get a direction of their own). Each octave of a degree is offset by a small step at the golden angle, so registers
spread without breaking the lattice. The plane scales the lattice into [0.1, 0.9]²; the torus wraps it (fifths one
way, thirds the other); the sphere uses a Fibonacci layout with low pitches to the south.

*b*ᵢ is Balance's gain. Over long runs each motion gives a note a typical share of the energy by where it sits: its
distance from the motion's frame centre (five rings) and its place among the notes in pitch (four quarters). Those
shares, trained offline per motion from rolls of every curated style (`tools/balance/train.c`, a fixed table in
flash), are divided out: *b*ᵢ = √(10^(*t*ᵢ/10) / *E*ᵢ), with *E*ᵢ the trained share and *t*ᵢ the target in dB: 0
(Balanced); Dark −*B* · 4 dB an octave from the middle of the range; Bright *B* · (3 − 12 *z*²) dB with *z* the
distance from 55 % of the range in units of 45 % (clamped ±9 dB). The gains are then scaled to keep the layer's energy
and clamped to 0.35–2.8; they are worked out again only when the motion, the notes or Balance (in steps of 1 %) change.

A note's tonal weight is its harmonicity to the ding (Tenney height: 1.15 − 0.1 log₂(*nd*), from 1 down to 0.45),
as 0.7 + 0.3 of it. (Before 1.0.5 a bass-voicing factor also turned the low notes over the ding down to an eighth;
with it the lowest octave played at 40 % of its share. Clarity keeps the low octaves clear instead.)

### Effects

Fuzz: two cascaded gain stages into slightly asymmetric soft clippers with band-limiting between them, and a tone
blend from a 500 Hz low-pass to a 1.4 kHz high-pass. Filter: one resonant state-variable filter, its low, band and
high outputs crossfaded by Filter type. Delay: a bucket-brigade-style line whose write rate sets the time. Reverb: an
8-line feedback delay network at half the sample rate (mutually prime lengths of 34–99 ms scaled by Size, four of them
modulated), with allpass diffusion; its input through a 4-pole 9 kHz low-pass first (so nothing above 12 kHz folds
back into the tail), its lines' damping split at 1.5 kHz (the top's decay 1 − 0.75·Damp of the rest's), its lines
(and the delay's) 16-bit floating point (1.0.7: a 5-bit exponent and 10-bit mantissa packed into an int16 by one FPU
multiply, so the rounding falls with the tail, about 66 dB under each value, instead of a fixed −71 dBFS hiss); Tuned snaps each line to a whole number of periods of a scale note. Compressor:
a bus compressor with parallel mix; Pump ducks it on layer 1's onsets. Limiter: peak detection over eight 8-sample segments 64 samples
ahead, a 16-sample attack and ~280 ms release, ceiling −0.5 dBFS; then a soft clip.

### Licence and credits

MANIFOLD is MPL-2.0. It is built on fm1-base by Charles Vestal (MPL-2.0), which includes code by Leo Kuroshita
(Felucca), Chris Farrell and others; see fm1-base's LICENSING.md. A firmware built with fm1-base's own Bluetooth stack
also contains Apache-2.0 and BSD-2-Clause code (an SBC encoder, micro-ecc, JieLi radio tables): their licence texts
ship with the firmware.
