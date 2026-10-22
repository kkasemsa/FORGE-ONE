# Building FORGE ONE

FORGE ONE is Felucca 1.6.1 plus FORGE, STARTER and PERFORM, on the `forge` branch of
<https://github.com/kkasemsa/FORGE-ONE> (`main` there is upstream Felucca, unchanged). The quickest way is
`tools/forge_build.sh` (macOS or Linux: it checks the prerequisites, fetches the toolchain and the SDK files,
builds, runs the tests, and with `--flash` installs); what follows is the same by hand.

The build makes three files in `build/`:

| File | What |
| --- | --- |
| `felucca.bin` | the firmware app |
| `loader/ota.bin` | the update loader |
| `felucca.fwsc` | the installable package (app + loader); FORGE ONE's CI and `tools/forge_build.sh` also copy it to `forge-one.fwsc` |

## Get the source

Five engines are in a git submodule (`firmware/engines/`, [Felucca-Engines](https://github.com/hugelton/Felucca-Engines),
MIT), so clone with `--recurse-submodules`:

```
git clone --recurse-submodules -b forge https://github.com/kkasemsa/FORGE-ONE.git
cd FORGE-ONE
```

In a clone made without it, run `git submodule update --init` once. GitHub's "Download ZIP" and the source archives
it adds to a release leave `firmware/engines/` empty; a FORGE ONE release (a `v*-forge.*` tag) also carries
`forge-one-<tag>-source.tar.gz`, the whole source with the submodule's files in it, and `forge-one-<tag>-dist.zip`,
what `tools/forge_dist.sh` packs (the package, the app, the loader, `SHA256SUMS`, `MANIFEST.txt`, the licence texts).
The build stops with "firmware/engines is empty" when the submodule's files are missing. Upstream Felucca is
<https://github.com/hugelton/Felucca>.

## Prerequisites (macOS)

- Python 3.10 or later with Pillow 10+ and fontTools 4.40+: `pip3 install 'Pillow>=10' 'fonttools>=4.40'` (the UI
  font and icons are rasterised at build time). The font raster shapes text with Pillow's RAQM layout engine
  (OpenType features; `tools/aa_raster.py`): Pillow's wheels carry it but load libfribidi at run time, so
  `brew install fribidi` first (Linux: `apt install libfribidi0`). Check with
  `python3 -c 'import PIL.features as f; print(f.check("raqm"))'` (must print `True`; `forge_build.sh` stops
  without it rather than building with the basic layout, whose metrics differ).
- Docker Desktop. The JieLi toolchain is Linux x86-64 only; the build runs each tool in a
  `linux/amd64` `debian:bookworm-slim` container (Rosetta on Apple silicon). Keep the source
  tree in a folder Docker can share, e.g. under `/Users`.
- The JieLi Linux toolchain (clang 4.0.1 for pi32v2, from JieLi's package server):

  ```
  tools/get_toolchain.sh            # installs to ~/.jieli/toolchain
  ```

  The script checks the archive's SHA-256 against the one pinned in it (`PIN_DEFAULT`; `JIELI_TOOLCHAIN_SHA256`
  overrides it, `=skip` extracts an unknown archive and says so). JieLi's server has no versioned URL, so when it
  serves a new archive the script stops and prints its digest: pin it once it is known to be good.

- The JieLi AC79 SDK (Apache-2.0). The package uses three of its files
  (`cpu/wl82/tools/uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`); they are not part of this tree.

  ```
  git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
      https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
  ```

- Node.js (optional, for the web tests).
- Emscripten (optional, `emcc`: the browser emulator, `web/emu/`).

On Linux x86-64 the toolchain runs natively and Docker is not needed.

## Build

```
./build.sh
```

`JIELI_TOOLCHAIN` and `AC79_SDK` override the default locations
(`~/.jieli/toolchain`, `~/fw-AC79_AIoT_SDK`).

FORGE ONE's package keeps Felucca's development identity (`FM-1_900`, version string `v1.6.1`, so the editor and
the installer treat it as 1.6.1) and `FORGE_VERSION` in `firmware/src/core.h` is its own version (the splash and
ABOUT). `tools/forge_dist.sh DIR` packs what a FORGE ONE release ships: `forge-one.fwsc`, the app, the loader,
`SHA256SUMS`, `MANIFEST.txt` (version, commit, build flags, test result), the sample attribution, `LICENSE`,
`LICENSING.md` and `LICENSES/` (the package contains Apache-2.0 SDK files and MIT engines, so the licence texts
travel with it). CI (`.github/workflows/forge-build.yml`) uploads that folder as the artifact of a green run only
(a failed run uploads its logs as `forge-one-FAILED-<sha>`), and on a `v*-forge.*` tag makes the release from it
with the full source tarball.

Upstream's `./build.sh --release 1.0` (identity `FM-1_910`, `build/release-1.0/`) still works but is not what
FORGE ONE ships.

Build options (environment, `0` or `1`; defaults in `firmware/src/felucca.c`, `core.h` and `icons.c`):

| Flag | Default | |
| --- | --- | --- |
| `FELUCCA_FLASH` | 1 | settings, presets and projects in flash |
| `FELUCCA_OTA` | 1 | update entry (needs `FELUCCA_FLASH`) |
| `FELUCCA_CDC` | 1 | USB serial console |
| `FELUCCA_CDC_DEFAULT` | 1 | `0`: the console is built in but left out of USB from boot (as MENU > USB SERIAL OFF) |
| `FELUCCA_UAC` | 1 | USB audio input (the master output, stereo, 44.1 or 48 kHz) |
| `FELUCCA_UAC_48K` | 1 | `0`: the USB audio input at 44.1 kHz only, as up to 1.0.5.2 (`firmware/src/usb.c`) |
| `FELUCCA_UAC_AS_FIRST` | 1 | the audio control header lists the audio streaming interface before the MIDI one, so the USB audio driver of older macOS (up to 15) finds it; `0`: 1.0 to 1.5's order, byte for byte (`firmware/src/usb.c`) |
| `FELUCCA_CPU2` | 1 | the second core is built in (MENU > SYSTEM > 2ND CORE, OFF by default); `0`: none of it, 1.6's one-core code and RAM (`firmware/hal/fm1_cpu1.h`, `firmware/src/fx.c`) |
| `FELUCCA_UART` | 1 | TRS MIDI IN |
| `FELUCCA_SLICE` | 1 | the SLICE engine |
| `FELUCCA_ICONS` | 1 | parameter icons on the knob cards |
| `FELUCCA_FM4` | 0 | the retired DIGITAL engine (4-operator FM) instead of its FM6 conversion |

`FELUCCA_USB_LAYOUT` (`0` to `3`, default `0`) picks other USB descriptor layouts for testing; see
`firmware/src/usb.c`.

## Samples

The CC0 instrument samples that the SAMPLE engine uses are in `assets/samples-cc0/`
(Versilian Studios, see `ATTRIBUTION.txt` there). `tools/fetch_cc0.py` downloads them
again from the source repositories. Without that folder the build still works, with no
instrument sets in the SAMPLE engine and no PIANO in SLICE (the PIANO set's middle C): they play a plain sine
at the note's pitch instead, and the screen says NO SAMPLE.

## Browser emulator

```
./build.sh
web/emu/build.sh
python3 -m http.server -d build/emu 8790     # open http://localhost:8790/
```

`web/emu/felucca_web.c` builds the firmware's sources as the host tests do, against a simulated FM-1, and
Emscripten compiles it to WebAssembly (`build/emu/felucca.wasm`, with `index.html`, `worklet.js` and the font in `fonts/`). The page
keeps the flash's storage sectors (projects, user presets, settings) in the browser. Not emulated: USB (audio,
MIDI out, the editor, updates), TRS MIDI and the user sample slots. A build of it is on the site:
<https://hugelton.github.io/Felucca/webapp/try/>.

## Tests

```
tests/run_tests.sh
```

Runs the host tests and, with Node.js, the web page tests (and, with Emscripten too, the browser emulator's). Run it after `./build.sh`
(it uses `build/` and needs `AC79_SDK` set as for the build). The suites cover flash storage,
user presets, projects of every format, backup, the keys and knobs, MIDI (USB, TRS, clock,
control), USB audio, the update entry and loader, the command-line installer, the UI (the real
drawing code against stubs: every screen in every palette is rendered and checked for clipped or
overlapping text; PNGs land in `build/ui_new/`), every engine (DRUM, NOISE, PHYS, FM6, SLICE, the
DIGITAL conversion), the chord keys, the modulation matrix, the FX layer, the reverbs, the SLICER
and swing. With `DAISYSP` pointing at a DaisySP checkout, the PHYS models are also compared with
their floating-point originals; without it that test is skipped. Felucca-Engines' own test
(`firmware/engines/tests/run.sh`) runs too, and its standalone host is built once more with Felucca's tables
against the same golden renders.

The regression suite (`tests/regress.c`) renders every engine and preset and compares a
hash of each render with `tests/golden.txt`; it also checks levels, voices and the CPU
cost (`tests/cpu_baseline.txt`, `tests/target_budget.txt`). After an intended change of
the sound, `GOLDEN_UPDATE=1 sh tests/run_tests.sh` rewrites the hashes; `BUDGET_UPDATE=1`
does the same for the cost files.

## Install

Upstream's web installer (<https://hugelton.github.io/Felucca/webapp/installer/>) installs released Felucca, not
FORGE ONE. For FORGE ONE use the command line (needs `pip3 install mido python-rtmidi`; `forge_build.sh --flash`
does the same), with the package from a green CI run's artifact or your own build:

```
python3 tools/fm1_install.py forge-one.fwsc          # (or build/felucca.fwsc: the same file)
python3 tools/fm1_install.py --info          # identity of the connected FM-1
```

Or, to install your own build from the web installer, make a local copy of the site and open it from `localhost`
(Web MIDI needs a secure context):

```
python3 web/make_site.py build/felucca.fwsc dev /tmp/felucca-site
cd /tmp/felucca-site && python3 -m http.server 8000
# open http://localhost:8000/webapp/installer/
```

Installing firmware is at your own risk. If an install fails and the FM-1 no longer
starts, recovery needs [FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter).
