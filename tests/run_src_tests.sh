#!/bin/bash
# Tests of source-level debugging (V32DBG): sstep, snext, finish, until, break/tbreak FILE:LINE,
# list, bt/frame/up/down, the Lua/src overlay, and the fallbacks without line information.
#
#   tests/run_src_tests.sh [dir with v32as]       (v32as in the PATH when no directory is given)
#
# The program is tests/src/bench.asm, written the way `v32lua -g` writes assembly (;@src markers,
# PUSH BP / MOV BP,SP frames, a runtime helper after `;@src -`), with the Lua sources bench.lua and
# fact.lua next to it. No stock tools are needed: tests/mkcart.py wraps the VBIN in a cartridge, and
# the BIOS is tests/gaps/bios.asm, which hands over to the cartridge.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
SIM=${V32SIM:-$HERE/../bin/v32sim}
[ -n "${1:-}" ] && PATH=$(cd "$1" && pwd):$PATH
command -v v32as >/dev/null || { echo "need v32as (give its directory)"; exit 2; }
W=${WORK:-$(mktemp -d)}; mkdir -p "$W"
pass=0; fail=0
ok()  { pass=$((pass+1)); }
bad() { fail=$((fail+1)); echo "  FAIL: $1"; }
check() { if [ "$2" = "$3" ]; then ok; else bad "$1: got '$2', want '$3'"; fi; }
has()   { case "$2" in *"$3"*) ok;; *) bad "$1: '$3' not in '$2'";; esac; }
hasnt() { case "$2" in *"$3"*) bad "$1: '$3' in '$2'";; *) ok;; esac; }

cp "$HERE"/src/bench.asm "$HERE"/src/bench.lua "$HERE"/src/fact.lua "$W"/
( cd "$W" && v32as -g bench.asm -o bench.vbin ) || { echo "cannot assemble bench.asm"; exit 2; }
python3 "$HERE/mkcart.py" "$W/bench.vbin" "$W/bench.v32"
v32as -b "$HERE/gaps/bios.asm" -o "$W/bios.vbin" && python3 "$HERE/mkcart.py" --bios "$W/bios.vbin" "$W/bios.v32" || exit 2

# sim ARGS : the script on stdin; stdout and stderr (the stop lines are on stderr in batch mode) together
sim() { timeout 30 "$SIM" -q -B "$W/bios.v32" --fast "$@" "$W/bench.v32" 2>&1; }
# lines THE-OUTPUT : just the "FILE:LINE" of each [sstep]/[snext]/[until]/[finish] stop, one line each
stops() { sed -n 's/^\[\(sstep\|snext\|until\|finish\)\] \([^ :]*:[0-9]*\).*/\2/p' | tr '\n' ' ' | sed 's/ $//'; }

echo "== the line table: break FILE:LINE"
out=$(printf 'break bench.lua:8\nbreak 9\nbreak -a bench.lua:7\nbreak fact.lua:3\nbreak add\nbreak 0x20000000\nbreak\nquit\n' | sim)
has "break FILE:LINE"        "$out" "[break] 0x20000012 (bench.lua:8 in main)"
has "no code: next line"     "$out" "line 9 has no code; using line 10"
has "-a: both addresses"     "$out" "[break] 0x2000001E (bench.lua:7 in main)"
has "-a: lowest too"         "$out" "[break] 0x20000008 (bench.lua:7 in main)"
has "other file"             "$out" "(fact.lua:3 in fact)"
has "function: first insn"   "$out" "0x20000034 (bench.lua:3"
has "list shows lines"       "$out" "[0] 0x20000012 (bench.lua:8)"
has "hex stays an address"   "$out" "0x20000000"
out=$(printf 'break 99\nbreak nosuch.lua:3\nquit\n' | sim)
has "past the end"           "$out" "no code at or after line 99"
has "unknown file"           "$out" "no code at or after nosuch.lua:3"
out=$(printf 'tbreak bench.lua:8\ncontinue\nbreak\nquit\n' | sim)
has "tbreak triggers"        "$out" "[tbreak] triggered on 0x20000012 (bench.lua:8 in main)"
hasnt "tbreak is removed"    "$(printf 'tbreak bench.lua:8\ncontinue\nbreak\nquit\n' | sim | tail -1)" "[0]"

echo "== sstep"
out=$(printf 'tbreak 2\ncontinue\nsstep\nsstep\nsstep\nsstep\nsstep\nsstep\nsstep\nsstep\nquit\n' | sim)
check "order (into calls, loop header again)" "$(echo "$out" | stops)" \
      "bench.lua:7 bench.lua:8 bench.lua:3 bench.lua:4 bench.lua:5 bench.lua:6 bench.lua:7 bench.lua:8"
has "text of the line" "$out" "[sstep] bench.lua:4 in add:   local s = a + b"
out=$(printf 'tbreak 11\ncontinue\nsstep\nquit\n' | sim)
check "through code without source" "$(echo "$out" | stops)" "bench.lua:12"

echo "== snext"
out=$(printf 'tbreak 8\ncontinue\nsnext\nsnext\nsnext\nsnext\nsnext\nsnext\nquit\n' | sim)
check "skips the call (and the loop back edge stops)" "$(echo "$out" | stops)" \
      "bench.lua:7 bench.lua:8 bench.lua:7 bench.lua:8 bench.lua:7 bench.lua:10"
