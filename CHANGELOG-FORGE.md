# FORGE changelog

Newest first. FORGE is the SEQ TOOLS F5 motif generator in FORGE ONE (based on Felucca 1.6.1); STARTER (G5, 0.6; 45 starters from 0.8, the FORGE PACK from 0.9) is its song starter; PERFORM (FX double tap, 0.7) its live mode.

| Version | Date | Theme |
| --- | --- | --- |
| 0.14 | 24 Oct 2026 | Three classic drum kits: 90, MP, SD; 808 and 909 kits in the pack |
| 0.13 | 23 Oct 2026 | SONG: four parts A–D and the arrangement from one seed |
| 0.12 | 23 Oct 2026 | F5 follows the track's sound: bass lines on a bass, held chords on a pad |
| 0.11 | 23 Oct 2026 | A logo on the splash: one flame on an anvil |
| 0.10 | 23 Oct 2026 | The FORGE palette: FORGE ONE's own look, the default |
| 0.9.3 | 23 Oct 2026 | The splash names its maker |
| 0.9.2 | 22 Oct 2026 | First hardware feedback: the pads lower, CHOIR AAH −12 dB |
| 0.9.1 | 22 Oct 2026 | Fixes from the third code review: PERFORM state, build and release packaging |
| 0.9 | 22 Oct 2026 | FORGE PACK: 40 sounds for the starters |
| 0.8 | 22 Oct 2026 | 45 song starters in 13 genres, Jarre included |
| 0.7 | 22 Oct 2026 | PERFORM: live mutes, solos, scenes, QUANT; throws that flow back; DUCK |
| 0.6 | 22 Oct 2026 | STARTER: a song start on all four tracks, sounds included |
| 0.5.1 | 22 Oct 2026 | Fixes from the second code review |
| 0.5 | 21 Oct 2026 | FORGE on DRUM tracks: beats in the eight styles |
| 0.4 | 21 Oct 2026 | Velocity by the beat; C4 / D4 reroll notes or rhythm |
| 0.3 | 21 Oct 2026 | Safe live turns, chords in every scale, pinned seeds |
| 0.2 | 21 Oct 2026 | The generator: SEED, DENSITY, EVOLVE, 8 styles |
| 0.1 | 21 Oct 2026 | Scaffold: F5 FORGE cell, CI build |

## 0.14

Three more model kits on the DRUM engine's KIT knob, after PERC: **90**, **MP**, **SD**. Like DEEP TIGHT PERC they
are data only (a row of numbers each in `DV_KIT`, `drum_voice.c`): the same runs, nothing new in the sample loops, no
extra CPU. The old kits, their stored values and every pinned render are unchanged; a project saved with a new kit
plays KIT STD's lane voices on stock Felucca 1.6.1 (an unknown KIT value is held to its range).

- **90**, the later analog drum machine's kit: a 55 Hz kick swept from about 210 Hz with a strong click (t-30 0.4 s),
  a brighter, snappier snare with a crack, a longer clap tail, bright hats (the open one 0.73 s), 80's rim, and 10's
  crash on BELL (shown CYM).
- **MP**, the 70s organ-top rhythm box (the sound under the early ambient synth records): a soft 70 Hz thump with no
  click, a short noise snare, maracas on HAT CL, a long noise cymbal on HAT OP, a 280 Hz bongo on TOM (CONGA), 2.5 kHz
  claves on RIM (CLAVE), a noise cymbal on BELL (CYM); a low accent.
- **SD**, the 80s hexagonal electronic kit: a kick dropped from about 240 Hz, a 200 Hz snare that glides down an
  octave under loud noise, 90's clap and hats, a big 110 Hz tom bent hard as it rings (0.5 s), 80's rim, 10's crash.
- FORGE PACK (43 sounds): BOOM KIT is now **808 KIT** (KIT 80, the long boom; same values), TECHNO KIT is **909 KIT**
  (KIT 90), HOUSE KIT plays KIT 90; new **MINI KIT** (KIT MP, REV 30) and **SIMMONS KIT** (KIT SD, REV 30).
