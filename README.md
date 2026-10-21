# Felucca-FORGE — Read Me First

Felucca-FORGE is custom firmware for the **M-VAVE FM-1**. It is Felucca 1.6.1 by Hügelton Instruments, with one
addition: **FORGE**, a seeded motif generator for melodic tracks on the SEQ TOOLS **F5** key.
Everything else works exactly as in Felucca 1.6.1.

| | |
| --- | --- |
| Based on | [Felucca](https://github.com/hugelton/Felucca) 1.6.1 (GPL-3.0-only) by Leo Kuroshita, Hügelton Instruments |
| Adds | FORGE 0.2: SEED, DENSITY, EVOLVE and 8 STYLES on SEQ TOOLS F5 |
| Branch | [`forge`](https://github.com/kkasemsa/Felucca-FORGE/tree/forge) (`main` is an untouched copy of upstream) |
| Status | Firmware builds and host tests pass in CI. **Not yet tested on a real FM-1.** |

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

1. Open the [Releases](https://github.com/kkasemsa/Felucca-FORGE/releases) page and download `felucca.fwsc` from the newest release.
2. If there is no release yet, open [Actions](https://github.com/kkasemsa/Felucca-FORGE/actions), click the newest
   green **Build** run on `forge`, and download the `felucca-forge-…` artifact. It is a zip, and `felucca.fwsc` is
   inside. You need to be signed in to GitHub to download artifacts.

## Install (macOS)

The official web installer only installs stock Felucca, so install FORGE with Felucca's own command-line installer.

1. Quit Ableton Live and every other app that uses MIDI. Connect the FM-1 with a USB **data** cable and turn it on.
2. In Terminal, make a Python environment with the two MIDI libraries the installer needs. Do this once:
    - `python3 -m venv ~/fm1-env`
    - `~/fm1-env/bin/pip install mido python-rtmidi`
3. Download the installer script
   [`tools/fm1_install.py`](https://raw.githubusercontent.com/kkasemsa/Felucca-FORGE/forge/tools/fm1_install.py)
   into the folder that holds `felucca.fwsc`.
4. Run it from that folder: `~/fm1-env/bin/python fm1_install.py felucca.fwsc`
   It asks before writing. Don't unplug the cable or let the Mac sleep until it says the install is done.
   The FM-1 restarts on its own.
5. Check it worked: hold **SEQ** on a melodic track. The SEQ TOOLS map should show a **FORGE** cell for key F5.

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
| [Felucca-FORGE User Guide](docs/USER-GUIDE.md) ([PDF](docs/pdf/Felucca-FORGE-User-Guide.pdf)) | The whole instrument: panel, pages, sequencer, engines, effects, MIDI, menu, and FORGE |
| [Felucca README](README-FELUCCA.md) | Upstream's own README, unchanged |
| [BUILDING.md](BUILDING.md) | Building the firmware yourself (CI does it on every push) |

## Credits and licence

Felucca is by Leo Kuroshita ([@kurogedelic](https://github.com/kurogedelic)), Hügelton Instruments, and its contributors:
see [CREDITS.md](CREDITS.md). Its full manual is the [Felucca wiki](https://github.com/hugelton/Felucca/wiki).
On 16 October 2026 the maintainer stepped back from active development; Felucca stays available with occasional maintenance.

FORGE and this fork are free software under [GPL-3.0-only](LICENSE), like Felucca. The bundled fonts, ported DSP and
SDK files keep their own licences ([LICENSES/](LICENSES/), [LICENSING.md](LICENSING.md)).
M-VAVE and FM-1 are trademarks of their respective owners. Neither Felucca nor this fork is affiliated with or endorsed by them.
