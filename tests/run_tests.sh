#!/bin/bash
# v32sim tests.
#
#   tests/run_tests.sh <toolchain dir> <v32io project dir>
#
#   toolchain dir     has compile, assemble, packrom and include/ (Vircon32 DevTools)
#   v32io project dir has lib/v32dev, lib/v32io, tools/v32dev.py, tests/v32dev
#
# Runs: the v32dev real-cartridge suite against v32sim (the memory card mailbox), the
# v32kbd keyboard (commands, host input file, terminal escape sequences, typed
# characters), and the memory card file persistence.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
SIM=${V32SIM:-$HERE/../bin/v32sim}
TCD=$(cd "${1:?toolchain dir}" && pwd)
IOD=$(cd "${2:?v32io project dir}" && pwd)
W=${WORK:-$(mktemp -d)}
mkdir -p "$W"
pass=0; fail=0
ok()  { pass=$((pass+1)); }
bad() { fail=$((fail+1)); echo "  FAIL: $1"; }
check() { if [ "$2" = "$3" ]; then ok; else bad "$1: got '$2', want '$3'"; fi; }

echo "== build test cartridges"
bash "$IOD/tests/v32dev/rom/build-roms.sh" "$TCD" "$IOD/lib/v32dev" "$W/roms" >/dev/null || { echo "build-roms failed"; exit 2; }
cp "$IOD"/lib/v32io/*.h "$W/roms/tc/include/"
( cd "$W/roms/carts" && cp "$HERE/carts/t_kbd.c" "$HERE/carts/t_con.c" . \
  && ../tc/compile t_kbd.c -o t_kbd.asm && ../tc/assemble t_kbd.asm -o t_kbd.vbin \
  && printf '<?xml version="1.0" encoding="UTF-8" standalone="no" ?>\n<rom-definition version="1.0">\n<rom type="cartridge" title="t_kbd" version="1.0" />\n<binary path="t_kbd.vbin" />\n<textures /><sounds />\n</rom-definition>\n' > t_kbd.xml \
  && ../tc/packrom t_kbd.xml -o t_kbd.v32 >/dev/null \
  && ../tc/compile t_con.c -o t_con.asm && ../tc/assemble t_con.asm -o t_con.vbin && sed s/t_kbd/t_con/g t_kbd.xml > t_con.xml && ../tc/packrom t_con.xml -o t_con.v32 >/dev/null ) || { echo "t_kbd build failed"; exit 2; }
BIOS=$W/roms/bios.v32
KBD=$W/roms/carts/t_kbd.v32

echo "== v32dev real-cartridge suite"
mkdir -p "$W/dev"
out=$(python3 -I "$IOD/tests/v32dev/dev_rom_test.py" "$HERE/v32sim_runner.sh" "$BIOS" "$W/roms/carts" "$W/dev" 2>&1)
echo "$out" | tail -1
echo "$out" | grep -q " 0 failures" && ok || { bad "v32dev suite"; echo "$out" | grep FAIL; }

# events NAME FRAMES INPUT_ARGS... : run t_kbd, stop after FRAMES frames, print the events
# read by the program as "code[+|-][T]" (+ press, - release, T typed character)
events() {
    local frames=$1; shift
    local cmds=$W/cmds.txt
    {
        for c in "${PRE[@]:-}"; do [ -n "$c" ] && echo "$c"; done
        printf 'watch 0x00000005 > %d\ncontinue\nprint [0x00000006]\n' "$frames"
        for i in $(seq 7 70); do printf 'print [0x%08x]\n' $i; done
        echo quit
    } > "$cmds"
    timeout 60 "$SIM" -n --fast --v32kbd=1 "$@" -B "$BIOS" "$KBD" < "$cmds" 2>&1 \
      | grep -o "(0x[0-9A-F]*)\]" | tr -d '()]' | {
        read -r n; n=$((n))
        local i=0 s=""
        while read -r h && [ $i -lt $n ]; do
            v=$((h)); t=""
            [ $((v&512)) -ne 0 ] && t="T"
            if [ -n "$t" ]; then s="$s$((v&255))$t "
            else s="$s$((v&255))$([ $((v&256)) -ne 0 ] && echo + || echo -) "; fi
            i=$((i+1))
        done
        echo "${s% }"
      }
}

echo "== v32kbd"
PRE=('kbd type "Hi!\n"' 'kbd key ctrl+c')
check "kbd type + key order" "$(events 60)" "6+ 104+ 104- 6- 105+ 105- 6+ 49+ 49- 6- 13+ 13- 10+ 99+ 99- 10-"
PRE=('kbd type "\xe9─"')
check "typed characters" "$(events 30)" "233T 21T"
PRE=('kbd key f5' 'kbd key shift+tab' 'kbd key a down' 'kbd key a up')
check "named keys" "$(events 40)" "18+ 18- 6+ 9+ 9- 6- 97+ 97-"

printf 'ls\r\033[A\033[3~\003\033x' > "$W/in.txt"
PRE=()
check "host input file (escape sequences)" "$(events 80 --kbd-input="$W/in.txt")" \
      "108+ 108- 115+ 115- 13+ 13- 1+ 1- 127+ 127- 10+ 99+ 99- 10- 12+ 120+ 120- 12-"

echo "== disassembly"
fl=$(printf 'asm fadd R1, 1.0e-5\nasm fadd R1, 0.1\nquit\n' | "$SIM" -n -B "$BIOS" "$KBD" 2>&1 | grep -o "FADD  R1, *[0-9.e+-]*" | sed 's/FADD  R1, *//' | tr '\n' ' ')
check "float immediates print shortest round-trip" "$fl" "1.0e-05 0.1 "
bi=$(printf 'load bios:%s\nquit\n' "$BIOS" | "$SIM" -n -B "$BIOS" "$KBD" 2>&1 | grep -c "ERROR")
check "load bios: at the prompt" "$bi" "0"

