# FORGE changelog

Newest first. FORGE is the SEQ TOOLS F5 motif generator in FORGE ONE (based on Felucca 1.6.1); STARTER (G5, 0.6) is its song starter.

| Version | Date | Theme |
| --- | --- | --- |
| 0.6 | 22 Oct 2026 | STARTER: a song start on all four tracks, sounds included |
| 0.5.1 | 22 Oct 2026 | Fixes from the second code review |
| 0.5 | 21 Oct 2026 | FORGE on DRUM tracks: beats in the eight styles |
| 0.4 | 21 Oct 2026 | Velocity by the beat; C4 / D4 reroll notes or rhythm |
| 0.3 | 21 Oct 2026 | Safe live turns, chords in every scale, pinned seeds |
| 0.2 | 21 Oct 2026 | The generator: SEED, DENSITY, EVOLVE, 8 styles |
| 0.1 | 21 Oct 2026 | Scaffold: F5 FORGE cell, CI build |

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