- Starters: JARRE (OXY, EQUI, FIELD) use MINI KIT; SYNTHWAVE DRIVE and NIGHT use SIMMONS KIT; Detroit, Acid House,
  Trance and Deep House kits are on 90 through HOUSE KIT, Dub MINI and Acid Techno through 909 KIT; Electro and Lo-Fi
  keep the 808 boom.
- Voiced by numbers (drum_test measures every lane: t-30, centroid, level against the STD kit's lane levels within
  1.5 dB); not yet heard on the instrument. The web editor's KIT list ends 90 MP SD.
- Tests: drum_test (targets for 90 MP SD, lane names, the KIT knob's order and end), v156_test (the kit count),
  forge_test (EQUI's MINI KIT), web tests (the KIT list).

## 0.13

SONG, built to the Song Mode spec v0.3 (MVP, §9), then v0.4.1 and v0.5 on the same branch.

**Spec v0.5, the performance layer (24 Oct 2026).**

- **D4 is FOCUS** (its reroll of every track's rhythm is removed; B3 rerolls T4's, C4 every unlocked track's notes,
  F3–B3 one track's). D4 held + F3 / G3 / A3 / B3 puts T1 BASS / T2 LEAD / T3 PAD / T4 DRUM on KNOB 1–4
  (`FOCUS T2 LEAD`); it stays after D4 is let go; D4 + the same key, or a D4 tap that picked nothing and turned
  nothing, turns it off. F3–B3 still decide at the key's down: FX held a mute, else D4 held the focus, else a reroll.
- **Faders:** D4 held alone, KNOB 1–4 are the focus track's (FOCUS off: the last focused, T1 at first); let go, the
  values stay. Knob priority: FX held (the macros, snapping back) > D4 held > FOCUS > SEED DENSITY EVOLVE GENRE.
- Params: T1 CUTOFF RESO ENV-MOD (`P_ED_FLT`) DECAY (`P_DEC`); T2 / T3 FILTER, A/R (`P_ATK` and `P_REL` by the same
  turn), SEND (`P_REV` and `P_DLY`), LEVEL; T4 KICK level (`P_LN0`), the kit's TUNE and DECY, DRV. Nearest where the
  spec's is missing: the filter by the engine (CUT FREQ TONE DCW BRIT MLVL, as BREAK's sweep), RESO as RES or Q,
  else FB (FM6) FOLD (PHASE) DUTY (LOFI), else DIST; DRUM has no per-lane tune or decay, so SNARE TUNE and HAT DECAY
  are the kit's TUNE and DECY; DECAY rather than ACCENT on T1 (no bass engine of the starters has an ACCENT edit;
  open question G2). The knob cards show the four with their values; the bottom line `FOCUS T2` / `FADER T1`.
- **Pickup** (§H1): the FM-1's knobs are endless encoders, so a turn always moves a value from where it is and none
  ever jumps; no pickup state or ▲ / ▼ hint is needed (test 35 checks this instead).
- **Scenes:** D5 / E5 recall SCENE 1 / 2, applied by the audio ISR at the next 1/16 before its step plays (`sc_apply`,
  seq.c; stopped: at once); FX + D5 / E5 stores the 20 focus params of T1–T4 and each track's engine (no PF_*
  effect). RAM only (`sg_scene`, 2 × 85 B), kept to power-off, not saved (v0.6). Review of 10456b0: a track whose
  engine changed since the store is skipped (`SCENE 1 SKIP T2`), every value is held to its param's range. No ramp:
  as with a knob turn, the engines read a param once a 32-sample block (0.7 ms) and the mix reads LEVEL and the sends
  once a block, so a value steps there (the 0.13 text that said the engines smooth it was wrong; only pitch bend
  is smoothed, voice.c). A short ramp on recall follows if test 42 hears a click.
