# FORGE

FORGE is a seeded motif generator for melodic tracks, added to Felucca's SEQ TOOLS layer on the **F5** key.
It writes one idea in the track's scale and lets it evolve across the pattern and from loop to loop.

## Use it

1. Pick a melodic track (FORGE is not offered on DRUM tracks). Set its LEN, ROOT and SCALE first.
2. Hold **SEQ** and tap **F5**. The pattern is written at once, and you hear it if the transport is running.
3. Turn the knobs. Each turn rewrites the pattern from the steps as they were before FORGE opened:

| Knob | Parameter | Range | What it does |
| --- | --- | --- | --- |
| K1 | SEED | 0–99 | Picks the pattern. The same seed and settings always give the same steps |
| K2 | DENSITY | 10–100 % | How many of the style's steps sound. Changing it keeps each remaining note's pitch |
| K3 | EVOLVE | 0–4 | 0 repeats the motif exactly. Higher values make later copies drift and add chance or conditions to some notes |
| K4 | STYLE | 8 styles | The rhythm and pitch profile. Choosing a style resets DENSITY to that style's default |

4. **OCT+** (or F5 again, another tool, or letting go of SEQ) applies the pattern. **OCT−** cancels it and puts the old steps back.
5. Changed your mind after applying? Hold **SAVE** to undo the whole FORGE in one step.

The header shows `FORGE <seed> <style>`, so you can note a seed you like and get the pattern back later.

## Styles

| Style | Card | Rhythm | Pitch |
| --- | --- | --- | --- |
| BERLIN | BERLIN | 16th ostinato, 8-step motif | Small steps around the root |
| DETROIT | DETROIT | Syncopated stabs | 4-note scale chords |
| DUB | DUB | Off-beats, long ties | 3-note scale chords, narrow range |
| MELODIC | MELO | 8ths | Rises and falls in an arc |
| ACID | ACID | 16ths, accents, slides | Root pedal, octave jumps, a few ratchets |
| TRANCE | TRANCE | Rolling 16ths, 8-step motif | Arpeggios of the scale's triad |
| SYNTH | SYNTH | 8ths, 8-step motif | Root, fifth and octave |
| AMBIENT | AMBI | Very sparse, long ties | Wide intervals, some chords |

MELODIC and AMBIENT are shortened on the knob card so they fit, but the header shows the full name.

## What it writes

FORGE writes steps 1 to LEN of the track and leaves steps past LEN alone. It only uses fields that Felucca 1.6
projects already store: notes (up to 4), NOTE/TIE/REST, accent, slide, RATCH, and CHANCE or conditions (2:2, 3:4, 2:4, 4:4).
So projects made with FORGE open on stock Felucca 1.6 and later. Every note velocity is 96, and step 1 is always the root.

Notes are placed at the style's octave: ACID starts around C2, BERLIN, DUB and SYNTH around C3, and the others around C4.
Use the track's TRN or the octave buttons to move them.

## Build

Builds run on GitHub Actions for every push to the `forge` branch (`.github/workflows/forge-build.yml`).
Download `felucca.fwsc` from the run's artifacts, or from a Release made by a `v*-forge.*` tag.
The code is in `firmware/src/ui_forge.c`, and the tests are in `tests/forge_test.c`.
Installing firmware is at your own risk. The web installer's **Return to official V15** takes you back.
