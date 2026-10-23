# FORGE ONE — Read Me First

FORGE ONE is custom firmware for the **M-VAVE FM-1**. It is Felucca 1.6.1 by Hügelton Instruments, with one
addition: **FORGE**, a seeded generator of melodies and beats on the SEQ TOOLS **F5** key, and its song starter,
**STARTER**, on **G5**, and a live **PERFORM** mode on the FX layer.
Everything else works exactly as in Felucca 1.6.1. FORGE ONE is an independent fork, not made or endorsed by
Hügelton Instruments; it was called Felucca-FORGE until 0.5.1.

| | |
| --- | --- |
| Based on | [Felucca](https://github.com/hugelton/Felucca) 1.6.1 (GPL-3.0-only) by Leo Kuroshita, Hügelton Instruments |
| Adds | FORGE 0.10: SEED, DENSITY, EVOLVE and 8 STYLES on SEQ TOOLS F5, melodies and beats; STARTER on G5, 45 song starters in 13 genres with the 41-sound FORGE PACK; PERFORM (FX double tap) for live mutes, solos, scenes and throws; the FORGE palette ([changelog](CHANGELOG-FORGE.md)) |
| Branch | [`forge`](https://github.com/kkasemsa/FORGE-ONE/tree/forge) (`main` is an untouched copy of upstream) |
| Status | Builds and host tests pass in CI on every push. Installed and running on the author's FM-1 since 0.6; PERFORM (0.7) and the 45 starters (0.8) are not yet played on hardware. |

## Before you install

- **Installing firmware is at your own risk.** If an install is interrupted, the FM-1 can be left unable to start.
  You can bring it back (see [If the FM-1 does not start](#if-the-fm-1-does-not-start)), but that may need extra hardware.
- **Back up first.** The [Felucca web editor](https://hugelton.github.io/Felucca/webapp/editor/) can make a full backup
  of projects, presets and settings. The backup leaves out the FM6 voice bank, so keep your `.syx` file.
- **Your projects stay compatible.** FORGE only writes step data that Felucca 1.6 already stores, so projects made
  with FORGE open on stock Felucca 1.6 and 1.6.1.
- New to Felucca? Install stock Felucca first with the [official web installer](https://hugelton.github.io/Felucca/)
  and get to know it, or [try it in your browser](https://hugelton.github.io/Felucca/webapp/try/).

## Get the firmware

1. Open the [Releases](https://github.com/kkasemsa/FORGE-ONE/releases) page and download `forge-one.fwsc` from the newest release.
2. If there is no release yet, open [Actions](https://github.com/kkasemsa/FORGE-ONE/actions), click the newest
   green **Build** run on `forge`, and download the `forge-one-…` artifact. It is a zip, and `forge-one.fwsc` is
   inside. You need to be signed in to GitHub to download artifacts.

## Install (macOS)

The official web installer only installs stock Felucca, so install FORGE with Felucca's own command-line installer.

1. Quit Ableton Live and every other app that uses MIDI. Connect the FM-1 with a USB **data** cable and turn it on.
2. In Terminal, make a Python environment with the two MIDI libraries the installer needs. Do this once:
    - `python3 -m venv ~/fm1-env`
    - `~/fm1-env/bin/pip install mido python-rtmidi`
3. Download the installer script
   [`tools/fm1_install.py`](https://raw.githubusercontent.com/kkasemsa/FORGE-ONE/forge/tools/fm1_install.py)
   into the folder that holds `forge-one.fwsc`.
4. Run it from that folder: `~/fm1-env/bin/python fm1_install.py forge-one.fwsc`
   It asks before writing. Don't unplug the cable or let the Mac sleep until it says the install is done.
   The FM-1 restarts on its own.
5. Check it worked: the start-up screen says **FORGE ONE**, and when you hold **SEQ** the SEQ TOOLS map shows **FORGE** on key F5 and **STARTER** on G5.

## Build it yourself (optional)

GitHub builds every push, so you only need this to build your own changes. On a Mac with Docker Desktop running:

```
git clone --recurse-submodules -b forge https://github.com/kkasemsa/FORGE-ONE.git ~/FORGE-ONE
cd ~/FORGE-ONE
tools/forge_build.sh            # set up and build build/forge-one.fwsc
tools/forge_build.sh --flash    # build and install on the FM-1
```

The first run downloads JieLi's toolchain and three AC79 SDK files and makes a Python environment in `~/fm1-env`;
later runs only build. `--test` also runs the host tests (about 10 minutes). See [BUILDING.md](BUILDING.md) for the details.

## Go back to stock

- **To stock Felucca:** use the [official web installer](https://hugelton.github.io/Felucca/) in Chrome or Edge.
- **To the M-VAVE firmware:** use the web installer's **Return to official V15**, or M-VAVE's own updater.

## If the FM-1 does not start

- **If the install stopped part-way**, run the same install command again: it finishes the write.
- **If the screen stays black**, check whether your Mac sees a USB device named **WL80UBOOT**. That is the chip's own
  boot mode, and the FM-1 can be recovered. The command-line installer cannot reach it:

1. Try another USB data cable, quit every MIDI app, and run the [official web installer](https://hugelton.github.io/Felucca/) again.
2. If it stays in boot mode, [FM-1 Transporter](https://github.com/kurogedelic/FM-1-transporter) can write the flash
   through a Seeed XIAO RP2040 wired to the FM-1's USB lines. Back up the flash first, then write the official firmware.

## The documents

| Document | What it covers |
| --- | --- |
| **Read Me First** (this page) | What this is, the risks, getting it on your FM-1, getting back |
| [FORGE User Guide](docs/FORGE-USER-GUIDE.md) ([PDF](docs/pdf/FORGE-User-Guide.pdf)) | FORGE only: controls, the 8 styles, EVOLVE, recipes |
| [FORGE ONE User Guide](docs/USER-GUIDE.md) ([PDF](docs/pdf/FORGE-ONE-User-Guide.pdf)) | The whole instrument: panel, pages, sequencer, engines, effects, MIDI, menu, and FORGE |
| [Tutorial (ภาษาไทย)](docs/TUTORIAL-TH.md) ([PDF](docs/pdf/FORGE-ONE-Tutorial-TH.pdf)) | A first 4-track jam: FORGE on tracks 1–3, drums by hand on track 4 |
| [Felucca README](README-FELUCCA.md) | Upstream's own README, unchanged |
| [BUILDING.md](BUILDING.md) | Building the firmware yourself (CI does it on every push) |

## Credits and licence

FORGE ONE is by Khantipol ([@kkasemsa](https://github.com/kkasemsa)): FORGE, STARTER, PERFORM and this fork's
documents. It is built on Felucca:

Felucca is by Leo Kuroshita ([@kurogedelic](https://github.com/kurogedelic)), Hügelton Instruments, and its contributors:
see [CREDITS.md](CREDITS.md). Its full manual is the [Felucca wiki](https://github.com/hugelton/Felucca/wiki).
On 16 October 2026 the maintainer stepped back from active development; Felucca stays available with occasional maintenance.

FORGE and this fork are free software under [GPL-3.0-only](LICENSE), like Felucca. The bundled fonts, ported DSP and
SDK files keep their own licences ([LICENSES/](LICENSES/), [LICENSING.md](LICENSING.md)).
M-VAVE and FM-1 are trademarks of their respective owners. Neither Felucca nor this fork is affiliated with or endorsed by them.
