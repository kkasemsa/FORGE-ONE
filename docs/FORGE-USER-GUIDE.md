# FORGE User Guide

FORGE 0.3 for Felucca-FORGE 1.6.1 · M-VAVE FM-1

FORGE writes a musical idea onto a melodic track in one key press, then lets that idea evolve. It lives in
Felucca's SEQ TOOLS layer on the **F5** key. Every pattern comes from a **seed**: the same seed and settings
always give the same steps, so you can note a seed you like and get the pattern back later.

All values in this guide are starting points. Judge the result by ear and turn the knobs until it sits right.

## 1. Quick start

1. Choose a melodic track with **ALGORITHM** (any engine except DRUM).
2. Set the track's **ROOT** and **SCALE** on the SCL page, and its length with SEQ TOOLS' **K1 LEN**.
   FORGE reads all three, so set them first.
3. Press **PLAY** if you want to hear the result as you work.
4. Hold **SEQ** and tap **F5**. A pattern is written at once, and the screen shows it as a grid of steps.
5. Turn **K1 SEED** until you hear an idea you like. Then shape it with **K2 DENSITY**, **K3 EVOLVE** and **K4 STYLE**.
6. Press **OCT+** to keep it, or **OCT−** to put the old steps back.

If you change your mind after keeping a pattern, hold **SAVE** to undo the whole FORGE in one step, as long as you
have not used another SEQ TOOLS key since.

## 2. Controls while FORGE is open

| Control | What it does | Range |
| --- | --- | --- |
| **K1 SEED** | Picks the pattern. Each seed is a different idea; turning past 99 wraps to 0 | 0–99 |
| **K2 DENSITY** | How many of the style's steps sound; above 75 % it also adds notes off the style's rhythm | 10–100 %, steps of 5 |
| **K3 EVOLVE** | How much the idea changes across the pattern and from loop to loop (section 4) | 0–4 |
| **K4 STYLE** | The rhythm and pitch profile (section 3). Choosing a style sets DENSITY to that style's default | 8 styles |
| **OCT+** | Keep the pattern and close FORGE | |
| **OCT−** | Cancel: the track's steps go back to how they were before FORGE opened | |
| **F5** again | Keep the pattern (same as OCT+) | |
| Another SEQ TOOLS key | Keeps the FORGE pattern, then runs that tool | |
| Letting go of **SEQ** | Keeps the pattern | |
| **SAVE** held (afterwards) | Undo the whole FORGE: the steps and their locks come back (if no other tool ran since) | |

Turning the knobs while the sequencer plays is safe: each step is replaced whole, so you never hear half-written
steps. Each knob turn rewrites the pattern from the steps as they were before FORGE opened, so turning a knob back
returns the earlier pattern exactly. The exception is STYLE: any turn of K4 sets DENSITY to the style's default,
so set DENSITY after choosing the style.

**What the screen shows.** Each time FORGE opens or a knob turns, the header briefly shows `FORGE <seed> <style>`,
for example `FORGE 42 BERLIN`. The four knob
cards show SEED, DENSITY, EVOLVE and STYLE. Below them, every step of the pattern is a cell, 16 to a row:

- a **bright cell** is a note, with a small mark at its pitch (higher mark, higher note)
- a **half-bright cell** is a tie, holding the note before it
- a **dark cell** is a rest

The footer shows **OCT+ APPLY** and **OCT− CANCEL**. With MENU > SYSTEM > HELP on, a second row shows
K1 SEED, K3 EVOLVE and K4 STYLE.

**F5 lights up** while FORGE is open.

## 3. The eight styles

| Style | Knob card | Motif | Rhythm | Pitch | Octave | Default DENSITY |
| --- | --- | --- | --- | --- | --- | --- |
| BERLIN | BERLIN | 8 steps | 16th ostinato, accents on 1 and 9 | Small steps around the root | C3 | 85 % |
| DETROIT | DETROIT | 16 | Syncopated stabs | 4-note scale chords | C4 | 70 % |
| DUB | DUB | 16 | Off-beats, long ties | 3-note scale chords, narrow range | C3 | 55 % |
| MELODIC | MELO | 16 | 8ths | Rises then falls in an arc | C4 | 75 % |
| ACID | ACID | 16 | 16ths with accents and slides | Root pedal, other notes nearby, octave jumps | C2 | 75 % |
| TRANCE | TRANCE | 8 | Rolling 16ths | Arpeggios of the scale's triad and octave | C4 | 90 % |
| SYNTH | SYNTH | 8 | 8ths | Root, fifth and octave, a third now and then | C3 | 90 % |
| AMBIENT | AMBI | 16 | Very sparse, long ties | Wide intervals, some 3-note chords | C4 | 50 % |

