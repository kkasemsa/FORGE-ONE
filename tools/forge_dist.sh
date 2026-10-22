#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# FORGE ONE: the distribution folder of a build (CI's artifact, a release's zip):
#   tools/forge_dist.sh DIR [commit] [ref] [run] [tests]
# DIR gets forge-one.fwsc (build/felucca.fwsc under its own name), felucca.bin, loader/ota.bin, SHA256SUMS,
# MANIFEST.txt (what was built, from what, how), LICENSE, LICENSING.md, LICENSES/, ATTRIBUTION.txt (the CC0 samples),
# README.md, BUILDING.md and the test log. Run it after ./build.sh and the tests.
set -e
cd "$(dirname "$0")/.."
D=${1:?usage: forge_dist.sh DIR [commit] [ref] [run] [tests]}
COMMIT=${2:-$(git rev-parse HEAD 2>/dev/null || echo unknown)}
REF=${3:-$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo unknown)}
RUN=${4:-local}
TESTS=${5:-$([ -f build/test.log ] && echo "see test.log" || echo "not run")}   # (CI passes "passed": it only gets here then)
[ -f build/felucca.fwsc ] || { echo "forge_dist: build/felucca.fwsc is missing (run ./build.sh first)" >&2; exit 1; }
rm -rf "$D"
mkdir -p "$D/loader" "$D/LICENSES"
cp build/felucca.fwsc "$D/forge-one.fwsc"
cp build/felucca.bin "$D/felucca.bin"
cp build/loader/ota.bin "$D/loader/ota.bin"
cp LICENSE LICENSING.md README.md BUILDING.md CREDITS.md "$D/"
cp LICENSES/*.txt "$D/LICENSES/"
[ -f assets/samples-cc0/ATTRIBUTION.txt ] && cp assets/samples-cc0/ATTRIBUTION.txt "$D/ATTRIBUTION.txt"
[ -f build/test.log ] && cp build/test.log "$D/test.log"
FV=$(sed -n 's/^#define FORGE_VERSION *"\([^"]*\)".*/\1/p' firmware/src/core.h)
UV=$(sed -n 's/^#define FELUCCA_VERSION *"\([^"]*\)".*/\1/p' firmware/src/felucca.c)
{
    echo "FORGE ONE $FV (on Felucca $UV)"
    echo "package     forge-one.fwsc  $(wc -c < build/felucca.fwsc) B"
    echo "app         felucca.bin     $(wc -c < build/felucca.bin) B"
    echo "loader      loader/ota.bin  $(wc -c < build/loader/ota.bin) B"
    echo "commit      $COMMIT"
    echo "ref         $REF"
    echo "run         $RUN"
    echo "built       $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "engines     $(git -C firmware/engines rev-parse HEAD 2>/dev/null || echo unknown) (Felucca-Engines, MIT)"
    echo "build flags $(env | grep -o '^FELUCCA_[A-Z0-9_]*=[0-9]' | sort | tr '\n' ' ')(unset: the defaults in firmware/src/felucca.c)"
    echo "host tests  $TESTS"
    echo "source      https://github.com/kkasemsa/FORGE-ONE (branch forge); upstream https://github.com/hugelton/Felucca"
    echo "licence     GPL-3.0-only (LICENSE, LICENSING.md, LICENSES/)"
} > "$D/MANIFEST.txt"
(cd "$D" && sha256sum forge-one.fwsc felucca.bin loader/ota.bin MANIFEST.txt > SHA256SUMS)
echo "dist: $D ($TESTS)"; cat "$D/MANIFEST.txt"