echo "== text console"
con=$("$SIM" -r -x -n --fast --max-frames=20 --console -B "$BIOS" "$W/roms/carts/t_con.v32" 2>&1 | head -3)
check "BIOS-font text captured" "$con" "$(printf 'Hello, v32 console\nline two\n  x=?é')"

echo "== memory card persistence"
( cd "$W/roms/carts" && cp "$HERE/carts/memwrite.asm" . && ../tc/assemble memwrite.asm -o memwrite.vbin >/dev/null \
  && printf '<?xml version="1.0" encoding="UTF-8" standalone="no" ?>\n<rom-definition version="1.0">\n<rom type="cartridge" title="memwrite" version="1.0" />\n<binary path="memwrite.vbin" />\n<textures /><sounds />\n</rom-definition>\n' > memwrite.xml \
  && ../tc/packrom memwrite.xml -o memwrite.v32 >/dev/null ) || bad "memwrite build"
python3 -I - "$W/p.memc" <<'PY'
import sys
open(sys.argv[1], "wb").write(b"V32-MEMC" + b"\0" * 4 * 262144)
PY
( timeout -s KILL 2 "$SIM" -r -n --frame-us=1000 -B "$BIOS" -M "$W/p.memc" "$W/roms/carts/memwrite.v32" </dev/null >/dev/null 2>&1 )
check "card file size" "$(stat -c %s "$W/p.memc")" "$((8 + 4 * 262144))"
check "write survives kill -9" "$(od -An -tx4 -j$((8 + 4*16)) -N4 "$W/p.memc" | tr -d ' ')" "c0de1234"
check "neighbour word untouched" "$(od -An -tx4 -j$((8 + 4*17)) -N4 "$W/p.memc" | tr -d ' ')" "00000000"

echo
echo "passed: $pass   failed: $fail"
[ "$fail" -eq 0 ]
