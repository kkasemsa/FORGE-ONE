#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 the FORGE ONE contributors
#
# FORGE ONE: build the firmware on your own Mac (or Linux x86-64), from setup to a .fwsc, in one command.
#
#   tools/forge_build.sh            set up what is missing, then build build/forge-one.fwsc
#   tools/forge_build.sh --test     ... and run the host tests (tests/run_tests.sh, about 10 minutes)
#   tools/forge_build.sh --flash    ... and install it on the FM-1 over USB (asks before writing)
#   tools/forge_build.sh --clean    remove build/ first
#
# What it sets up the first time (and leaves alone after):
#   ~/fm1-env                     a Python environment: Pillow fonttools mido (the build), python-rtmidi (--flash)
#   ~/.jieli/toolchain            JieLi's Linux toolchain (tools/get_toolchain.sh, from JieLi's server)
#   ~/fw-AC79_AIoT_SDK            the three AC79 SDK files the package needs (Apache-2.0, from Gitee)
#   firmware/engines              the Felucca-Engines submodule
# On a Mac the toolchain runs in Docker (linux/amd64; Rosetta on Apple silicon): Docker Desktop must be running,
# and this checkout must be in a folder Docker shares (anywhere under /Users is fine).
set -eu

cd "$(dirname "$0")/.."
ROOT=$(pwd)
ENV="${FM1_ENV:-$HOME/fm1-env}"
TC_DEST="${JIELI_DEST:-$HOME/.jieli}"
export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$TC_DEST/toolchain}"
export AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
SDK_TAG="AC79NN_SDK_V1.2.1_2023-12-13"
SDK_URL="https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git"

TEST=0; FLASH=0; CLEAN=0
for a in "$@"; do
    case "$a" in
        --test) TEST=1 ;;
        --flash) FLASH=1 ;;
        --clean) CLEAN=1 ;;
        -h|--help) sed -n '4,19p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "unknown option: $a (try --help)"; exit 2 ;;
    esac
done

step() { printf '\n\033[1;33m== %s\033[0m\n' "$*"; }
ok() { printf '   \033[32mok\033[0m  %s\n' "$*"; }
die() { printf '\n\033[1;31mstop:\033[0m %s\n' "$*"; exit 1; }