- **Motif** is the length of the idea before it repeats. If LEN is shorter than the motif, the motif is LEN steps long.
- **Octave** is where the root sits: the root note is the track's ROOT in that octave (A minor on ACID starts on A2).
  Melodies move a few scale steps above and below the root (AMBIENT more than an octave). Octave jumps only go up.
- **Step 1 is always the root**, so every pattern starts at home.
- **The scale decides the notes.** Every note is in the track's ROOT and SCALE. Chords, and the TRANCE and SYNTH
  arpeggios, are built from real intervals (a third, fifth and seventh above the note) kept inside the scale, so
  they work in every scale. In CHR the chords are minor (root, minor third, fifth, minor seventh); in the
  pentatonic scales the "seventh" is the scale's sixth; in WHOLE the chords are augmented.
- **The knob card** shortens MELODIC and AMBIENT so they fit. The header shows the full names.

**Picking a sound for each style.** These are starting points:

| Style | Try it on |
| --- | --- |
| BERLIN | ANALOG or PHASE with a short decay; a slow LFO on the filter |
| DETROIT | WHEEL, FM6 or ANALOG pads with a short envelope; a 1/8D delay |
| DUB | ANALOG or FM6 chords with HALL reverb and a dotted delay; ECHO THROW in the FX layer |
| MELODIC | FM6, PHASE or TRIO leads |
| ACID | ANALOG with resonance up, MONO or LEGATO voice mode so the slides glide |
| TRANCE | ANALOG or TRIO with the gate short; the SLICER for movement |
| SYNTH | ANALOG or LOFI bass |
| AMBIENT | GRAIN, PHYS or FM6 pads with long release and HALL reverb |

Chords need a polyphonic track: set the voice mode to **POLY** on EDIT > VOICE (K1). On MONO, LEG or UNI only one
note of each chord sounds.

## 4. EVOLVE: how the idea changes

FORGE writes the motif once, then copies it through the track's LEN. EVOLVE decides how much the copies differ
and how much each loop differs from the last.

| EVOLVE | What happens |
| --- | --- |
| **0** | A pure loop. Every copy is the motif, with no chance, conditions or ratchets |
| **1** | Later copies sometimes sit a scale step higher or lower, and a few notes move a step. A few ornament notes get a 75 % or 50 % chance. ACID and TRANCE ratchet a few notes |
| **2** | More drift. Some ornament notes get a **condition** instead of a chance: 2:2, 3:4, 2:4 or 4:4 |
| **3** | As 2, and later copies also drop a note or add one in a rest |
| **4** | The most change: drift, rhythm changes and ornaments on more notes |

- **The first copy keeps the motif's notes and rhythm.** Drift and note changes only happen in later copies, and
  build up the further along they are. The first copy can still get chances, conditions and (ACID, TRANCE) ratchets.
- **If LEN equals the motif length** (a 16-step style on LEN 16), there is only one copy, so EVOLVE only adds
  chances, conditions and ratchets. Use a longer LEN, or an 8-step style, to hear the copies drift.
- **Conditions** make one pattern change over several loops: a 4:4 note plays only on every fourth loop, and 2:2 only
  on every second. Each track counts its own loops from PLAY and from the start of each SONG section.
- **Chance** notes play on some loops and not others, so the line breathes.
- You can see and edit the chances and conditions FORGE wrote on **SEQ > DETAIL** (K1).

**For long-form playing**, try a short motif on a long LEN: for example BERLIN (8 steps) on LEN 64 at EVOLVE 2.
The ostinato stays recognisable while the copies and conditions keep it moving.

## 5. Recipes

These are starting points. Each one lists the track settings to make first, then the FORGE settings.

