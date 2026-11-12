#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
mkdir -p build
buildFlags=$(pkg-config --cflags --libs sdl2 tinyxml2)
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic src/*.cpp \
    $buildFlags -o build/dungeon-maintenance
cp -R data build/
printf 'Built successfully. Run ./build/dungeon-maintenance\n'