out=$(printf 'break fact.lua:5\ncontinue\nunbreak 0\nsnext\nbt\nquit\n' | sim)
has "recursion: the same frame" "$out" "[snext] fact.lua:6 in fact:   return n * m"
check "recursion: depth unchanged" "$(echo "$out" | grep -c '^#')" "2"

echo "== finish"
out=$(printf 'break add\ncontinue\nfinish\nbt\nquit\n' | sim)
has "from the first instruction (no frame yet)" "$out" "[finish] bench.lua:8 in main:"
has "back in main" "$out" "#0  main (bench.lua:8)"
out=$(printf 'break fact.lua:3\ncontinue\nfinish\nbt\nquit\n' | sim)
has "recursion: one level only" "$out" "[finish] fact.lua:5 in fact"
check "recursion: three frames left" "$(echo "$out" | grep -c '^#')" "4"
out=$(printf 'finish\nquit\n' | sim)
has "outermost frame" "$out" "outermost"

echo "== until"
out=$(printf 'tbreak 8\ncontinue\nuntil\nuntil\nquit\n' | sim)
check "until leaves the loop" "$(echo "$out" | stops)" "bench.lua:10 bench.lua:11"
out=$(printf 'tbreak 7\ncontinue\nuntil 10\nbt\nquit\n' | sim)
check "until LINE" "$(echo "$out" | stops)" "bench.lua:10"

echo "== backtrace, frame, up, down, list"
out=$(printf 'break fact.lua:3\ncontinue\nbt\nquit\n' | sim)
has "bt frame 0"   "$out" "#0  fact (fact.lua:3)"
has "bt recursion" "$out" "#3  fact (fact.lua:5)"
has "bt main"      "$out" "#4  main (bench.lua:10)"
out=$(printf 'break fact.lua:3\ncontinue\nup\nup\nframe\nframe 0\ndown\nquit\n' | sim)
has "up"        "$out" "#1  fact (fact.lua:5)"
has "frame"     "$out" "#2  fact (fact.lua:5)"
has "frame 0"   "$out" "#0  fact (fact.lua:3)"
has "down at the bottom" "$out" "you cannot go down"
out=$(printf 'break fact.lua:3\ncontinue\nlist\nquit\n' | sim)
has "list marks the line" "$out" "=>    3"
has "list text"           "$out" "function fact(n)"
out=$(printf 'break fact.lua:3\ncontinue\nup\nlist\nlist\nquit\n' | sim)
has "list of the selected frame" "$out" "=>    5"
out=$(printf 'list bench.lua:8\nlist add\nquit\n' | sim)
has "list FILE:LINE (line)" "$out" "    8"$'\t'"  t = add(t, i)"
has "list FUNCTION"  "$out" "local function add(a, b)"

echo "== modes and the overlay"
out=$(printf 'tbreak 8\ncontinue\nquit\n' | timeout 30 "$SIM" --source -L lua -B "$W/bios.v32" --fast "$W/bench.v32" 2>&1)
has "lua overlay" "$out" "  LUA bench.lua:8:   t = add(t, i)"
out=$(printf 'tbreak 8\ncontinue\nquit\n' | timeout 30 "$SIM" --source -L src -B "$W/bios.v32" --fast "$W/bench.v32" 2>&1)
has "src overlay" "$out" "  LUA bench.lua:8:   t = add(t, i)"
out=$(printf 'tbreak 8\ncontinue\nsstep\nquit\n' | timeout 30 "$SIM" --source -L lua -B "$W/bios.v32" --fast "$W/bench.v32" 2>&1)
check "a source step prints its line once" "$(echo "$out" | grep -c 'bench.lua:3')" "1"
out=$(printf 'set mode = src\nset\nquit\n' | sim)
has "set mode = src" "$out" "mode:    src"
out=$(printf 'tbreak 2\ncontinue\nsstep\n\n\nquit\n' | timeout 30 "$SIM" -B "$W/bios.v32" --fast "$W/bench.v32" 2>&1)
check "a blank line repeats sstep" "$(echo "$out" | stops)" "bench.lua:7 bench.lua:8 bench.lua:3"

echo "== a breakpoint ends a source step"
out=$(printf 'break fact.lua:3\ncontinue\nbreak bench.lua:11\nsnext\nquit\n' | sim)
has "breakpoint wins" "$out" "[breakpoint] triggered on"

echo "== no source information"
out=$(printf 'sstep\nsnext\nfinish\nlist\nbreak 25\nbreak\nquit\n' | sim -n)
has "sstep refuses"   "$out" "no source line information is loaded"
has "decimal break stays an address without lines" "$out" "[0] 0x00000019"
check "bt keeps the call list" "$(printf 'bt\nquit\n' | sim -n)" ""

echo "== a \`-\` entry ends the source (what ;@src - should write)"
{ cat "$W/bench.vbin.debug"; echo "0x20000060,-,0"; } > "$W/sent.vbin.debug"; cp "$W/bench.v32" "$W/sent.v32"
out=$(printf 'break 0x20000060\ncontinue\nbt\nfinish\nquit\n' | timeout 30 "$SIM" -q -B "$W/bios.v32" --fast "$W/sent.v32" 2>&1)
has "no source in the helper"     "$out" "#0  ?? [0x20000060]"
has "its caller, from [SP]"       "$out" "#1  main (bench.lua:11)"
has "finish out of the helper"    "$out" "[finish] bench.lua:12 in main:"

echo
echo "passed $pass, failed $fail"
[ "$fail" -eq 0 ]
