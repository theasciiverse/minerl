#!/bin/sh
# Records asciiverse/minerl.cast, the catalog preview, at 100x30
# (hub/RECORDING.md) with hub/tools/castrec.py from a sibling hub checkout:
# the title screen, a new claim, digging down through the dirt for ore,
# along a seam, a ladder, deeper again, and a look at the help. The
# game plays from a throwaway state folder; the build in out/ is used
# (asciiverse/build.sh).
#
#   sh asciiverse/record.sh
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
castrec=${CASTREC:-$root/../../hub/tools/castrec.py}
inst=$(mktemp -d)
state=$(mktemp -d)
mkdir -p "$inst/cuss"
cp "$root/out/minerl" "$inst/"
cp "$root/cuss/"*.cuss "$inst/cuss/"
s="w3;kn;w2.5"
# Dig straight down from the start: dirt takes three swings a tile
for i in $(seq 1 75); do s="$s;kj;w0.12"; done
# Then sideways along a seam, a ladder, and deeper again
for i in $(seq 1 15); do s="$s;kl;w0.12"; done
s="$s;kw;w0.6"
for i in $(seq 1 40); do s="$s;kj;w0.12"; done
# A look at the help, then deeper
s="$s;w1;k?;w4;kesc;w1"
for i in $(seq 1 40); do s="$s;kj;w0.12"; done
s="$s;w2"
XDG_STATE_HOME=$state python3 "$castrec" "$root/asciiverse/minerl.cast" "$state" "$s" -- "$inst/minerl"