| Idea | Track settings | FORGE settings |
| --- | --- | --- |
| Berlin School sequencer | ANALOG, LEN 32, DIV 1/16, A MIN | BERLIN, DENSITY 85, EVOLVE 2 |
| Dub techno chord stab | FM6 or ANALOG, POLY, LEN 16, D MIN, HALL reverb | DUB, DENSITY 55, EVOLVE 1 |
| Rolling acid line | ANALOG, MONO, LEN 16, resonance up, E MIN | ACID, DENSITY 75, EVOLVE 1, then turn SEED |
| Melodic techno lead | FM6 or PHASE, LEN 32, F# MIN | MELODIC, DENSITY 70, EVOLVE 2 |
| Trance arpeggio | ANALOG or TRIO, LEN 16, gate short | TRANCE, DENSITY 90, EVOLVE 0 |
| Ambient drift | GRAIN or PHYS, POLY, LEN 64, DIV 1/8, C MAJ | AMBIENT, DENSITY 50, EVOLVE 3 |

**Working with a pattern you like.** Note its seed and style. After you apply it, the steps are ordinary Felucca steps:
edit single notes on SEQ > STEP, add parameter locks, or use the other SEQ TOOLS (REVERSE, SHIFT, COOK) on top.

## 6. What FORGE writes, and what it leaves alone

**Writes:** steps 1 to LEN of the selected track. Each step gets notes (1, 3 or 4), NOTE, TIE or REST, accent,
slide, ratchet (×2 or ×3), and a chance or condition. Every note has velocity 96.

**Leaves alone:**
- steps past LEN
- the track's LEN, DIV, SWING and GATE, its sound, and its automation
- the other three tracks

**Parameter locks:** when you apply a pattern, steps that held something before and are now empty lose their locks,
as with EUCLID. Undo (SAVE held) brings them back.

**Compatibility:** FORGE only uses fields that Felucca 1.6 already stores, so projects made with FORGE open on stock
Felucca 1.6 and 1.6.1. Projects with conditional steps do not open on Felucca 1.5.1 or older. FORGE at EVOLVE 2 and
up can write conditions, so check SEQ > DETAIL or AUTOMATION before going back.

**Seeds and settings:** the same SEED, STYLE, DENSITY, EVOLVE, LEN, ROOT and SCALE always give the same steps.
From FORGE 0.3 on, a set of seeds is pinned by a test, so a later version only gives a seed a different pattern when
its changelog says so. Patterns you want to keep for certain belong in a project.
Change any of them and the pattern changes. At EVOLVE 0, changing DENSITY keeps the pitch of the notes that stay;
with EVOLVE 1 and up it can also change how the later copies drift.

## 7. When it does not do what you expect

| What you see | Why, and what to do |
| --- | --- |
| F5 does nothing | The track is a DRUM track (FORGE is for melodic tracks only), or a SONG is playing ("STOP TO EDIT") |
| The notes are too low or too high | Each style has its own octave. To move a pattern, set **QNT** to **SEQ+T** on the SCL page and turn **TRN** (TRN alone, and the octave buttons, only move the keys). Or try a style in another octave |
| Chords play as single notes | The voice mode is MONO, LEG or UNI. Set **POLY** on EDIT > VOICE |
| ACID slides do not glide | Use MONO or LEG voice mode, and some GLIDE on EDIT > VOICE (K2) |
| The pattern changes every loop and I want it fixed | Set EVOLVE to 0, or remove chances and conditions on SEQ > DETAIL |
| The knobs edit LEN, not SEED | FORGE is not open (F5 is not lit). Tap F5 while holding SEQ |
| I applied it and want the old steps back | Hold **SAVE** before using another tool (one undo). Or press **OCT−** before letting go of SEQ: it puts back everything the layer changed since it opened, LEN, DIV, SWING and GATE included |

## 8. Credits

FORGE is part of Felucca-FORGE, a fork of [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita, Hügelton
Instruments, and contributors. GPL-3.0-only. Source: [github.com/kkasemsa/Felucca-FORGE](https://github.com/kkasemsa/Felucca-FORGE),
`forge` branch; FORGE's code is in `firmware/src/ui_forge.c`.
