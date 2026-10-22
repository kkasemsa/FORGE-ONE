# FORGE User Guide

FORGE 0.7 in FORGE ONE (Felucca 1.6.1) · M-VAVE FM-1

FORGE writes a musical idea onto a track in one key press, then lets that idea evolve: a melody on a synth track,
a beat on a DRUM track (section 6). From 0.6, **STARTER** on the G5 key writes all four tracks at once, sounds
included (section 7). FORGE lives in
Felucca's SEQ TOOLS layer on the **F5** key. Every pattern comes from a **seed**: the same seed and settings
always give the same steps, so you can note a seed you like and get the pattern back later.

All values in this guide are starting points. Judge the result by ear and turn the knobs until it sits right.

## 1. Quick start

1. Choose a track with **ALGORITHM**. This quick start is for a synth track; a DRUM track makes a beat instead (section 6).
2. Set the track's **ROOT** and **SCALE** on the SCL page, and its length with SEQ TOOLS' **K1 LEN**.
   FORGE reads all three, so set them first.
3. Press **PLAY** if you want to hear the result as you work.
4. Hold **SEQ** and tap **F5**. A pattern is written at once, and the screen shows it as a grid of steps.
5. Turn **K1 SEED** until you hear an idea you like. Then shape it with **K2 DENSITY**, **K3 EVOLVE** and **K4 STYLE**.
   Like the rhythm but not the notes? Press **C4** for new notes. Like the notes but not the rhythm? Press **D4**.
6. Press **OCT+** to keep it, or **OCT−** to put the old steps back.

If you change your mind after keeping a pattern, hold **SAVE** to undo the whole FORGE in one step, as long as you
have not used another SEQ TOOLS key since.

## 2. Controls while FORGE is open

| Control | What it does | Range |
| --- | --- | --- |
| **K1 SEED** | Picks the pattern. Each seed is a different idea; turning past 99 wraps to 0 | 0–99 |
| **K2 DENSITY** | How many of the style's steps sound; above 75 % it also adds notes off the style's rhythm | 10–100 %, steps of 5 |
| **K3 EVOLVE** | How much the idea changes across the pattern and from loop to loop (section 4) | 0–4 |
| **K4 STYLE** | The rhythm and pitch profile (section 3). Choosing a style sets DENSITY to that style's default; turning past AMBIENT wraps to BERLIN | 8 styles |
| **C4** (RANDOM) | New notes on the same rhythm. Each press is the next variation | NOTES 0–99 |
| **D4** (COOK) | A new rhythm; each step that still sounds keeps its pitch (with EVOLVE, later copies may drift differently) | RHYTHM 0–99 |
| **OCT+** | Keep the pattern and close FORGE | |
| **OCT−** | Cancel: the track's steps go back to how they were before FORGE opened | |
| **F5** again | Keep the pattern (same as OCT+) | |
| Another SEQ TOOLS key (not C4 or D4) | Keeps the FORGE pattern, then runs that tool | |
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

Under the cells, **C4 NOTES** and **D4 RHYTHM** show how many times you have rerolled each (lit once above 0).
Turning K1 SEED or K4 STYLE sets both back to 0.

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

**Working with a pattern you like.** Note its seed, style and the C4 / D4 numbers. After you apply it, the steps are ordinary Felucca steps:
edit single notes on SEQ > STEP, add parameter locks, or use the other SEQ TOOLS (REVERSE, SHIFT, COOK) on top.

## 6. FORGE on a drum track

From 0.5, F5 works on a DRUM track too. The same four knobs and the same eight style names make a **beat** instead
of a melody. The map shows FORGE as a small **F** cell at the right end of the lanes row.

| Control | On a DRUM track |
| --- | --- |
| **K1 SEED** | Picks the beat |
| **K2 DENSITY** | How many hits. Each style's main hits stay from 50 % up; above 75 % extra 16th hats and rim ghosts come in |
| **K3 EVOLVE** | 0 is a pure loop. From 1, later bars move a hat or two and add ghost snares and rims with a chance; from 2 the last bar ends in a **snare roll fill on a 4:4 condition** (every 4th loop); from 3 kicks off the beat come and go |
| **K4 STYLE** | The beat (table below). Sets DENSITY to the beat's default |
| **C4** (LOW) | New kick, snare, clap and tom; the hats stay |
| **D4** (HATS) | New hats, rim and bell; the low lanes stay |

The screen shows every step as a column of eight strips, KICK at the bottom and BELL at the top; a brighter strip is an
accent. At LEN 33 to 64 the strips would be too thin, so each step shows two columns of marks instead: kick, snare,
clap and tom on the left, hats, rim and bell on the right. The kick on step 1 is always there, and kicks on 1 and 9 are accented; snares and claps on 2 and 4 are
accented half the time. Velocity follows the beat as on melodic tracks. Lane mutes (EDIT layer) and lane levels
are not touched.