# --------------------------------------------------------------------------------------------------- checks ---
step "Checking the machine"
OS=$(uname -s); ARCH=$(uname -m)
command -v git >/dev/null || die "git is missing. On a Mac: xcode-select --install"
command -v python3 >/dev/null || die "python3 is missing. On a Mac: xcode-select --install (or brew install python)"
command -v curl >/dev/null || die "curl is missing"
[ -f firmware/src/ui_forge.c ] || die "this is not a FORGE ONE checkout (no firmware/src/ui_forge.c)"
ok "$OS $ARCH, $(python3 --version 2>&1), git $(git --version | awk '{print $3}')"
if [ "$OS" != Linux ] || [ "$ARCH" != x86_64 ]; then
    command -v docker >/dev/null || die "Docker is missing. Install Docker Desktop: https://www.docker.com/products/docker-desktop/"
    docker info >/dev/null 2>&1 || die "Docker is not running. Open Docker Desktop, wait for it to start, then run this again"
    case "$ROOT" in
        /Users/*|/home/*) ;;
        *) echo "   note: $ROOT may not be shared with Docker; if the build cannot see its files, move the checkout under /Users" ;;
    esac
    ok "Docker is running (the toolchain runs as linux/amd64)"
fi

# --------------------------------------------------------------------------------------------- python env ---
step "Python environment ($ENV)"
if [ ! -x "$ENV/bin/python" ]; then
    python3 -m venv "$ENV"
    ok "created"
fi
"$ENV/bin/python" -c 'import PIL, fontTools, mido' 2>/dev/null || {
    "$ENV/bin/pip" install --quiet --upgrade pip
    "$ENV/bin/pip" install --quiet Pillow fonttools mido
}
ok "Pillow, fonttools, mido"
if [ $FLASH = 1 ]; then                                 # (the installer's MIDI: a wheel on macOS; Linux needs ALSA's headers)
    "$ENV/bin/python" -c 'import rtmidi' 2>/dev/null || "$ENV/bin/pip" install --quiet python-rtmidi ||
        die "python-rtmidi did not install (on Linux: sudo apt install libasound2-dev, then run this again)"
    ok "python-rtmidi"
fi
export PYTHON="$ENV/bin/python"

# ----------------------------------------------------------------------------------------------- sources ---
step "Sources"
git submodule update --init --recursive firmware/engines >/dev/null
ok "firmware/engines (Felucca-Engines) checked out"
ok "branch $(git rev-parse --abbrev-ref HEAD) at $(git rev-parse --short HEAD)$(git diff --quiet || echo ', with local changes')"

# --------------------------------------------------------------------------------------------- toolchain ---
step "JieLi toolchain ($JIELI_TOOLCHAIN)"
if [ ! -x "$JIELI_TOOLCHAIN/pi32v2/bin/clang" ]; then
    echo "   downloading from JieLi's server (a few hundred MB, once)..."
    sh tools/get_toolchain.sh "$TC_DEST" || die "the toolchain download failed. Try again later, or from another network"
fi
ok "pi32v2 clang there"

# ------------------------------------------------------------------------------------------------- SDK ---
step "AC79 SDK files ($AC79_SDK)"
need_sdk=0
for f in uboot.boot cfg_tool.bin cfg/eq_cfg_hw.bin; do
    [ -f "$AC79_SDK/cpu/wl82/tools/$f" ] || need_sdk=1
done
if [ $need_sdk = 1 ]; then
    tmp=$(mktemp -d)
    echo "   fetching the three files the package needs from Gitee..."
    if git clone --quiet --depth 1 --branch "$SDK_TAG" --filter=blob:none --sparse "$SDK_URL" "$tmp/sdk" 2>/dev/null; then
        git -C "$tmp/sdk" sparse-checkout set cpu/wl82/tools
    else
        rm -rf "$tmp/sdk"
        git clone --quiet --depth 1 --branch "$SDK_TAG" "$SDK_URL" "$tmp/sdk" || die "the SDK download from Gitee failed"
    fi
    mkdir -p "$AC79_SDK/cpu/wl82/tools/cfg"
    cp "$tmp/sdk/cpu/wl82/tools/uboot.boot" "$tmp/sdk/cpu/wl82/tools/cfg_tool.bin" "$AC79_SDK/cpu/wl82/tools/"
    cp "$tmp/sdk/cpu/wl82/tools/cfg/eq_cfg_hw.bin" "$AC79_SDK/cpu/wl82/tools/cfg/"
    rm -rf "$tmp"
fi
ok "uboot.boot, cfg_tool.bin, cfg/eq_cfg_hw.bin"

# ----------------------------------------------------------------------------------------------- build ---
[ $CLEAN = 1 ] && { step "Clean"; rm -rf build; ok "build/ removed"; }
step "Build"
t0=$(date +%s)
./build.sh
[ -f build/felucca.fwsc ] || die "the build made no build/felucca.fwsc"
if command -v shasum >/dev/null; then SUM="shasum -a 256"; else SUM=sha256sum; fi
cp build/felucca.fwsc build/forge-one.fwsc                # (FORGE ONE: the package under its own name)
( cd build && $SUM forge-one.fwsc felucca.bin loader/ota.bin > SHA256SUMS )
ok "build/forge-one.fwsc ($(wc -c < build/felucca.fwsc | tr -d ' ') bytes, $(( $(date +%s) - t0 )) s)"
ok "FORGE $(sed -n 's/^#define FORGE_VERSION "\(.*\)"/\1/p' firmware/src/core.h) on Felucca $(sed -n 's/^#define FELUCCA_VERSION "\(.*\)".*/\1/p' firmware/src/felucca.c | head -1)"

# ----------------------------------------------------------------------------------------------- tests ---
if [ $TEST = 1 ]; then
    step "Host tests (about 10 minutes)"
    command -v cc >/dev/null || die "no C compiler for the host tests. On a Mac: xcode-select --install"
    sh tests/run_tests.sh 2>&1 | tee build/test.log | grep -E "^== |FAIL|PASSED|FAILED" || true
    grep -q "ALL HOST TESTS PASSED" build/test.log || die "host tests failed: see build/test.log"
    ok "all host tests passed (build/test.log)"
fi

# ----------------------------------------------------------------------------------------------- flash ---
if [ $FLASH = 1 ]; then
    step "Install on the FM-1"
    echo "   Quit Ableton Live and every other MIDI app; connect the FM-1 with a USB data cable and turn it on."
    echo "   Do not unplug it or let the Mac sleep while it writes."
    "$PYTHON" tools/fm1_install.py build/forge-one.fwsc
fi

step "Done"
echo "   Firmware: $ROOT/build/forge-one.fwsc"
[ $FLASH = 1 ] || echo "   To install: tools/forge_build.sh --flash   (or: $PYTHON tools/fm1_install.py build/forge-one.fwsc)"