- CLEAR keeps FOCUS, its values and the scenes; leaving SONG turns FOCUS off and keeps the params and scenes.
- Tests (forge_test `test_song_perf`, labelled 31–41; 42 is the device test): D4 tap does nothing to steps or
  rerolls, B3 still rerolls T4; focus select without reroll or mute; key-down priority in every order; T1's CUTOFF /
  RESO only; relative turns after a track change and a scene; FX over FOCUS snaps back; faders latch; FOCUS off by
  tap and by the same key; scene store / recall at the 1/16 boundary, `SCENE 2 EMPTY`; FX + D5 stores only; CLEAR and
  leaving SONG. Test 29 now covers F5 only. UI lint: 2 new screens (FOCUS T2, FOCUS T4 on part C).

**Spec v0.3 (23 Oct 2026).**

- **F#5 in STARTER opens SONG**: four related parts from one song seed, A VERSE, B CHORUS, C BRIDGE and D, which is
  LIFT (a bigger last chorus) in BERLIN, DEEP HOUSE, SYNTHWAVE, AMBIENT and JARRE and BREAK (a breakdown) in the
  other eight genres (`FS_GENRE_D`, next to `FS_GENRE`). Shared: the sounds, ROOT, SCALE, tempo, SWING, genre, seed.
- **GENERATE** (OCT+) asks `OVERWRITE A-D?` with the four slots' current names; OCT+ writes slots A–D (`S42 VERSE`,
  `S42 CHORUS`, `S42 BRIDGE`, `S42 LIFT` / `S42 BREAK`) and the 8-row default chain for D's type; OCT- writes nothing
  and puts the four tracks back.
- Black keys 1–4 play a part, 5–8 lock a track, C4 / D4 reroll the part shown (locked tracks byte-identical);
  after writing, a reroll asks `SAVE B?`: OCT+ saves that slot only, OCT- keeps it unsaved.
- No format change: parts are FUN10 projects, the arrangement the existing chain, labels the project names; stock
  Felucca 1.6.1 loads and plays them. Factory presets and every existing pinned render are unchanged.
- Choices the spec left to the build: entry key **F#5** (free in STARTER); reroll on **C4 / D4** for the part shown,
  with the part picked on black keys 1–4 (A3–D4 white keys are other tools, and C4 / D4 are the reroll keys); locks on
  black keys 5–8 (the FM-1 has no track keys). The kits have no crash, so CHORUS / LIFT accent step 1 with an open
  hat. Fills use the b:b condition of the section's last loop, or sit in a pattern as long as the section (up to 64
  steps), rather than `COND_FILL`, which only plays while FILL is held. BREAK's pad sweep uses automation events
  (they hold between steps; a step lock lasts one step) on the pad engine's cutoff (else FREQ, TONE, DCW, BRIT,
  MLVL; else LEVEL).
- Tests (forge_test, `test_song`): the D table; each part rendered twice byte-identical; each slot loads back as its
  part; the chain of §3b for SYNTHWAVE and TRANCE; B denser than A in all 13 genres; C a new motif; LIFT's lead
  ≥ 12 semitones over A's, denser than B, a fill; BREAK's empty T1, no kick / clap, the roll, the pad filter down
  then open; the chain played on stock code row by row; confirm OCT- leaves all slots and tracks byte-identical;
  SAVE X? writes only that slot; a locked track survives a reroll; labels fit 12 characters. UI lint: 3 new screens.
