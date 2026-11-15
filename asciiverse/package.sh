#!/bin/sh
# Packs the build into dist/minerl-<target>.tar.gz for av: the program at
# the top with its screens (cuss/) beside it, where it reads them from
# wherever it is started. The save and the scores live in the player's own
# folder ($XDG_STATE_HOME/minerl, or Application Support on macOS), never in
# the install folder. Left out: cuss/editor, the interface editor's screens.
#
#   asciiverse/package.sh x86_64-unknown-linux-musl
set -eu
cd "$(dirname "$0")/.."
target="$1"
stage=stage/minerl
rm -rf stage
mkdir -p "$stage/cuss" dist
cp out/minerl LICENSE "$stage/"
cp README.md "$stage/README.upstream.md"
cp asciiverse/README.txt "$stage/README.txt"
cp cuss/*.cuss "$stage/cuss/"
tar -czf "dist/minerl-$target.tar.gz" -C "$stage" .
ls dist
