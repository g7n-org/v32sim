#!/bin/bash
# Differential tests: v32sim against the stock Vircon32 console logic.
#
#   tests/run_gaps.sh <tools dir> [name filter]
#
#   tools dir   holds refrun, assemble and packrom, as built by tests/ref/build.sh
#
# Every tests/gaps/*.asm is a cartridge. The BIOS is tests/gaps/bios.asm: it hands over
# to the cartridge, and its error handler saves R0-R13 at RAM[0x100..0x10D], the frame
# and cycle counter at 0x10E/0x10F, and halts. A cartridge says how to run it with
# one or more lines of the form
#
#   ;@ frames=N dump=COUNT@HEXADDR [date=Y,D] [time=SECONDS] [name=LABEL]
#
# (frames: upper bound, the cartridges halt; dump: RAM words compared, default 288@0).
# Each line is one run on both emulators; the two dumps must be identical.
# An optional line  ;% KEY WORD...  gives words to describe after a failure.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
TOOLS=$(cd "${1:?tools dir (refrun, assemble, packrom)}" && pwd)
FILTER=${2:-}
GAPS=${GAPS_DIR:-$HERE/gaps}      # where the cartridges are (the fuzzer writes its own)
SIM=${V32SIM:-$HERE/../bin/v32sim}
W=${WORK:-$(mktemp -d)}
mkdir -p "$W"

pass=0; fail=0
BIOS=$W/bios.v32
VTEX=$W/test.vtex
VSND=$W/test.vsnd

# the 1x1 texture and the short sound every BIOS needs (the packer's own test assets)
python3 - "$VTEX" "$VSND" <<'EOF'
import struct, sys
# V32-VTEX: signature, width, height, then RGBA pixels
open(sys.argv[1], "wb").write(b"V32-VTEX" + struct.pack("<II", 1, 1) + b"\xff\xff\xff\xff")
# V32-VSND: signature, number of samples, then stereo int16 pairs
open(sys.argv[2], "wb").write(b"V32-VSND" + struct.pack("<I", 2) + struct.pack("<hhhh", 100, -100, 200, -200))
EOF

pack() {    # pack TYPE NAME  : NAME.asm -> NAME.v32 (cartridge or bios) in $W
    local type=$1 name=$2 src=$3
    local abase=""
    [ "$type" = bios ] && abase="-b"
    "$TOOLS/assemble" $abase "$src" -o "$W/$name.vbin" >"$W/$name.asm.log" 2>&1 || { echo "assemble $name failed"; cat "$W/$name.asm.log"; return 1; }
    local tex="<textures />" snd="<sounds />"
    if [ "$type" = bios ]; then
        tex="<textures><texture path=\"test.vtex\" /></textures>"
        snd="<sounds><sound path=\"test.vsnd\" /></sounds>"
    fi
    cat > "$W/$name.xml" <<XML
<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<rom-definition version="1.0">
<rom type="$type" title="$name" version="1.0" />
<binary path="$name.vbin" />
$tex
$snd
</rom-definition>
XML
    "$TOOLS/packrom" "$W/$name.xml" -o "$W/$name.v32" >"$W/$name.pack.log" 2>&1 || { echo "packrom $name failed"; cat "$W/$name.pack.log"; return 1; }
}

pack bios bios "$HERE/gaps/bios.asm" || exit 2

for src in "$GAPS"/*.asm; do
    name=$(basename "$src" .asm)
    [ "$name" = bios ] && continue
    case "$name" in *"$FILTER"*) ;; *) continue;; esac
    pack cartridge "$name" "$src" || { fail=$((fail+1)); continue; }
    n=0
    grep '^;@' "$src" | while IFS= read -r line; do echo "$line"; done > "$W/$name.runs"
    [ -s "$W/$name.runs" ] || echo ";@ frames=5" > "$W/$name.runs"
    while IFS= read -r line; do
        n=$((n+1))
        frames=5; dump="288@0"; date=""; time=""; label=$n
        for kv in ${line#;@}; do
            case $kv in
                frames=*) frames=${kv#frames=};;
                dump=*)   dump=${kv#dump=};;
                date=*)   date=${kv#date=};;
                time=*)   time=${kv#time=};;
                name=*)   label=${kv#name=};;
            esac
        done
        rargs=(--dump "$dump"); sargs=(--dump="$dump")
        [ -n "$date" ] && rargs+=(--date "$date")
        [ -n "$time" ] && rargs+=(--time "$time")
        if [ -n "$date$time" ]; then
            y=${date%,*}; d=${date#*,}; s=${time:-0}
            sargs+=(--fixed-clock="${y:-2026},${d:-0},$s")
        fi
        timeout 120 "$TOOLS/refrun" "$W/bios.v32" "$W/$name.v32" "$frames" "${rargs[@]}" >"$W/$name.$n.ref" 2>"$W/$name.$n.referr"
        # v32sim: a hardware error drops to the prompt (the debugger), so feed it 'continue's
        yes continue | head -20 | timeout 120 "$SIM" -q -r --fast -n -B "$W/bios.v32" --max-frames="$frames" "${sargs[@]}" "$W/$name.v32" >"$W/$name.$n.sim" 2>"$W/$name.$n.simerr"
        if cmp -s "$W/$name.$n.ref" "$W/$name.$n.sim"; then
            pass=$((pass+1))
        else
            fail=$((fail+1))
            echo "FAIL $name [$label]"
            # show the first differing words: address (hex), reference, v32sim
            first=${dump#*@}
            paste -d' ' <(seq 0 $((${dump%@*}-1))) "$W/$name.$n.ref" "$W/$name.$n.sim" 2>/dev/null \
              | awk -v base=$((16#$first)) '$2 != $3 { printf "    [%08X] ref=%d  sim=%d\n", base+$1, $2, $3; if (++c >= 12) exit }'
            [ -s "$W/$name.$n.simerr" ] && head -3 "$W/$name.$n.simerr" | sed 's/^/    sim stderr: /'
        fi
    done < "$W/$name.runs"
done

echo "gaps: $pass passed, $fail failed"
[ $fail -eq 0 ]
