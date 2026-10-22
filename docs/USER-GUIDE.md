# FORGE ONE User Guide

Felucca 1.6.1 with FORGE 0.9.2 · M-VAVE FM-1

This guide covers the whole instrument: the Felucca firmware by Hügelton Instruments, and FORGE, the motif generator
this fork adds. Felucca's own sections follow its official manual, the [Felucca wiki](https://github.com/hugelton/Felucca/wiki),
in the order and with the labels the FM-1 itself uses. FORGE has its own chapter (chapter 9) and its own short guide.

Values given as settings are starting points. Judge them by ear.

**Contents**

1. Installing and going back
2. The panel
3. Basic gestures
4. Tracks and sounds
5. The pages, button by button
6. The sequencer
7. SEQ TOOLS
8. Songs
9. FORGE
10. Quick layers
11. Engines
12. Effects
13. Modulation
14. Scales, chord keys and the arpeggiator
15. Saving: sounds, patterns and projects
16. MIDI and USB
17. The menu
18. Playing with other gear
19. Troubleshooting
20. Appendix A: Basic operations
21. Appendix B: Compatibility between versions
22. Credits and licence

---

## 1. Installing and going back

**FORGE ONE** is installed with Felucca's command-line installer, because the official web installer only
installs stock Felucca. The steps are in the [Read Me First](../README.md#install-macos). In short:

1. Download `forge-one.fwsc` from the fork's Releases page (or from the newest green Actions build).
2. Quit every app that uses MIDI. Connect the FM-1 with a USB data cable.
3. Install `mido` and `python-rtmidi` in a Python environment, then run the installer (exact commands in the Read Me First).
4. Wait for the FM-1 to restart. The start-up screen says **FORGE ONE** and its version; MENU > SYSTEM > ABOUT
   shows FORGE ONE (by Khantipol) above Felucca's credit. Hold SEQ: the SEQ TOOLS map shows FORGE on F5, STARTER on G5.

**Stock Felucca** is installed from the [web installer](https://hugelton.github.io/Felucca/) in Chrome or Edge: connect
the FM-1, open the page, press **Install**. No extra hardware is needed. You can also
[try Felucca in your browser](https://hugelton.github.io/Felucca/webapp/try/) without an FM-1.

**Going back to the M-VAVE firmware:** use the web installer's **Return to official V15**, or M-VAVE's updater.

**Before any install**, make a full backup with the [web editor](https://hugelton.github.io/Felucca/webapp/editor/).
The backup holds projects, presets and settings, but not the FM6 voice bank: keep its `.syx` file.

Installing firmware is at your own risk. If an install is cut off, see chapter 19.

---

## 2. The panel

| Area | Controls | What they do |
| --- | --- | --- |
| Left knobs | **MASTER** | Volume (speaker, headphones, and USB audio unless MENU > USB LEVEL is FIXED) |
| | **SELECT** | Tempo (BPM). MENU > CONTROL > SELECT can make it step pages, play the mod wheel or set key velocity |
| | **PRESETS** | The selected track's sound, on every page that is not a list |
| | **ALGORITHM** | Selects track T1–T4, on every page |
| Right knobs | **KNOB 1–4** (K1–K4) | The four values shown on the screen's knob cards |
| Buttons, row 1 | **FX, SCL, ENV, LFO, EDIT, GLO** | Page buttons. Tap for the page, tap again for the next page. FX, SCL, EDIT and GLO also have a quick layer when held |
| Buttons, row 2 | **HOME, SAVE, ARP, SEQ** | Page buttons. HOME held opens the menu, SAVE held is undo, SEQ held is SEQ TOOLS |
| | **PLAY/STOP** | Starts and stops all four tracks. Lit green while playing |
| | **REC** | Arms the selected track to record. Held: the REC layer |
| Left of the keys | **OCT−, OCT+** | Octave down / up. On action pages and in dialogs, OCT+ does it and OCT− goes back |
| Keys | 27 keys, **F3 to G5** | 16 white keys and 11 black keys. They play the selected track, and in layers they are shortcuts |

**The screen.** The header shows the selected track, the project slot its pattern came from ("1 A", or "1 A*" once
edited), the play state, a metronome and the tempo. Below it are the four knob cards, then the page's picture
(oscilloscope, piano roll, envelope …), then a footer with hints.

**The keys' names.** This guide calls the keys by note name: the white keys are F3 G3 A3 B3 C4 D4 E4 F4 G4 A4 B4 C5
D5 E5 F5 G5, and the black keys F#3 G#3 A#3 C#4 D#4 F#4 G#4 A#4 C#5 D#5 F#5. "Black keys 1–4" are F#3, G#3, A#3 and C#4.

---

## 3. Basic gestures

| Gesture | What it does |
| --- | --- |
| Tap a page button | Opens its page; tap again for the next page of that family. Page buttons act when you let go |
| Tap HOME | Back to the HOME screen. On HOME, taps go HOME → MIXER → CLOCK |
| Hold a page button (FX, GLO, SCL, EDIT, REC, SEQ) | Opens its **quick layer**: the keys and knobs become shortcuts and a map shows (chapter 10) |
| Double-tap a layer button | Keeps the layer open without holding. Tap the button again to close it |
| Hold **HOME** | Opens the menu (chapter 17) |
| Hold **SAVE** | Undo the last load or edit. Hold again to redo |
| Hold **LFO** and turn a knob | **MOMENTARY**: the value jumps back when you let go of LFO. OCT+ while holding keeps it |
| **OCT+ / OCT−** | Octave up / down. On action pages and in dialogs: OCT+ does it, OCT− goes back. In the menu: change the value |
| OCT− + OCT+ together | Octave reset. Held for 5 s: UBOOT, the chip's boot mode for updates (a countdown shows from 2 s; let go to cancel) |
| **GLO + PLAY** | Restart from the top without stopping |

What can be pressed **breathes** softly, rather than blinking. Idle buttons and keys glow dim so you can find the panel
in the dark (MENU > DISPLAY > LEDS).

---

## 4. Tracks and sounds

- **Four tracks**, T1–T4. Each is one synth part with its own engine, sound and 64-step pattern. Drums are a track
  with the DRUM engine.
- **8 voices** are shared between the four tracks.
- **ALGORITHM** selects the track on every page. The pages and the keys edit and play the selected track.
- **PRESETS** changes the selected track's sound on every page that is not a list. Loading a sound never touches
  the track's pattern.
- **Thirteen engines** (chapter 11), each with factory presets. Every factory sound has a category (BASS, LEAD, PAD,
  PLUCK, KEYS, DRUM, FX, OTHER) to browse by.
- **User presets:** 32 slots. **Projects:** 4 slots, A–D (chapter 15).
- **INIT** loads the engine's first preset. **CLEAN INIT** loads its basic sound with nothing added: no FX sends,
  INSERT off, no LFO, ENV or MOD amounts, no glide, POLY voice mode.
- **RESTORE LAST** (on by default): turn the FM-1 off and on and the music is as you left it.

---

## 5. The pages, button by button

Tap a button for its first page, and again for the next. The knob cards show K1–K4 in order.

| Button | Pages (taps go round) | Held |
| --- | --- | --- |
| **HOME** | HOME, MIXER, CLOCK | The menu |
| **SEQ** | STEP, DETAIL, AUTOMATION | SEQ TOOLS, on every page |
| **GLO** | SONG | The GLO layer |
| **SAVE** | USER, PRESETS, PHRASES, PROJECT, TOOLS | Undo |
| **LFO** | LFO, LFO 2, LFO DEST, MOD | MOMENTARY (with a knob) |
| **EDIT** | EDIT 1, EDIT 2, (engine pages), VOICE, VOICE 2, VOICE 3 | The EDIT layer |
| **FX** | FX, SLICER, INSERT, INSERT 2, DLY, REVERB, CHORUS | The FX layer |
| **SCL** | SCL, CHORD | The SCL layer |
| **ENV** | ENV, ENV DEST | |
| **ARP** | ARP, ARP 2 | |
| **REC** | (arms the track) | The REC layer |

**Knobs on each page**

| Page | K1 | K2 | K3 | K4 |
| --- | --- | --- | --- | --- |
| HOME | The selected track's four main sound values (with MENU > HOME LEVELS: T1–T4 LEVEL) | | | |
| MIXER | LEVEL | PAN | REV (send) | MUTE |
| CLOCK | BPM (40–240) | SWING | CLOCK source: INT, USB, TRS | |
| STEP | The cursor step | | A note's LEN (ties) | |
| DETAIL | CHANCE (and conditions) | RATCH | NUDGE | VEL |
| AUTOMATION | The row (or PRESETS) | Its step | Its parameter | Its value |
| SONG | SECTION | TRACK | PAT (slot) | REPS |
| USER | SLOT | K2–K4 pick LOAD, ERASE or SAVE; OCT+ does it | | |
| PRESETS | Browse (as PRESETS) | ENGINE | FAV | LIST |
| PHRASES | PHRASE | | | |
| PROJECT | SLOT | ERASE | LOAD | SAVE (turn a knob to pick the action; OCT+ does it) |
| TOOLS | CLEAR SEQ | INIT (one click on: CLEAN INIT) | | (OCT+ does it) |
| LFO | RATE | WAVE: SIN, TRI, SAW, SQR, S&H | PHS (phase) | FADE |
| LFO 2 | SYNC: OFF, 4BAR … 1/32 | TRIG: NOTE, FREE | POL: BI, UNI | |
| LFO DEST | PIT | FLT | SHP | AMP |
| MOD | SLOT (1–4) | SRC | DST | AMT |
| EDIT 1 / EDIT 2 | The engine's E1–E4 / E5–E8 | | | |
| FILTER (ANALOG) | TYPE: LP, BP, HP | CUT | RES | KTR |
| LANES / LANES 2 (DRUM) | KICK, SNARE, CLAP, HATCL / HATOP, TOM, RIM, BELL levels | | | |
| VOICE | VCE: POLY, MONO, LEG, UNI | GLIDE | GLMOD: RATE, TIME | PRIO: LAST, LOW, HIGH |
| VOICE 2 | ALLOC: ROT, REUSE | DTUNE | PAN | MUTE |
| VOICE 3 | SPRD (stereo spread) | | | |
| FX | DST (distortion) | CHO send | DLY send | REV send |
| SLICER | OFF, GATE, STUT | PAT (1–16) | RATE | DEPTH |
| INSERT | TYPE | A | B | C (named by TYPE) |
| INSERT 2 | MIX (dry / wet) | | | |
| DLY (all tracks) | TIME | FDBK | COLR | MIX |
| REVERB (all tracks) | TYPE: ROOM, SPRING, HALL | SIZE | DAMP | |
| CHORUS (all tracks) | RATE | DEPTH | | |
| SCL | ROOT | SCL | QNT | TRN |
| CHORD | CHRD | VOIC | | |
| ENV | ATK | DEC | SUS | REL |
| ENV DEST | FLT | PIT | SHP | ESYNC |
| ARP | MODE | RATE | OCT | GATE |
| ARP 2 | SWG | PROB | HOLD (latch) | ORD: NOTE, PLAY |

- **EDIT's engine pages** come after EDIT 2: FILTER on ANALOG; LANES and LANES 2 on DRUM; SLICES on SLICE; OPERATOR,
  OP ENV and OPERATOR 2 on FM6.
- **DLY, REVERB and CHORUS** are shared by all tracks. Each track sends to them from its FX page.

---

## 6. The sequencer

Each track has a 64-step pattern with its own **LEN** (1–64), **DIV** (4BAR … 1/32, triplets included),
**SWING** and **GATE**. Set these four in the SEQ TOOLS layer (hold SEQ, K1–K4).

**A step** holds up to 4 notes (a chord), or is a TIE (it holds the note before) or a REST. It can also have:
**accent**, **slide**, **CHANCE** or a **condition**, **RATCH** (played 2, 3 or 4 times in its length), **NUDGE**
(timing) and **VEL**.

### 6.1 Entering notes

- **Step recording.** On SEQ > STEP with the transport stopped, press **REC**, then play notes: each key (or chord)
  goes into the cursor step and the cursor moves on. MENU > CONTROL > CHORD ENTRY picks how chords are entered:
  HOLD (keys held together become the step) or ADD (each key adds a note, K1 moves on).
- **Live recording.** Press REC on any other page. If the transport is stopped, it starts playing too. Play, and the
  notes are recorded as the loop goes round (overdub). Each note keeps your timing as a NUDGE.
- **The piano roll.** SEQ > STEP draws the pattern. K1 moves the cursor, K3 sets a note's length (right adds ties,
  left removes them). When the pattern is longer than one screen, the roll follows the play position while playing.
- **The drum grid.** On a DRUM track, SEQ > STEP is a grid: white keys are steps, black keys 1–8 pick the lane
  (KICK, SNARE, CLAP, HATCL, HATOP, TOM, RIM, BELL). Steps 1, 5, 9 and 13 are marked.
- **STEP PREVIEW** (MENU > CONTROL): moving the cursor while stopped plays the step it lands on.

### 6.2 DETAIL: chance, ratchet, nudge, velocity, conditions

SEQ > DETAIL is the STEP page with K1–K4 as the cursor step's **CHANCE**, **RATCH**, **NUDGE** and **VEL**, drawn as
lanes over the steps. PRESETS walks the steps; on a DRUM track a white key picks the step. **EDIT** puts the step's
four values back to their defaults.

**Conditions.** Turn K1 past 100 % to choose a condition instead of a chance:
**1:2 2:2 1:3 2:3 3:3 1:4 2:4 3:4 4:4 FILL !FILL**.

- **a:b** plays the step on loop *a* of every *b* loops of its track: 1:2 every other loop, 4:4 only every 4th.
  Each track counts its own loops, from PLAY and from the start of each SONG section.
- **FILL** steps play only while **GLO + E4** is held; **!FILL** steps only while it is not.
- A step has a chance or a condition, not both.

### 6.3 Timing: NUDGE and QUANTIZE

Live recording keeps when you played each note as a per-step **NUDGE**, in 1/16 of a step (from half a step early to
7/16 late). The track's **QUANTIZE** decides what plays: ON (the default) plays every step on the grid, OFF plays your
own timing. Set nudges by hand on DETAIL (K3), or hold steps on STEP and turn PRESETS.

### 6.4 Parameter locks and automation

- **Locks.** On SEQ > STEP, hold a step and turn K1–K4: those values sound on that step only. The knobs lock the
  parameters of the sound page you showed last (HOME, ENV, LFO, FX, EDIT …). **EDIT** with the step held clears its locks.
- **Automation.** Knob moves can be recorded per step while recording. An automation icon marks the knob cards it drives.
- **SEQ > AUTOMATION** lists everything that happens on the track's steps: PLAY / CLEAR, QUANTIZE, each step's CHANCE,
  RATCH, NUDGE and VEL, its locks and its automation. Change a row's step, parameter or value, turn a lock into
  automation or back, add or delete rows (EDIT deletes). There are 128 records shared by the four tracks.

### 6.5 Metronome and count-in

**CLICK** (OFF, while recording, or always) at three levels, and a **COUNT-IN** of 1 or 2 bars before recording from
stop. Set them in the REC layer or MENU > AUDIO. The click goes to the headphones and speaker only, never to USB audio
or into a pattern.

---

## 7. SEQ TOOLS

Hold **SEQ** on any page. The white keys become pattern tools for the selected track, and K1–K4 are **LEN**, **DIV**,
**SWING** and **GATE**. Each tool is one undo (SAVE held). **OCT−** puts the track back as it was when the layer opened.

| Key | Tool | What it does |
| --- | --- | --- |
| F3 | CLEAR | Clears steps 1 to LEN |
| G3 | REVERSE | Plays the pattern backwards |
| A3 / B3 | SHIFT < / > | Moves every step one left / right |
| C4 | RANDOM | A new beat (DRUM), or new notes in the scale on the same rhythm |
| D4 | COOK | Changes the pattern a little; press again for more |
| E4 | BEAT | DRUM only: four on the floor, backbeat, 8th hats |
| F4 | EUCLID | A Euclidean rhythm: K1 HITS, K2 STEPS, K3 ROT. OCT+ keeps it, OCT− cancels |
| G4 – C5 | Lane CLEAR, REVERSE, FILL, RANDOM | DRUM only: the selected lane (black keys 1–8 pick it) |
| D5 | DOUBLE | The pattern twice (LEN ×2, up to 64 steps) |
| E5 | HALF | The first half (LEN ÷ 2) |
| **F5** | **FORGE** | FORGE ONE: a seeded motif generator, a melody on a synth track or a beat on a DRUM track (chapter 9). While it is open, C4 and D4 reroll its two halves |
| **G5** | **STARTER** | FORGE ONE 0.6: all four tracks at once, a sound and a pattern each (bass, lead, pad, drums), from one seed (9.6). OCT− puts all four back |

Tools only work while no SONG is playing ("STOP TO EDIT").

---

## 8. Songs

**GLO > SONG** chains up to 16 **sections**. Each section names the project slot (A–D) that **each track** plays, or
"-" for a track that rests there, and how many times it repeats. The page draws a timeline with a lane per track.

- K1 SECTION, K2 TRACK, K3 PAT, K4 REPS.
- The white keys A3, B3, C4 and D4 set the slot.
- Each section starts every track's loop count again, so conditions work per section: in a section repeated 4 times,
  a 4:4 step plays on its last repeat.

---

## 9. FORGE

FORGE writes a musical idea onto a track in one key press, then lets that idea evolve: a melody on a synth track, a
beat on a DRUM track (9.5). Every pattern comes
from a **seed**: the same seed and settings always give the same steps. The [FORGE User Guide](FORGE-USER-GUIDE.md)
has more recipes and detail.

### 9.1 Using it

1. Select a track. On a synth track set its ROOT and SCALE (SCL page) and LEN (SEQ TOOLS K1) first.
2. Hold **SEQ**, tap **F5**. The pattern is written at once (heard while playing) and drawn as a grid of steps:
   bright cells are notes with a pitch mark, half-bright cells are ties, dark cells are rests.
3. Turn the knobs. Each turn rewrites the pattern from the steps as they were before FORGE opened.
4. **OCT+** (or F5 again, another tool, or letting go of SEQ) keeps it. **OCT−** cancels.
5. **SAVE** held afterwards undoes the whole FORGE, if no other tool ran since.

| Knob | Parameter | Range | What it does |
| --- | --- | --- | --- |
| K1 | SEED | 0–99 | Picks the pattern |
| K2 | DENSITY | 10–100 % | How many of the style's steps sound (above 75 %, extra notes too) |
| K3 | EVOLVE | 0–4 | How much the idea changes across the pattern and from loop to loop |
| K4 | STYLE | 8 styles | Rhythm and pitch profile. Any turn sets DENSITY to the style's default |
| C4 | New notes | NOTES 0–99 | The same rhythm, new notes |
| D4 | New rhythm | RHYTHM 0–99 | A new rhythm; each step that still sounds keeps its pitch (at EVOLVE 0) |

The header briefly shows `FORGE <seed> <style>` after each turn, and the screen shows the C4 / D4 counts under the
steps. K1 SEED sets both counts back to 0. F5 is lit while FORGE is open.

### 9.2 Styles

| Style | Motif | Rhythm | Pitch | Octave |
| --- | --- | --- | --- | --- |
| BERLIN | 8 | 16th ostinato | Small steps around the root | C3 |
| DETROIT | 16 | Syncopated stabs | 4-note scale chords | C4 |
| DUB | 16 | Off-beats, long ties | 3-note scale chords | C3 |
| MELODIC (card: MELO) | 16 | 8ths | Rises then falls in an arc | C4 |
| ACID | 16 | 16ths, accents, slides | Root pedal, octave jumps | C2 |
| TRANCE | 8 | Rolling 16ths | Triad arpeggios | C4 |
| SYNTH | 8 | 8ths | Root, fifth, octave | C3 |
| AMBIENT (card: AMBI) | 16 | Very sparse, long ties | Wide intervals, some chords | C4 |

Step 1 is always the root. Every note is in the track's ROOT and SCALE; chords are built from real intervals kept in
the scale, so they work in every scale (minor in CHR). Melodies move above and below the root; octave jumps only go up.

### 9.3 EVOLVE

| EVOLVE | What happens |
| --- | --- |
| 0 | A pure loop: every copy of the motif is the same; no chance, conditions or ratchets |
| 1 | Later copies sometimes sit a scale step away; a few ornament notes get a 75 % or 50 % chance; ACID and TRANCE ratchet a few |
| 2 | More drift; some ornaments get a condition (2:2, 3:4, 2:4, 4:4) |
| 3 | As 2, and later copies drop or add a note |
| 4 | The most change |

The first copy keeps the motif's notes and rhythm (it can still get chances, conditions and ratchets). With LEN equal
to the motif length there is only one copy.

### 9.4 What it writes

Steps 1 to LEN of the selected track: notes, NOTE / TIE / REST, accent, slide, ratchet, chance or condition, and
velocity (louder on the beat, softer on off-beat 16ths). It does not change steps past LEN, the track's LEN, DIV, SWING or GATE, its sound, or the other tracks. Applying
removes locks from steps that end up empty. Projects made with FORGE open on stock Felucca 1.6 and 1.6.1.

---

### 9.5 On a DRUM track

The same knobs and style names make a beat: K1 SEED, K2 DENSITY (the style's main hits stay from 50 % up, extra hats
over 75 %), K3 EVOLVE (0 a pure loop; 1 and up moves hats and adds ghost snares with a chance; 2 and up ends the last bar
in a snare-roll fill on a 4:4 condition; 3 and up moves kicks off the beat), K4 STYLE. **C4** rerolls the low lanes
(kick, snare, clap, tom) and **D4** the hats, rim and bell. The screen shows each step as eight strips, KICK at the
bottom, an accent brighter (at LEN 33 to 64, two columns: kick, snare, clap, tom on the left; hats, rim, bell on the right). The kick on step 1 is always there. The result is ordinary drum-grid steps.

| Style | Beat |
| --- | --- |
| BERLIN | Four on the floor, straight hats, rim ticks, no snare |
| DETROIT | Four on the floor, clap on 2 and 4, open hats off the beat |
| DUB | Kick on 1 and 9, rim on 2 and 4, sparse hats |
| MELODIC | Four on the floor, clap on 2 and 4, off-beat hats |
| ACID | Four on the floor, snare on 2 and 4, off-beat 16th hats, ratchets |
| TRANCE | Four on the floor, clap on 2 and 4, open hats off the beat, dense |
| SYNTH | Kick on 1 and 7, heavy snare on 2 and 4, 8th hats |
| AMBIENT | A kick on 1, sparse hats, tom, rim and bell |

### 9.6 STARTER: all four tracks (G5)

Hold **SEQ** and tap **G5** on any track. STARTER loads a sound on each track and writes a pattern for each from one
seed: **T1 BASS, T2 LEAD, T3 PAD, T4 DRUM** (T4 becomes a DRUM track). All four take the selected track's ROOT; the
SCALE is yours if you set one, or the starter's if the track is on CHR. The tempo goes to the starter's BPM, unless
the FM-1 follows an external clock.

K1 SEED, K2 DENSITY (50–150 % of each part's own), K3 EVOLVE and K4 GENRE (13 genres). Black keys 1–4 pick the
genre's variation (3 or 4 each, 45 starters in all); each loads its own sounds, tempo, scale and swing. Sounds in
*italics* are from the **FORGE PACK** (0.9): 41 sounds built on factory presets with their own settings (the full
list is in the FORGE User Guide); they save and load as ordinary edited sounds, and SAVE > USER keeps one. C4 rerolls the notes of all four, D4 the rhythms. **OCT+** (or G5 again) keeps it; **OCT−** puts
all four tracks back as they were, sounds, steps, automation and tempo included. SAVE-held undo cannot undo
STARTER, so use OCT− before you let go of SEQ. PAN and MUTE are never changed; LEVEL only by the pack's pads (0.9.2:
they sit 7 to 12 dB lower, so a chord does not bury the bass), and OCT− puts it back.

| Genre (K4) | Black key, variation | T1 bass | T2 lead | T3 pad | T4 kit · beat | BPM | Scale* | Swing |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Berlin | 1 **SEQ** | *BERLIN SEQ* | TRIO ARP LEAD | *BERLIN PAD* | DRUM DEEP KIT · Berlin | 120 | MIN | – |
|  | 2 **DRIFT** | LOFI WAVE BASS | *BERLIN RESO* | *STRING MACH* | DRUM PERC KIT · Ambient | 108 | MIN | – |
|  | 3 **CHOIR** | FM6 FM BASS | TRIO SYNC LEAD | *CHOIR PAD* | DRUM DEEP KIT · Berlin | 116 | HARM | – |
|  | 4 **ARP** | *DEEP BASS* | LOFI STEP LEAD | *BERLIN PAD* | *RHYTHM BOX* · Berlin | 124 | DOR | – |
| Detroit | 1 **STABS** | FM6 FM BASS | *WARM STAB* | FM6 TINE EP | *HOUSE KIT* · Detroit | 126 | DOR | – |
|  | 2 **STRNG** | *DEEP BASS* | ANALOG STRINGS | *HOUSE PIANO* | *HOUSE KIT* · Detroit | 128 | MIN | – |
|  | 3 **ELECT** | *ELECTRO BASS* | LOFI PULSE LD | PHASE STRING | *BOOM KIT* · Electro | 125 | PHRY | – |
|  | 4 **DEEP** | FM6 FM BASS | *GLASS ARP* | ANALOG SOFT PAD | DRUM DEEP KIT · Melodic | 124 | MIN | – |
| Dub Techno | 1 **CHORD** | *DUB SUB* | *DUB CHORD* | GRAIN SHIMMER | *DUB KIT* · Dub | 118 | DOR | – |
|  | 2 **DEEP** | VOICE WOW BASS | FM6 PAD | GRAIN CLOUD PAD | *DUB KIT* · Dub | 120 | MIN | – |
|  | 3 **MINI** | *DUB SUB* | PHASE RESO | *DUB PAD* | *TECHNO KIT* · Dub | 124 | DOR | – |
|  | 4 **ECHO** | ANALOG SUB BASS | FM6 TINE EP | GRAIN FROZEN | *DUB KIT* · Dub | 116 | MIN | – |
| Melodic | 1 **ARC** | TRIO FAT BASS | ANALOG SAW LEAD | FM6 PAD | DRUM DRUM KIT · Melodic | 122 | MIN | – |
|  | 2 **ARP** | *DEEP BASS* | TRIO SYNC LEAD | *TRANCE PAD* | DRUM TIGHT KIT · Melodic | 124 | MIN | – |
|  | 3 **PIANO** | FM6 FM BASS | SAMPLE PIANO | GRAIN CLOUD PAD | DRUM DEEP KIT · Melodic | 122 | DOR | – |
|  | 4 **PLUCK** | TRIO FAT BASS | *TRANCE PLUCK* | *CHOIR PAD* | DRUM DRUM KIT · Melodic | 123 | MIN | – |
| Acid House | 1 **CLASS** | *ACID 303* | *HOUSE PIANO* | ANALOG SOFT PAD | *HOUSE KIT* · Deep house | 124 | MIN | 10% |
|  | 2 **JACK** | *ACID SQUARE* | ANALOG RAVE | *CHOIR AAH* | *HOUSE KIT* · Deep house | 122 | DOR | 15% |
|  | 3 **FLOOR** | *ACID 303* | LOFI ARP 8BIT | PHASE ORGAN | DRUM DRUM KIT · Acid | 128 | MIN | – |
| Acid Techno | 1 **ROLL** | ANALOG ACID | *CHIP ARP* | ANALOG SOFT PAD | *TECHNO KIT* · Acid | 128 | MIN | – |
|  | 2 **WARE** | *ACID SQUARE* | LOFI STEP LEAD | ANALOG PWM STR | *TECHNO KIT* · Acid | 135 | MIN | – |
|  | 3 **HARD** | *ACID 303* | FM6 BRASS | PHASE WIRE | *TECHNO KIT* · Acid | 140 | PHRY | – |
| Trance | 1 **GATE** | ANALOG SUB BASS | TRIO ARP LEAD | *TRANCE PAD* | *HOUSE KIT* · Trance | 138 | MIN | – |
|  | 2 **LIFT** | ANALOG SUB BASS | *SUPERSAW* | *CHOIR PAD* | *HOUSE KIT* · Trance | 138 | MIN | – |
|  | 3 **PSY** | ANALOG SQR BASS | TRIO RING BELL | PHASE RESO | DRUM TIGHT KIT · Trance | 142 | PHRY | – |
| Deep House | 1 **KEYS** | *HOUSE BASS* | FM6 TINE EP | ANALOG SOFT PAD | DRUM DEEP KIT · Deep house | 122 | DOR | 15% |
|  | 2 **ORGAN** | VOICE WOW BASS | *HOUSE ORGAN* | ANALOG STRINGS | *HOUSE KIT* · Deep house | 120 | MIN | 20% |
|  | 3 **VOCAL** | ANALOG SUB BASS | *VOX CHANT* | FM6 PAD | DRUM DEEP KIT · Deep house | 121 | MIN | 10% |
| Synthwave | 1 **DRIVE** | *OCTAVE BASS* | ANALOG SAW LEAD | ANALOG PWM STR | *BOOM KIT* · Synthwave | 110 | MIN | – |
|  | 2 **NIGHT** | TRIO FAT BASS | *BRASS STAB* | FM6 PAD | *BOOM KIT* · Synthwave | 98 | MIN | – |
|  | 3 **8BIT** | LOFI WAVE BASS | *8BIT LEAD* | TRIO CHIP CHOIR | DRUM TIGHT KIT · Synthwave | 118 | MAJ | – |
| Breakbeat | 1 **AMEN** | *REESE* | FM6 BRASS | ANALOG STRINGS | *BREAK KIT* · Breakbeat | 134 | MIN | 10% |
|  | 2 **NUSK** | *ACID SQUARE* | LOFI STEP LEAD | PHASE STRING | *BREAK KIT* · Breakbeat | 135 | PHRY | 5% |
|  | 3 **ELECT** | *ELECTRO BASS* | *SCREAM* | FM6 PAD | *BOOM KIT* · Electro | 128 | MIN | 5% |
| Ambient | 1 **CLOUD** | *DRONE BASS* | FM6 BELL | GRAIN CLOUD PAD | DRUM PERC KIT · Ambient | 80 | LYD | – |
|  | 2 **DRONE** | PHYS DRONE STRING | SAMPLE FLUTE | *AIR PAD* | DRUM PERC KIT · Ambient | 72 | DOR | – |
|  | 3 **GLASS** | *DRONE BASS* | PHYS MARIMBA | *CHOIR PAD* | DRUM PERC KIT · Ambient | 84 | LYD | – |
|  | 4 **RAIN** | FM6 FM BASS | TRIO RING BELL | *GLASS PAD* | DRUM PERC KIT · Ambient | 76 | MIN | – |
| Lo-Fi | 1 **PIANO** | *LOFI BASS* | SAMPLE PIANO | *LOFI PAD* | *BOOM KIT* · Lo-fi | 84 | DOR | 25% |
|  | 2 **TAPE** | TRIO FAT BASS | *WARM KEYS* | WHEEL SOFT FLUTE | DRUM DRUM KIT · Lo-fi | 88 | MIN | 20% |
|  | 3 **SAX** | *LOFI BASS* | SAMPLE SAX | FM6 PAD | DRUM PERC KIT · Lo-fi | 80 | DOR | 22% |
| Jarre | 1 **OXY** | ANALOG SQR BASS | *OXY LEAD* | *STRING MACH* | *RHYTHM BOX* · Rhythm box | 104 | MIN | – |
|  | 2 **EQUI** | LOFI WAVE BASS | TRIO ARP LEAD | *CHOIR AAH* | *RHYTHM BOX* · Electro | 120 | MIN | – |
|  | 3 **FIELD** | FM6 FM BASS | *BRASS STAB* | ANALOG PWM STR | *RHYTHM BOX* · Rhythm box | 112 | DOR | – |
|  | 4 **ZOO** | VOICE WOW BASS | *VOX CHANT* | GRAIN SHIMMER | DRUM TIGHT KIT · Electro | 118 | PHRY | – |

\* The starter's SCALE is used only when the selected track's SCALE is CHR (the default). Any other SCALE you set is kept.

---

## 10. Quick layers

Hold a button for its layer. Keys pressed in a layer are shortcuts: they play no notes, send no MIDI and record
nothing. Double-tap the button to keep the layer open. In SET layers (GLO, SCL, EDIT, SEQ, REC), **OCT−** puts back
what the layer changed since it opened.

### FX layer (hold FX)

The white keys hold performance effects on the master while pressed. **Black keys 1–4 mute T1–T4** while held.
K1–K4 are the macros **FILTER**, **CRUSH**, **THROW** (the dry mix into the delay and reverb) and **DEPTH**.

| Key | Effect | Key | Effect |
| --- | --- | --- | --- |
| F3 | REPEAT 1/8 | A4 | OCT DN (harmonizer an octave down) |
| G3 | REPEAT 1/16 | B4 | FLANGER (in time with the tempo) |
| A3 | REPEAT 1/32 | C5 | PHASER (in time with the tempo) |
| B3 | REVERSE | D5 | REV THROW (every track's reverb send full) |
| C4 | LPF | E5 | ECHO THROW (every track's delay send full) |
| D4 | HPF | F5 | CUTOFF THROW (the master low-pass closes; FORGE ONE 0.7) |
| | | G5 | DIST THROW (every track's DIST at least 100; FORGE ONE 0.7) |
| E4 | TAPE STOP | | |
| F4 | FREEZE | | |
| G4 | OCT UP (harmonizer an octave up; DEPTH is its shimmer) | | |

- **Any white key can hold any effect:** hold the key in the layer and turn PRESETS. EDIT puts its default back.
- **MENU > CONTROL > FX LATCH** makes the effects toggle on and off, so nothing has to stay held. FX + OCT− turns
  everything off.
- **The throws flow back (FORGE ONE 0.7).** REV, ECHO, CUTOFF and DIST THROW come in at once, and when you let go
  they glide back to the tracks' own settings over about 0.75 s instead of stopping.
- **DUCK (FORGE ONE 0.7)** is a sidechain pump: while it is on, every kick of a DRUM track dips the other tracks by
  12 dB, and they come back over an 8th note. It is not on a key by default: assign it to a white key with PRESETS,
  or use PERFORM's D#5.

### PERFORM (double-tap FX) — FORGE ONE 0.7

Double-tap **FX** and the FX layer stays open as **PERFORM**: both hands free, mutes, solos, scenes and effects on
one screen. The white keys are the FX map's effects, exactly as above. The black keys and knobs change:

| Key | What it does |
| --- | --- |
| F#3 G#3 A#3 C#4 (black 1–4) | **MUTE** T1–T4, latched (press again to unmute) |
| **OCT+** held + black 1–4 | **SOLO** T1–T4, latched; several can be soloed |
| D#4 F#4 G#4 A#4 (black 5–8) | **Scenes 1–4**: tap to recall the mutes and solos stored there; hold 0.6 s to store the current ones |
| C#5 (black 9) | **QUANT**: BAR (mutes, solos and scenes wait for the next bar while playing) or NOW |
| D#5 (black 10) | **DUCK** on / off |
| F#5 (black 11) | **CLEAR**: every track on, no solo, every effect and macro off, at once |
| K1–K4 | T1–T4 **LEVEL**. Tap **OCT+** alone to switch them to the FX macros (FILTER CRUSH THROW DEPTH) and back |
| OCT− | Every effect and macro off (as FX LATCH's ALL OFF) |
| FX (tap) | Ends PERFORM |

The screen shows each track as ON, MUTE or SOLO (a track that will change at the next bar is lit in the waiting
colour), the scenes (stored ones bright, the last one recalled selected), QUANT, DUCK, the knobs' mode and the effects
that are on. Hold a white key to see the effect map, and turn PRESETS to reassign it as in the FX layer.

- **QUANT BAR** puts the change in exactly at the bar line, before the bar's first step plays. Stopped, changes are
  immediate. A change still waiting goes in at once when you STOP or switch QUANT to NOW (nothing waits for the next
  PLAY); loading a project drops it.
- Mutes are the tracks' MUTE (saved with the project, as GLO's). Solos stay after you leave PERFORM, until CLEAR or
  GLO's C4 UNMUTE ALL (which clears every mute, every PERFORM solo and a waiting change). DUCK stays until CLEAR, OCT-
  or FX LATCH off. Scenes and QUANT are kept with the settings, not the project.

### GLO layer (hold GLO)

| Key / knob | What it does |
| --- | --- |
| Black keys 1–4 | MUTE T1–T4 (latched; lit = sounding) |
| F3, G3, A3, B3 | SOLO T1–T4 while held |
| C4 | UNMUTE ALL (every mute, PERFORM's solos and a waiting QUANT change too) |
| D4 | MIDI LEARN on / off (chapter 16) |
| E4 | FILL while held (FILL / !FILL steps) |
| F4 | TAP tempo |
| K1–K4 | T1–T4 LEVEL |
| GLO + PLAY | Restart from the top without stopping |
| GLO + SELECT | Tempo, even with BPM LOCK |

### SCL layer (hold SCL)

Press any key: its note name becomes the track's **ROOT**. K1–K4 are ROOT, SCL, CHRD and VOIC. The LEDs show the root
lit and the scale's notes breathing.

### EDIT layer (hold EDIT)

The white keys from F3 pick the **engines**, one key each, in PRESETS order. After them come **INIT** (E5) and
**CLEAN INIT** (F5), which ask first. K1 ENG, K2 No. (the engine's sounds), K3 FAV. On a DRUM track the black keys
1–8 **mute its lanes** (KICK … BELL), for live play only: never saved or recorded.

### SEQ layer (hold SEQ)

SEQ TOOLS: chapter 7, FORGE on F5 and STARTER on G5: chapter 9.

### REC layer (hold REC)

| Key | What it does |
| --- | --- |
| F3 | CLEAR: the selected track's steps and automation (one undo) |
| G3 | CLICK: OFF, REC, ON |
| A3 | COUNT-IN: OFF, 1 BAR, 2 BARS |
| B3 | CLICK LEVEL: LOW, MID, HIGH |

K1–K3 set the same three settings. A tap of REC still arms and disarms the track.

---

## 11. Engines

In the order the FM-1 lists them:

| Engine | What it is |
| --- | --- |
| **ANALOG** | Virtual analog: two oscillators (WAVE, SYNC, SUB), noise, drive, a resonant LP / BP / HP filter (EDIT > FILTER) |
| **FM6** | Classic 6-operator FM (Dexed-based), 32 algorithms. SLOT picks a factory patch (F1–F8), the track's own (OWN) or a voice from a bank sent by the web editor (B1–B32). Edit on OPERATOR, OP ENV, OPERATOR 2, or in the web editor (imports `.syx`) |
| **PHASE** | Phase distortion, with a wavefolder (FOLD, EDIT 2) |
| **LOFI** | Chiptune: pulse, triangle, saw, noise and a 4-bit wave RAM, stepped envelope, sweep, arpeggio |
| **SAMPLE** | Multisampled PIANO, FLUTE, SAX, and 3 user sample slots |
| **VOICE** | Formant oscillator: sung vowels |
| **TRIO** | 3 oscillators with ring modulation and sync, multimode filter |
| **WHEEL** | Tonewheel organ: drawbars, percussion, key click, drive, rotary speaker |
| **GRAIN** | Granular textures from the built-in samples or a user slot |
| **PHYS** | Physical models: modal resonators, strings, struck membranes, sympathetic strings |
| **NOISE** | Noise from analog to digital: colours, crackle, shift-register and metallic tones |
| **SLICE** | A drum break, a piano note or your own sample cut into slices, one per key; set slices by hand on SLICES |
| **DRUM** | An 8-lane kit on the General MIDI key map, all synthesized. KIT: STD, 80, 10, 66, 55, 77, DEEP, TIGHT, PERC. EDIT > LANES sets each lane's level |

A missing sample (an empty user slot) plays a plain sine at the note's pitch, and the screen says NO SAMPLE once.
User samples are uploaded from the web editor.

---

## 12. Effects

**Per track**, in signal order:

1. **DIST** (FX page, K1): distortion.
2. **INSERT** (FX > INSERT): TYPE OFF, SOFT, HARD, FOLD, FUZZ (drives: DRIVE, TONE, LEVEL), CRUSH (BITS, RATE, LPF),
   PHASR, FLANG, CHOR (RATE, DEPTH, FDBK). **INSERT 2**: MIX (dry / wet).
3. **SLICER** (FX > SLICER): a tempo-synced 16-step GATE or STUTter with 16 patterns.
4. **Sends** (FX page, K2–K4): CHO, DLY and REV, to the shared effects.

**Shared by all tracks:**

- **DLY:** TIME (1/4, 1/8, 1/16, 1/32, triplets, 1/2, 1/1, 2BAR, 4BAR, and dotted 1/4D, 1/8D, 1/16D), FDBK, COLR, MIX.
  In time with the tempo; it holds up to 1.49 s.
- **REVERB:** TYPE **HALL** (default, smooth stereo, up to about 4.5 s), ROOM or SPRING; SIZE; DAMP.
- **CHORUS:** RATE, DEPTH.
- A **master limiter**.

The **FX layer** (chapter 10) adds performance effects on the master.

---

## 13. Modulation

- **ENV** (ENV page): ATK, DEC, SUS, REL. **ENV DEST**: the envelope's amount to FLT, PIT and SHP, and **ESYNC**, which
  sets ATK, DEC and REL as note values of the tempo (1/64T to 4 bars).
- **LFO** (LFO page): RATE, WAVE (SIN, TRI, SAW, SQR, S&H), PHS, FADE. **LFO 2**: SYNC to a note value (4BAR to 1/32,
  following the tempo or an external clock), TRIG (NOTE or FREE), POL (bipolar or unipolar). **LFO DEST**: the LFO's
  amount to PIT, FLT, SHP and AMP.
- **MOD matrix** (LFO > MOD): 4 slots per track. K1 picks the slot, then SRC, DST, AMT.
    - Sources: LFO, ENV, VEL, KEY, RAND, MODW, AT (aftertouch), EXPR, S&H, SLEW.
    - Destinations: PITCH, CUT, SHP, AMP, PAN, DIST, CHO, DLY, REV, RATE, VIB, E1–E8, DEPTH (the LFO's own depth),
    VEL and TIME (humanize: each note's velocity, a step up to 30 ms late).
- **MOMENTARY:** hold LFO and turn a knob on HOME or a sound / FX page; let go and the value jumps back.
- **SPREAD** (EDIT > VOICE 3): the voices of a POLY or UNISON track alternate left and right of its PAN.

---

## 14. Scales, chord keys and the arpeggiator

**SCL page:** ROOT, SCL (the scale), QNT, TRN (transpose ±24).

- **Scales:** CHR, MAJ, MIN, DOR, MIX, PEN, MPEN, HARM, PHRY, LYD, LOC, MEL, BLUES, WHOLE, DIMHW, DIMWH.
- **QNT** (how the keys and sequence follow the scale):
    - OFF: the normal chromatic keyboard.
    - SNAP: every key, rounded down to the scale.
    - WHITE: the scale on the white keys only; C4 plays ROOT, black keys are silent.
    - SEQ: as SNAP, and the sequenced notes are snapped as they play (the steps are not changed).
    - SEQ+T: as SEQ, and the sequence follows TRN too (record at TRN 0, then turn it).
- **MENU > SCALE LEDS** shows the scale on the keys all the time.

**CHORD page** (tap SCL again): **CHRD** makes one finger play a chord: OFF, DIA3 / DIA7 (the scale's triads or
sevenths), or fixed shapes MAJ, MIN, DOM7, MAJ7, MIN7, SUS4, POW. **VOIC**: CLOSE, OPEN, INV1, INV2, +OCT. Chord keys
work on the keys, MIDI in, recording and the arpeggiator.

**ARP page:** MODE, RATE, OCT (1–4), GATE. **ARP 2**: SWG, PROB, HOLD (latch), ORD.
Modes: UP, DN, UPDN, RND, ORD (the order you pressed), REPEAT, and for chords DNUP, UP+8, CONV, DIVG, PINKY, THUMB,
WALK, CHORD. The ARP button flashes on the beat.

**Voice modes** (EDIT > VOICE): POLY, MONO, LEG (legato), UNI (unison), with GLIDE.

---

## 15. Saving: sounds, patterns and projects

| What | Where | How |
| --- | --- | --- |
| **A sound** (user preset) | SAVE > USER | Stop, tap SAVE, pick a slot with K1, then OCT+ and OCT+ again. Name it with the keys. 32 slots, each with a category |
| **Load a factory or user sound** | SAVE > PRESETS, or turn PRESETS | K2 ENGINE, K3 FAV (favourites), K4 LIST |
| **A pattern from a project** | SAVE > PHRASES | K1 picks the phrase, OCT+ loads it into the selected track |
| **A project** (all four tracks, sounds, patterns, song) | SAVE > PROJECT | K1 picks the slot (A–D). Turn K4 to pick SAVE (K3 LOAD, K2 ERASE), then OCT+. EDIT names it. Save while stopped |
| **Clear a sequence, INIT a sound** | SAVE > TOOLS | K1 CLEAR SEQ, K2 INIT or CLEAN INIT, OCT+ does it |
| **Undo** | Hold SAVE | Undoes the last load or edit; hold again to redo |
| **Everything** | Web editor | Full backup and restore (not the FM6 voice bank), and one project to a file and back |

**RESTORE LAST** keeps an autosave of its own, written when the transport is stopped and nothing has been touched for
10 s. The four project slots are never overwritten by it.

---

## 16. MIDI and USB

**USB** is class-compliant: MIDI in and out, and a stereo audio input called "Felucca" (44.1 or 48 kHz) that records
the master output on the computer. No driver is needed. On macOS 13–15, set MENU > SYSTEM > USB SERIAL to OFF so the
audio input appears.

**MIDI in** comes over USB or TRS. **MENU > MIDI > MIDI IN** decides which channels play which track:

| MIDI IN | Channels |
| --- | --- |
| CH1-4 (default) | Channels 1–4 play T1–T4 |
| CH5-8, CH9-12, CH13-16 | Another block of four (for example two FM-1s on one cable) |
| SEL | Every channel plays the selected track |
| TRACK | Each track its own channel: MENU > MIDI > T1–T4 CHANNEL (1–16, OMNI or OFF) |

- The keys send on the selected track's channel. Pitch bend, sustain and panic are supported.
- **Clock** from INT, USB or TRS (HOME > CLOCK, K3).
- **AUTO SELECT:** a MIDI note into another track makes it the selected one.
- **CC map:** 5 GLIDE, 7 LEVEL, 10 PAN, 71 resonance, 72 release, 73 attack, 74 brightness, 75 decay, 91 reverb send,
  93 chorus send, 94 delay send.
- **MIDI LEARN** (GLO + D4): turn an FM-1 knob, then move a control on your controller. Up to 16 learned CCs, for any
  knob of any track. OCT− clears the picked one; MENU > MIDI > LEARN CLEAR clears them all.
- **Program Change** on a track's channel loads that track's sound by its number in PRESETS (from 0). The engine never
  changes.

---

## 17. The menu

Hold **HOME**. ALGORITHM moves between the five tabs, PRESETS through the rows, and any knob (or OCT+ / OCT−) changes
the value. Press HOME to close. The web editor's Settings tab can change all of these too.

| Tab | Rows |
| --- | --- |
| **DISPLAY** | COLOR (10 palettes, and CUSTOM from the web editor), STYLE (FLAT, LINE), LARGE, ANIM (ON, OFF, IDLE), LEDS (OFF, DIM LO, DIM HI, INV, INV LO, INV HI), SCREEN OFF (NEVER … 60 MIN), SCOPE (OUT, MIX), HOME (SCOPE, TRACKS, LEVELS) |
| **CONTROL** | HOLD (layer hold time), KNOB ACCEL, FX LATCH, BPM LOCK, SCALE LEDS, STEP PREVIEW, CHORD ENTRY (HOLD, ADD), SELECT (BPM, PAGE, MODW, VEL) |
| **AUDIO** | SPEAKER EQ (FLAT, LOWCUT, BASS+), USB LEVEL (MASTER, FIXED), CLICK, CLICK LEVEL, COUNT-IN, TUNE (±50 cents) |
| **MIDI** | MIDI IN, T1–T4 CHANNEL, AUTO SELECT, SCREEN WAKE (+MIDI, PANEL), LEARN CLEAR |
| **SYSTEM** | USB SERIAL, RESTORE LAST, HELP, 2ND CORE (beta), CALIBRATION, INFO, ABOUT |

- **SPEAKER EQ** also shapes the headphones and USB audio: keep it on FLAT when recording.
- **HELP ON** shows a short hint when a page or layer opens.
- **2ND CORE** (new in 1.6.1, beta, OFF by default): ON renders some parts on the second core from the next power-on,
  with the same sound. A unit whose second core misreads goes back to one core and sets it OFF.
- **INFO** shows the version, the USB link and the CPU load.

---

## 18. Playing with other gear

These are setup suggestions for sequencing the FM-1 from a controller, a groovebox or a DAW.

- **Four parts from one sequencer:** set MIDI IN to **CH1-4** and send each part on channels 1–4, or use **TRACK** and
  give each track the channel your sequencer already uses.
- **Tempo sync:** set HOME > CLOCK source to USB or TRS to follow an external clock. LFO SYNC, ENV SYNC, the delay and
  the SLICER follow it too. COUNT-IN does not work with an external clock.
- **FORGE with external sequencing:** FORGE writes the FM-1's own patterns. Run the FM-1's sequencer in sync (external
  clock) for FORGE tracks, and play other tracks from your sequencer over MIDI.
- **Recording:** the USB audio input records the master; set USB LEVEL FIXED for a level independent of MASTER, and
  SPEAKER EQ to FLAT.

---

## 19. Troubleshooting

| Problem | What to do |
| --- | --- |
| An install stopped part-way | Run the same install command again (Read Me First): it finishes the write |
| The screen stays black after an install | Check for a USB device named **WL80UBOOT** (boot mode); the MIDI installer cannot reach it. Try another USB data cable, quit every MIDI app and run the [web installer](https://hugelton.github.io/Felucca/) again. If it stays in boot mode, [FM-1 Transporter](https://github.com/kurogedelic/FM-1-transporter) with a Seeed XIAO RP2040 can write the official firmware |
| The installer cannot find the FM-1 | Use a data cable, not a charge-only one. Quit Ableton Live and other MIDI apps |
| The Mac does not see the USB audio input | MENU > SYSTEM > USB SERIAL OFF (macOS 13–15) |
| A tool says STOP TO EDIT | A SONG is playing. Stop it first |
| F5 shows no FORGE | The firmware is stock Felucca, or older than FORGE 0.5 on a DRUM track |
| G5 shows no STARTER | The firmware is older than FORGE 0.6 |
| A step sometimes does not play | It has a CHANCE or a condition: check SEQ > DETAIL |
| Chords play one note | Voice mode is MONO, LEG or UNI: set POLY on EDIT > VOICE |
| A project does not open on an older firmware | See Appendix B |

Bugs in Felucca itself: [Felucca Issues](https://github.com/hugelton/Felucca/issues). Bugs in FORGE or this fork:
[FORGE ONE Issues](https://github.com/kkasemsa/FORGE-ONE/issues).

---

## 20. Appendix A: Basic operations

| I want to … | Do this |
| --- | --- |
| Pick a track | Turn ALGORITHM |
| Change a track's sound | Turn PRESETS |
| Change the tempo | Turn SELECT (or GLO + SELECT, HOME > CLOCK, GLO + F4 tap) |
| Start / stop | PLAY |
| Restart from the top while playing | GLO + PLAY |
| Record live | REC, then play |
| Enter steps | SEQ > STEP, stop, REC, play notes |
| Set pattern length | Hold SEQ, turn K1 (LEN) |
| Make a random variation | Hold SEQ, D4 (COOK) |
| Generate a motif | Hold SEQ, F5 (FORGE), turn K1–K4, OCT+ |
| Lock a value on one step | SEQ > STEP, hold the step, turn a knob |
| Mute a track | Hold GLO, press its black key (1–4) |
| Play a fill | Hold GLO + E4 (FILL steps) |
| Throw the reverb | Hold FX + D5 (REV THROW) |
| Undo | Hold SAVE |
| Save a sound | Stop, SAVE, K1 slot, OCT+, OCT+ |
| Save a project | SAVE (tap on to PROJECT), K1 slot, turn K4 (SAVE), OCT+ |
| Open the menu | Hold HOME |

---

## 21. Appendix B: Compatibility between versions

| From | To | What to know |
| --- | --- | --- |
| FORGE ONE | Felucca 1.6 / 1.6.1 | Everything opens and plays the same; FORGE patterns are ordinary steps |
| 1.6 / 1.6.1 (and FORGE) | 1.5 / 1.5.1 | Projects with **conditional steps** do not open there (FORGE at EVOLVE 2 and up can write them). Clear conditions first, or keep a backup. MIDI IN TRACK plays as a channel block, FOLD is ignored, DEEP / TIGHT / PERC play as 77, MOD VEL / TIME read as DEPTH |
| 1.5 or later | 1.2 – 1.4.1 | Projects saved by 1.5 or later do not open |
| 1.4 or later | 1.1.5.x | Projects saved by 1.4 or later do not open |

Every release opens everything older releases saved. Full notes: [Felucca release notes](https://github.com/hugelton/Felucca/wiki/Release-Notes).

---

## 22. Credits and licence

**Felucca** is by Leo Kuroshita ([@kurogedelic](https://github.com/kurogedelic)), Hügelton Instruments, with many
contributors: see [CREDITS.md](../CREDITS.md). Its engines and DSP include work ported from CrispyZebra, DaisySP, Mutable
Instruments' eurorack code, Dexed, klattsch and others, each under its own licence ([LICENSES/](../LICENSES/)). This guide
draws on the [Felucca wiki](https://github.com/hugelton/Felucca/wiki), the official manual.

**FORGE** and this fork, FORGE ONE, are by the FORGE ONE contributors.

Felucca and FORGE ONE are free software under GPL-3.0-only. M-VAVE and FM-1 are trademarks of their respective
owners; neither project is affiliated with or endorsed by them.
