# FORGE changelog

Newest first. FORGE is the SEQ TOOLS F5 motif generator in Felucca-FORGE (based on Felucca 1.6.1).

| Version | Date | Theme |
| --- | --- | --- |
| 0.4 | 21 Oct 2026 | Velocity by the beat; C4 / D4 reroll notes or rhythm |
| 0.3 | 21 Oct 2026 | Safe live turns, chords in every scale, pinned seeds |
| 0.2 | 21 Oct 2026 | The generator: SEED, DENSITY, EVOLVE, 8 styles |
| 0.1 | 21 Oct 2026 | Scaffold: F5 FORGE cell, CI build |

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
