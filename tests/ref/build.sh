#!/bin/bash
# Build the headless reference runner and the Vircon32 assembler / ROM packer.
#
#   tests/ref/build.sh <ComputerSoftware checkout> <output dir>
#
# The checkout is https://github.com/Vircon32/ComputerSoftware at commit e75b537 (the
# revision V32SIMGAPS.md compares against). Produces <output dir>/{refrun,assemble,packrom}.
# Only a C++17 compiler is needed: SDL2 (which the assembler uses solely to find its own
# folder) is replaced by a stub header, and the ROM packer needs nothing but tinyxml2,
# which comes with the checkout.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
SRC=$(cd "${1:?ComputerSoftware checkout}" && pwd)
OUT=$(mkdir -p "${2:?output dir}" && cd "$2" && pwd)
CXX=${CXX:-g++}
FLAGS="-O1 -std=c++17 -w"

mkdir -p "$OUT/stub"
cat > "$OUT/stub/SDL.h" <<'EOF'
// minimal stand-in for SDL.h: the assembler only asks for its executable's base path
#pragma once
#include <stdlib.h>
#include <string.h>
static inline int SDL_Init(unsigned) { return 0; }
static inline char* SDL_GetBasePath() { return strdup("./"); }
static inline void SDL_free(void* p) { free(p); }
static inline void SDL_Quit() {}
EOF

DEV="$SRC/DevelopmentTools"
INF="-I$DEV -I$DEV/DevToolsInfrastructure -I$DEV/ExternalLibraries/tinyxml2 -I$SRC/VirconDefinitions -I$OUT/stub"
$CXX $FLAGS $INF "$DEV"/Assembler/*.cpp "$DEV"/DevToolsInfrastructure/*.cpp -o "$OUT/assemble"
$CXX $FLAGS $INF "$DEV"/RomPacker/*.cpp "$DEV"/DevToolsInfrastructure/*.cpp \
     "$DEV/ExternalLibraries/tinyxml2/tinyxml2.cpp" -o "$OUT/packrom"

LOGIC="$SRC/DesktopEmulator/ConsoleLogic"
$CXX $FLAGS -I"$LOGIC" -I"$SRC/VirconDefinitions" "$HERE/refrun.cpp" "$LOGIC"/*.cpp -o "$OUT/refrun"
echo "built: $OUT/refrun $OUT/assemble $OUT/packrom"