| Style | Kick | Snare / clap | Hats | Extras | Default DENSITY |
| --- | --- | --- | --- | --- | --- |
| BERLIN | Four on the floor | None | Straight 8ths, 16ths at high density | Rim ticks, an open hat | 70 % |
| DETROIT | Four on the floor, a push on 11 | Clap on 2 and 4 | 8ths, open hat off the beat | Tom, rim | 70 % |
| DUB | 1 and 9, a push on 11 | Rim on 2 and 4 | 8ths | Open hat | 55 % |
| MELODIC | Four on the floor | Clap on 2 and 4 | Off-beat 8ths, more at high density | Open hat, a tom on 16 | 70 % |
| ACID | Four on the floor, a push on 12 | Snare on 2 and 4, clap | Off-beat 16ths | Open hat, rim; the most ratchets | 75 % |
| TRANCE | Four on the floor | Clap on 2 and 4 | 8ths with open hats off the beat | Rim | 85 % |
| SYNTH | 1 and 7, a push on 12 | Snare on 2 and 4 | 8ths | Open hat, tom | 70 % |
| AMBIENT | 1 only | None | Sparse | Open hat, tom, rim, a bell | 45 % |

Pick the kit first (PRESETS, or EDIT > K1 KIT): the same beat sounds very different on 80, DEEP or PERC. The beat is
ordinary drum-grid steps once applied, so edit it on the grid, mute lanes in the EDIT layer, or run the lane tools over it.

## 7. STARTER: a song start on all four tracks

**STARTER** (SEQ TOOLS **G5**, from 0.6) is FORGE for the whole song. One press loads a sound on each of the four
tracks and writes a pattern for each, all from one seed:

| Track | Part |
| --- | --- |
| T1 | BASS |
| T2 | LEAD |
| T3 | PAD (chords) |
| T4 | DRUM (T4 becomes a DRUM track) |

1. Select a track and set its **ROOT** on the SCL page. All four tracks take that ROOT. Leave SCALE on CHR to use the
   starter's own scale, or set one to use yours.
2. Hold **SEQ** and tap **G5** (on any track). Four sounds load and four patterns are written at once; the tempo goes
   to the starter's BPM (not when the FM-1 follows an external MIDI clock).
