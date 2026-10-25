#!/bin/bash
# Tests of the scripting features: symbols, run N, until, expect, --batch, exit codes,
# signed watch, console FILE / --attrs, --profile-by=function.
#
#   tests/run_batch_tests.sh <toolchain dir>      (v32cc, v32as, packrom and StandardBios.v32 in it,
#                                                  or in the PATH when no directory is given)
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
SIM=${V32SIM:-$HERE/../bin/v32sim}
[ -n "${1:-}" ] && PATH=$(cd "$1" && pwd):$PATH
BIOS=${V32BIOS:-$(command -v StandardBios.v32)}
for t in v32cc v32as packrom; do command -v $t >/dev/null || { echo "need $t (give the toolchain dir)"; exit 2; }; done
[ -f "$BIOS" ] || { echo "need StandardBios.v32 (or set V32BIOS)"; exit 2; }
W=${WORK:-$(mktemp -d)}; mkdir -p "$W"
pass=0; fail=0
ok()  { pass=$((pass+1)); }
bad() { fail=$((fail+1)); echo "  FAIL: $1"; }
check() { if [ "$2" = "$3" ]; then ok; else bad "$1: got '$2', want '$3'"; fi; }
has()   { case "$2" in *"$3"*) ok;; *) bad "$1: '$3' not in '$2'";; esac; }

( cd "$W" && cp "$HERE"/batch/prog.c "$HERE"/batch/build.sh . && sh build.sh prog >/dev/null 2>&1 ) || { echo "cannot build prog.c"; exit 2; }
CART=$W/prog.v32
# sim ARGS... : stdin is the script, stdout and stderr go to $W/out and $W/err, the exit status is returned
sim() { timeout 60 "$SIM" -B "$BIOS" --fast "$@" "$CART" > "$W/out" 2> "$W/err"; }

echo "== symbols"
printf 'run 400\nprint counter\nprint/d [counter]\nprint &counter\nprint buf+2\nprint global_g\nquit\n' | sim -q
has "print counter"  "$(cat $W/out)" "counter"
has "print &counter" "$(cat $W/out)" "0x00000002"
check "symbol output only on stdout" "$(cat $W/err)" ""
printf 'run 60\nx/d buf\nquit\n' | sim -q
check "x buf: 4 words by default" "$(grep -c '^' $W/out)" "4"
printf 'print nosuchsymbol\nquit\n' | sim -q; check "unknown symbol exit status" "$?" "4"

echo "== run N, expect, quit N, exit codes"
printf 'run 400\nexpect print counter > 0\nexpect print counter < 400\nquit\n' | sim -q;     check "expect ok" "$?" "0"
check "batch prints nothing for a passing expect" "$(cat $W/out)$(cat $W/err)" ""
printf 'run 400\nexpect print counter == 99999\nprint 1\nquit\n' | sim -q; check "expect fails" "$?" "1"
has "failure message" "$(cat $W/err)" "FAILED"
check "run ends at the first failure" "$(cat $W/out)" ""
printf 'run 3\nquit 7\n' | sim -q; check "quit N" "$?" "7"
printf 'run 3\nquit\n' | sim -q;   check "quit" "$?" "0"
printf 'run 400\nprint/d counter\nrun 50\nprint/d counter\nquit\n' | sim -q
check "run is relative" "$(grep -c counter $W/out)" "2"
a=$(sed -n 1p $W/out | grep -o ': *[0-9]*' | tr -dc 0-9); b=$(sed -n 2p $W/out | grep -o ': *[0-9]*' | tr -dc 0-9)
[ -n "$a" ] && [ "$b" -gt "$a" ] && ok || bad "second run advances the counter ($a, $b)"
printf 'run 3\nexpect console "nothing"\nquit\n' | sim -q; check "expect console fails" "$?" "1"

echo "== until"
printf 'until "never" timeout 20\nquit\n' | sim -q; check "until timeout" "$?" "1"
has "until message" "$(cat $W/err)" "did not appear within 20 frames"

echo "== signed watch"
printf 'watch/d [neg] < 0 negw\ncontinue\nprint/d neg\nquit\n' | sim -q --max-frames=2000
has "signed watch triggers" "$(cat $W/err)" "negw"
has "neg is negative" "$(cat $W/out)" "-"

echo "== console FILE and --attrs"
printf 'run 5\nconsole %s/scr.txt\nconsole --attrs=all %s/attr.txt\nquit\n' "$W" "$W" | sim -q
check "console file" "$(test -f $W/scr.txt && echo yes)" "yes"
has "attrs: fg grid" "$(cat $W/attr.txt)" "-- fg --"
has "attrs: bg grid" "$(cat $W/attr.txt)" "-- bg --"

echo "== --profile-by=function"
sim -q -r --max-frames=600 --profile-by=function --profile-top=3 </dev/null
has "hottest function" "$(cat $W/out $W/err)" "work"
sim -q -r --max-frames=600 --profile-by=function --profile-out="$W/prof.txt" </dev/null
has "text profile" "$(cat $W/prof.txt)" "# v32sim function profile v1"

echo "== not batch: the prompt is still there"
printf 'run 3\nquit\n' | timeout 60 "$SIM" -B "$BIOS" --fast "$CART" 2>&1 | grep -q 'v32sim>' && ok || bad "interactive prompt echo"

echo "$pass passed, $fail failed"
[ $fail = 0 ]
