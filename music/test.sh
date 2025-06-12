#!/bin/sh

ORIG="$1"
TEST="/tmp/output.nes"

cmake --build --preset linux --target music_test || exit
./build/linux/music/music_test "$ORIG"

xxd "$ORIG" > /tmp/orig
xxd "$TEST" > /tmp/test

colordiff -u /tmp/orig /tmp/test && echo "No diffs!"