- Spec v0.4.1 (keymap final): **F3 G3 A3 B3 reroll one track** of the part shown (T1–T3 notes, T4 the drum
  rhythm; a locked track: `T3 LOCKED`; written: `SAVE X?`); **E4–C5 are punch-in effects held with no FX
  button**: REPEAT 1/16, LPF, HPF, TAPE STOP, REV THROW, ECHO THROW; **FX held + F3–B3 mute T1–T4** while held
  (decided at the key's down); **C#5 CLEAR**, **D#5 DUCK**; FX LATCH toggles them; K1–K4 with FX held are the FX
  macros; D5–F5 do nothing; leaving SONG turns everything off. Routed through perform.c's `perf_key_press` with a
  SONG map (`perf_song`, `sg_fxmask`, seq.c `SG_FX`): no new DSP, buffers or storage. Held mutes are master-side
  (never `P_MUTE`, never saved) and never unmute a chain `-` track or change the chain or its clock. Tests 19–29.
  Host CPU proxy for test 30 (`tests/song_cpu_probe.c`: a TRANCE 42 BREAK song played through its 8-row chain on
  the host, cc -O2 x86-64, best of 5 runs, a relative figure, not the FM-1's): no effect 174.7 ns/sample, REPEAT
  1/16 alone 195.7, REPEAT 1/16 + ECHO THROW + DUCK 213.2 (a later rerun: 179.0 / 195.0 / 206.1); the same DSP
  path as PERFORM; no sample at full scale. The device's CPU / PEAK / LATE (test 30) are measured on hardware.
- After review of 52fe595: in SONG, K1–K4 with FX held are the FX macros and snap back to 0 when FX is let go,
  as in the FX layer (FX LATCH: they stay until CLEAR). Test 28 now holds the mutes through the keys (the ISR's
  path, FX held at the key's down) in INTRO and OUTRO: T3, which the row plays, is silenced while held and back
  on release; T1, which the row silences, never sounds, held, released or untouched; the chain and its rows'
  timing match a run without mutes.
- Test map (tests/forge_test.c `check` labels carry the spec's numbers): spec v0.3 tests 1–11 in `test_song`
  (labelled 1–11); spec v0.4.1 15–18 in `test_song_fix`; 19–29 in `test_song_fx`, plus the FX-knob check; 30 is the
  device test, with `tests/song_cpu_probe.c` as its host proxy.
- Review of PR #1, before merge: GENERATE checks the save can happen before writing anything, and if a slot is
  refused halfway it puts the chain back and says `SAVE FAILED X`; turning DENSITY or EVOLVE keeps the song written
  (only SEED or GENRE make a new one), so a reroll still asks `SAVE X?`; a refused `SAVE X?` (playing: `STOP TO
  SAVE`) keeps the prompt up; BREAK's pad is one loop per section (past 64 steps: 1/8 at half the steps), so its
  filter opens only in the section's last bar, with patterns of a bar or less opening over their second half.

## 0.12

From a question on the instrument: F5 on a pad track wrote a bass-like line, because the STYLE alone set the register.

- **F5 reads the role of the track's sound**, its category (the PRESETS list's BASS, PAD, LEAD, ...; a user preset's
  own, a FORGE PACK sound its base preset's) and fits the style to it:
  - **BASS**: single notes in the octave of C2 near the root; MELODIC, TRANCE and AMBIENT become root-fifth-octave
    lines (no arcs, arpeggios or wide leaps, no octave jump over the octave). Header: `BASS 42 BERLIN`.
  - **PAD**: a chord on the bar's 1 and 9 only, held with ties, moving a scale step at a time, the root's chord voiced
    between F#3 and F4; no slides, ratchets or octave jumps; EVOLVE drops chords but never adds one. `PAD 42 BERLIN`.
  - **LEAD, PLUCK, KEYS, FX, OTHER**: the style exactly as in 0.11 (the 144 pinned seeds are unchanged).
- T1's default sound (ANALOG ACID) is a BASS, so F5 on a fresh T1 now writes a bass line.
- STARTER and DRUM tracks are unchanged (STARTER's 135 pinned renders too).
- New test: every style x 4 scale-and-root sets x EVOLVE 0..4 x 20 seeds on a lead, a bass and a pad sound (9,600
  renders): all notes in the scale, basses single notes never over B4 and 70 % or more under C4, pad chords only on
  1 and 9, 3 notes or more, A2..E6, held 4 steps or more on average, no ratchet or slide.

## 0.11

- **The FORGE ONE logo on the boot splash**: one flame (red, ember, gold) on an anvil inside the UI's 270-degree dial,
  four steps cut in the anvil's face (the four tracks, the first lit), a cyan spark flying off; the wordmark
  "forge one" under it ("one" in ember), then "0.11 by Khantipol" and "on Felucca v1.6.1 by Hügelton". The same
  family as Forge Five's (the dial, the monoline lettering, after Clipper's tools/gen_logo.py) with FORGE ONE's
  own mark. It is always drawn in the FORGE palette's colours, whatever palette is chosen.
- `tools/gen_fo_logo.py` makes it at build time (`build/gen/fo_logo.h`, a 4.4 KB RLE of 16 colours) and the PNGs in
  `assets/forge-one/` (logo, icon, the splash as the device shows it).
- Decided, not done: the review's physically armed update mode (0.9.1). FORGE ONE keeps upstream's update path.

## 0.10

- **The FORGE palette**, FORGE ONE's own look and the default on a fresh setting: deep indigo night (BG 10 12 26),
  a slate surface, warm paper text, ember THEME (values, curves, steps: 255 164 56) and a cool cyan ACCENT for the
  one hot thing (cursor, playhead, the selected track: 86 222 214). Appended after MONO as id 10, so every saved
  setting keeps its palette; MENU > DISPLAY > COLOR steps to it after MONO. It passes `gen_ui_palettes.py`'s 21
  contrast checks with room (body text 16:1, labels 8:1, values 10:1) and the UI lint over all 243 screens.
- The web editor reads palette names from the device, so it shows FORGE as it is; CUSTOM moves to id 11
  (EDITOR_PROTOCOL.md). Stock Felucca's ids 0..9 are unchanged.

## 0.9.3

- The splash reads FORGE ONE / 0.9.3 by Khantipol / on Felucca v1.6.1 / by Hügelton, as ABOUT does.

## 0.9.2

First feedback from the instrument: "Choir Ahh: too loud".

- **A pack sound can set its track's LEVEL** (`lvl` in `forge_sound_t`, 0 = left alone; OCT− restores it with the
  rest). The eight pads use it: CHOIR PAD at 88 (−12 dB), the others at 96–98 (−7 to −8 dB). Measured on the host
  (2 s of each starter), CHOIR's pad peak fell from 61 k to 24 k, EQUI's from 57 k to 23 k, in line with the basses.
- **CHOIR AAH is a pack sound now**: the factory VOICE preset as it is, at −12 dB; ACID HOUSE JACK and JARRE EQUI use
  it (41 pack sounds). Starters that use other factory pads untouched (ROLL, STRNG, ELECT, FLOOR): say so if they are
  loud too.
- The guides say which sounds set LEVEL and that PAN / MUTE still never change.

## 0.9.1

Beam's third review (of 0.8), six findings, all taken:

- **GLO's C4 UNMUTE ALL now clears PERFORM's latched solos and a waiting QUANT change too** (it only cleared the
  mutes, while PERFORM's notes said it cleared the solos). DUCK is an effect and stays until CLEAR, OCT- or FX LATCH
  off; the guide says so.
- **A queued QUANT change no longer survives STOP**: it goes in at once on STOP (as a change made while stopped
  does) and on QUANT switched to NOW ("AT ONCE" now means it); a project loaded drops it. Before, a mute queued just
  before STOP went in after the next PLAY. Four regressions in forge_test (STOP, QUANT OFF, UNMUTE ALL, project load).
- **Build: Pillow's RAQM layout engine is checked** before the font raster runs (`forge_build.sh` stops with the
  install line, CI fails in the packages step); Pillow 10+ and fontTools 4.40+ are the documented and installed
  minimum. Without RAQM, Pillow fell back to basic layout and the font metrics differed silently.
- **BUILDING.md is FORGE ONE's**: it clones the fork's `forge` branch, describes FORGE ONE's artifacts and release
  assets, and the command-line install (upstream's web installer installs Felucca, not FORGE ONE).
- **CI uploads installable firmware only from a green run** (build and host tests passed); a failed run uploads its
  logs as `forge-one-FAILED-<sha>`. The artifact is now the distribution folder `tools/forge_dist.sh` packs:
  the package, the app, the loader, `SHA256SUMS`, `MANIFEST.txt` (version, commit, run, engines commit, build flags,
  test result) and the licence texts.
- **A release carries the licence texts and the whole source**: `forge-one-<tag>-dist.zip` (LICENSE, LICENSING.md,
  LICENSES/, the sample attribution) and `forge-one-<tag>-source.tar.gz` with the engines submodule's files, which
  GitHub's own archive leaves out.
- **The toolchain download is digest-checked**: `tools/get_toolchain.sh` saves the archive, prints its SHA-256 and
  extracts only when it is the pinned one (`JIELI_TOOLCHAIN_SHA256` overrides; `skip` for an unknown archive). The
  pin is jieli-linux-toolchains-20260730.1 (26027508 B), from CI's fetch of 22 Oct 2026.
- Not done, by design: a physically armed update mode (the review's USB SysEx point). It is upstream's update path,
  `fm1_install.py` and the web installer depend on it, and a confirmation on the instrument would change how every
  install works. Decided (23 Oct 2026): not for FORGE ONE.

## 0.9

- **FORGE PACK: 40 sounds for the starters.** 11 basses (BERLIN SEQ, ACID 303, ACID SQUARE, REESE, DUB SUB, …), 15
  leads, stabs and keys (SUPERSAW, OXY LEAD, DUB CHORD, HOUSE PIANO, HOUSE ORGAN, VOX CHANT, …), 8 pads (STRING MACH,
  BERLIN PAD, CHOIR PAD, GLASS PAD, …) and 6 drum kits (BOOM, HOUSE, BREAK, RHYTHM BOX, TECHNO, DUB). Each is a factory
  preset with its own settings on top (oscillator and filter, envelope, voice, glide, unison, sends, LFO), so nothing
  is added to the factory lists: projects, the Felucca web editor and stock Felucca see ordinary edited sounds.
- 42 of the 45 starters now use pack sounds (78 of the 180 parts); the rest keep factory presets where they fit.
- A new test plays every starter for 2 s and checks all four tracks sound and the master never sits at full scale.
  Patterns are unchanged (the sounds do not touch the seeds).
- Designed without listening on hardware: levels between tracks vary, and each sound is a starting point.

## 0.8

- **45 starters in 13 genres.** STARTER's K4 is now GENRE: Berlin School, Detroit, Dub Techno, Melodic Techno,
  Acid House, Acid Techno, Trance, Deep House, Synthwave, Breakbeat, Ambient, Lo-fi and Jarre. While STARTER is open,
  black keys 1–4 pick the genre's variation (3 or 4 each); the screen shows them as a row of cells and the keys light.
- Each starter has its own four presets, part patterns, LENs, scale, tempo, drum beat and swing. Six new drum beats:
  deep house, breakbeat, lo-fi, synthwave, an early rhythm box and electro. Deep House, Lo-fi and Breakbeat swing.
- At most one heavy engine (GRAIN or PHYS) per starter, for the CPU. 0.6's AMBIENT (now AMBIENT CLOUD) uses FM6 BELL
  instead of PHYS KALIMBA beside its GRAIN pad.
- **ABOUT** (MENU > SYSTEM) now reads "FORGE 0.8 by Khantipol", then Felucca's version and credit, then FORGE ONE's
  repository; README and CREDITS.md credit FORGE ONE to Khantipol (@kkasemsa).
- **Patterns changed:** each starter now has its own seed streams, so a seed gives a different song than in 0.6/0.7.
  The 135 starter seeds are pinned again; FORGE's own 144 are unchanged.

## 0.7

- **PERFORM: double-tap FX.** The FX layer locked becomes a live-performance mode: black keys 1–4 latch MUTE
  (with OCT+ held, SOLO), black keys 5–8 are four scenes of mutes and solos (tap to recall, hold 0.6 s to store),
  C#5 QUANT (changes wait for the next bar, put in before the bar's first step plays), D#5 DUCK, F#5 CLEAR.
  K1–K4 are the track levels; OCT+ tapped alone switches them to the FX macros. The white keys are the FX map as
  always. Scenes and QUANT are kept with the settings. Holding FX works as before.
- **Throws that flow back.** REV THROW and ECHO THROW now glide back over about 0.75 s when let go, and two new
  throws join them on the free keys: **F5 CUTOFF THROW** (the master low-pass closes) and **G5 DIST THROW** (every
  track's DIST at least 100). 16 effects on the FX map.
- **DUCK**, a sidechain pump: while on, each kick of a DRUM track dips the other tracks by 12 dB, back over an 8th.
  On PERFORM's D#5, or assign it to a white key.
- Felucca's own tests updated where they checked F5 G5 empty, the fast throw release, and the FX lock's knobs.

## 0.6

- **STARTER on SEQ TOOLS G5.** One press loads a factory sound on each track and writes a pattern for each from one
  seed: T1 BASS, T2 LEAD, T3 PAD (chords), T4 DRUM. Eight starters with the FORGE style names, each with its own four
  sounds, LENs, tempo and scale (used when the selected track's SCALE is CHR). All four take the selected track's ROOT.
- K1 SEED, K2 DENSITY (50–150 % of each part's own), K3 EVOLVE, K4 STARTER (wraps, loads its sounds and tempo);
  C4 / D4 reroll the notes / rhythms of all four. OCT+ or G5 again keeps it; OCT− puts all four tracks back exactly
  (sound, parameters, steps, FM6 patch, automation, tempo). LEVEL, PAN and MUTE are never changed.
- The SEQ TOOLS map has a G5 STARTER cell (on a DRUM track, F and G as letter cells at the end of the lanes row).
  STARTER's screen shows each track's preset and first 16 steps.
- FORGE itself is unchanged: its 144 pinned seeds write the same steps. 24 starter seeds are pinned too.

## 0.5.1

- **New name: FORGE ONE** (was Felucca-FORGE). The start-up screen and MENU > ABOUT show FORGE ONE with its
  version and Felucca's credit; the firmware file is `forge-one.fwsc`; the repository is
  [kkasemsa/FORGE-ONE](https://github.com/kkasemsa/FORGE-ONE). The editor protocol still reports Felucca, so the
  Felucca web editor and backups work as before.
- **Drum fill never replaces the kick on step 1.** At LEN 2 or 3 with EVOLVE 2 or more, the fill could land on step 1
  and put a snare in place of the kick. The fill now needs a motif of at least 4 steps and never touches step 1.
- **STYLE wraps** like SEED: past AMBIENT comes BERLIN, and back. Changing STYLE also sets the C4 / D4 counts back to 0,
  as a new SEED does.
- **Long drum patterns are easier to read.** At LEN 33 to 64 each step shows two columns of 3 px marks (kick, snare,
  clap, tom on the left; hats, rim, bell on the right) instead of eight 1 px strips.
- **Step 1 is always ROOT.** Every SCALE contains its root, so this already held; a new test checks all 16 scales,
  12 roots and 8 styles, and the code would fall back to the nearest scale note if a scale ever lacked its root.
- Code clean-up: an unused value removed, comments and the test-suite label brought up to date.
- Same seeds, same patterns: all 144 pinned seeds are unchanged.

## 0.5

- **FORGE on a DRUM track.** F5 is offered on DRUM tracks too, as a small F cell at the right end of the lanes row
  (the lane cells sit 1 px apart, DOUBLE and HALF read Dx2 and E/2 there). The same SEED, DENSITY, EVOLVE and the
  eight style names make a beat: each style has core hits per lane (kept from DENSITY 50 % up) and optional ones
  (each with DENSITY's chance); over 75 % extra 16th hats and rim ghosts. The kick on step 1 is always there; kicks on
  1 and 9 are accented, snare / clap on 2 and 4 half the time. EVOLVE moves hats and adds ghost snares / rims with a
  chance on empty off-beats (1 up), ends the last bar in a snare-roll fill on a 4:4 condition (2 up), and moves
  kicks off the beat (3 up); hats ratchet at the style's rate. C4 rerolls the low lanes (LOW), D4 the hats (HATS).
  The preview shows each step as eight lane strips. DENSITY remembers the beat's default when FORGE opens on a
  DRUM track after a synth track and back.
- **Pinned seeds** gain 48 drum patterns (8 styles × 2 settings × 3 seeds). The 96 melodic ones are unchanged.

## 0.4

- **Velocity follows the beat.** Notes on the beat are louder, off-beat 16ths softer, with a little variation
  (40–124). Each style has its own range: ACID the widest, DUB the gentlest. Every copy of the motif has the same
  velocities, so EVOLVE 0 is still a pure loop. Accents still play at 127.
- **Reroll notes or rhythm.** While FORGE is open, **C4** gives new notes on the same rhythm and **D4** a new rhythm
  (each step that still sounds keeps its pitch). The screen shows NOTES and RHYTHM counts under the steps; turning
  K1 SEED sets them back to 0. Each count is part of the pattern: SEED 42 with NOTES 3 is always the same.
- ⚠ **Every pinned seed changes in its velocities only.** With the C4 / D4 counts at 0, the notes, rhythm, chances,
  conditions and ratchets are exactly 0.3's (checked by hashing all 96 pinned patterns with VEL set back to 96).

## 0.3

- **Turning the knobs while playing is safe.** Each step is now built whole and then swapped in with the audio
  interrupt briefly held off, so the sequencer never plays a half-written step. Up to 0.2 every turn first put the old
  steps back and then wrote the new ones over them in place. Cancel (OCT−) puts the old steps back the same way.
- **Chords work in every scale.** Chord notes (DETROIT, DUB, AMBIENT) and the TRANCE and SYNTH arpeggios are built
  from real intervals (third, fifth, seventh, octave above the note) kept inside the scale. Up to 0.2 they counted scale
  notes, which gave clusters in CHR (C–D–E) and wrong fifths in pentatonic and whole-tone scales. AMBIENT's wide leaps
  are now fourths and fifths by interval too.
- **Pinned seeds.** `tests/forge_test.c` checks 96 patterns (8 styles × 4 settings × 3 seeds) against
  `tests/forge_golden.txt`, so a seed keeps its pattern unless a release says otherwise.
- ⚠ **Some seeds give different patterns than in 0.2** (checked against the 0.2 code):
  - unchanged: BERLIN, MELODIC and ACID in every scale; DETROIT, DUB and SYNTH in seven-note scales (MAJ, MIN …)
  - changed: DETROIT, DUB and SYNTH in CHR, pentatonic and other scales without seven notes; TRANCE and AMBIENT in
    most scales

  Patterns already applied to your projects are ordinary steps and do not change.

## 0.2

- FORGE on SEQ TOOLS F5 for melodic tracks: K1 SEED, K2 DENSITY, K3 EVOLVE, K4 STYLE; OCT+ applies, OCT− cancels,
  SAVE held undoes.
- Eight styles: BERLIN, DETROIT, DUB, MELODIC, ACID, TRANCE, SYNTH, AMBIENT.
- Only writes fields Felucca 1.6 projects already hold: no format change.

## 0.1

- The F5 FORGE cell in SEQ TOOLS (a placeholder), and the GitHub Actions build and host tests.