3. Turn **K1 SEED** for another song idea and **K4 STARTER** for another genre: each starter loads its own four
   sounds and tempo. **K2 DENSITY** makes all four busier or sparser (50–150 % of each part's own), and **K3 EVOLVE**
   works as in FORGE. **C4** gives all four new notes, **D4** new rhythms.
4. Press **OCT+** (or G5 again) to keep it, or **OCT−** to put all four tracks back exactly as they were: sounds,
   settings, steps, automation and tempo.

The screen shows a row a track: its preset and its first 16 steps. Under them, the C4 and D4 counts.

| Starter | T1 BASS | T2 LEAD | T3 PAD | T4 DRUM | BPM | LEN (T1–T4) | SCALE* |
| --- | --- | --- | --- | --- | --- | --- | --- |
| BERLIN | ANALOG SQR BASS: 16th pedal sequence, octave jumps | TRIO ARP LEAD: a high, sparse line | GRAIN CLOUD PAD: one long chord a bar | DEEP KIT | 120 | 16 16 32 16 | MIN |
| DETROIT | FM6 FM BASS: off-beat root and fifth | ANALOG PWM STR: 4-note chord stabs | FM6 TINE EP: 4-note chords | TIGHT KIT | 126 | 16 16 32 16 | DOR |
| DUB | ANALOG SUB BASS: sparse, held | PHASE STRING: dub chord stabs | GRAIN SHIMMER: one chord a bar | DEEP KIT | 118 | 16 16 32 16 | DOR |
| MELODIC | TRIO FAT BASS: rolling off-beat 16ths | ANALOG SAW LEAD: a rising and falling line | FM6 PAD: long chords | DRUM KIT | 122 | 16 32 32 16 | MIN |
| ACID | ANALOG ACID: the 303 line, slides | LOFI ARP 8BIT: an arpeggio | ANALOG SOFT PAD: one chord a bar | TIGHT KIT | 128 | 16 16 32 16 | MIN |
| TRANCE | ANALOG SUB BASS: off-beat | TRIO ARP LEAD: the rolling triad arpeggio | ANALOG STRINGS: long chords | DRUM KIT | 138 | 16 16 32 16 | MIN |
| SYNTH | TRIO FAT BASS: 8th root and fifth | ANALOG BRASS: root-fifth riff | FM6 PAD: long chords | DRUM KIT | 110 | 16 16 32 16 | MIN |
| AMBIENT | ANALOG SUB BASS: a drone, one note a bar | PHYS KALIMBA: wide, sparse notes | GRAIN CLOUD PAD: very long chords | PERC KIT | 80 | 32 32 64 16 | LYD |

\* The starter's SCALE is used only when the selected track's SCALE is CHR (the default). Any other SCALE you set is kept.

**What STARTER changes:** each track's sound (engine and factory preset, with its effect sends), its pattern, LEN,
DIV (1/16), ROOT and SCALE, and the tempo. **It leaves alone:** each track's LEVEL, PAN and MUTE, the master and
the effects' own settings. Steps that held something before and are now empty lose their parameter locks when you
apply, as with FORGE; a sound load also drops automation of the old engine's own controls.

**Undo:** STARTER changes four tracks, and SAVE-held undo holds only one, so after STARTER it has nothing to undo.
**OCT−** while STARTER is open is the way back. Save your project first if it matters.

**Seeds:** the same SEED, C4 and D4 numbers, STARTER, DENSITY, EVOLVE and ROOT (and SCALE, if you set one) always
give the same four patterns. A set of starter seeds is pinned by a test, as FORGE's are.

**CPU:** each starter's four engines were chosen to play together, but FORGE ONE has not yet been tested on a real
FM-1. If you hear voices drop out, swap the pad for a lighter engine (ANALOG or FM6) on the EDIT layer.

## 8. What FORGE writes, and what it leaves alone

**Writes:** steps 1 to LEN of the selected track. On a synth track each step gets notes (1, 3 or 4), NOTE, TIE or REST, accent,
slide, ratchet (×2 or ×3), and a chance or condition. Velocity follows the beat: notes on the beat are louder and
off-beat 16ths softer, with a little variation, between 40 and 124 (BERLIN, TRANCE and ACID the widest, DUB and
AMBIENT the gentlest). Accented notes play at 127 as always. Every copy of the motif has the same velocities.

**Leaves alone:**

- steps past LEN
- the track's LEN, DIV, SWING and GATE, its sound, and its automation
- the other three tracks

**Parameter locks:** when you apply a pattern, steps that held something before and are now empty lose their locks,
as with EUCLID. Undo (SAVE held) brings them back.

**Compatibility:** FORGE only uses fields that Felucca 1.6 already stores, so projects made with FORGE open on stock
Felucca 1.6 and 1.6.1. Projects with conditional steps do not open on Felucca 1.5.1 or older. FORGE at EVOLVE 2 and
up can write conditions, so check SEQ > DETAIL or AUTOMATION before going back.

**Seeds and settings:** the same SEED, C4 and D4 numbers, STYLE, DENSITY, EVOLVE, LEN, ROOT and SCALE always give
the same steps.
From FORGE 0.3 on, a set of seeds is pinned by a test, so a later version only gives a seed a different pattern when
its changelog says so. Patterns you want to keep for certain belong in a project.
Change any of them and the pattern changes. At EVOLVE 0, changing DENSITY keeps the pitch of the notes that stay;
with EVOLVE 1 and up it can also change how the later copies drift.

## 9. When it does not do what you expect

| What you see | Why, and what to do |
| --- | --- |
| F5 does nothing | A SONG is playing ("STOP TO EDIT"), or the firmware is older than 0.5 on a DRUM track |
| G5 does nothing | A SONG is playing ("STOP TO EDIT"), or the firmware is older than 0.6 |
| STARTER changed my sounds and I want them back | Press **OCT−** before letting go of SEQ. After applying, there is no undo for STARTER: load your project again |
| The notes are too low or too high | Each style has its own octave. To move a pattern, set **QNT** to **SEQ+T** on the SCL page and turn **TRN** (TRN alone, and the octave buttons, only move the keys). Or try a style in another octave |
| Chords play as single notes | The voice mode is MONO, LEG or UNI. Set **POLY** on EDIT > VOICE |
| ACID slides do not glide | Use MONO or LEG voice mode, and some GLIDE on EDIT > VOICE (K2) |
| The pattern changes every loop and I want it fixed | Set EVOLVE to 0, or remove chances and conditions on SEQ > DETAIL |
| The knobs edit LEN, not SEED | FORGE is not open (F5 is not lit). Tap F5 while holding SEQ |
| I applied it and want the old steps back | Hold **SAVE** before using another tool (one undo). Or press **OCT−** before letting go of SEQ: it puts back everything the layer changed since it opened, LEN, DIV, SWING and GATE included |

## 10. Credits

FORGE is part of FORGE ONE, a fork of [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita, Hügelton
Instruments, and contributors. GPL-3.0-only. Source: [github.com/kkasemsa/FORGE-ONE](https://github.com/kkasemsa/FORGE-ONE),
`forge` branch; FORGE's code is in `firmware/src/ui_forge.c`, STARTER's in `firmware/src/ui_starter.c`.
