#!/bin/bash
# Malformed BIOS / cartridge files: the console refuses each of them, so v32sim must too.
#
#   tests/run_romcheck.sh <tools dir>      (tools: refrun, assemble, packrom)
#
# Starts from a good cartridge and BIOS (tests/gaps), breaks one thing at a time with
# Python, and requires refrun (exit 3 = ThrowException) and v32sim (exit 21) to agree on
# accept/reject. The accepted baseline must run on both.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
TOOLS=$(cd "${1:?tools dir}" && pwd)
SIM=${V32SIM:-$HERE/../bin/v32sim}
W=${WORK:-$(mktemp -d)}
mkdir -p "$W"
# build the good pair with the gap runner's own packing (one trivial test)
WORK=$W bash "$HERE/run_gaps.sh" "$TOOLS" g01_rng >/dev/null 2>&1
[ -f "$W/bios.v32" ] && [ -f "$W/g01_rng.v32" ] || { echo "cannot build baseline"; exit 2; }

python3 - "$W" <<'PY'
import struct, sys, os
W = sys.argv[1]
cart = open(f"{W}/g01_rng.v32", "rb").read()
bios = open(f"{W}/bios.v32", "rb").read()
def put(name, data): open(f"{W}/bad_{name}.v32", "wb").write(bytes(data))
def w32(b, off, v): b[off:off+4] = struct.pack("<I", v & 0xFFFFFFFF)
def r32(b, off): return struct.unpack_from("<I", b, off)[0]
c = lambda: bytearray(cart)
bi = lambda: bytearray(bios)

put("c_ok", cart); put("b_ok", bios)
put("c_oddsize", cart + b"\0\0")
put("c_tiny", cart[:64])
x = c(); x[:8] = b"V32-XXXX"; put("c_sig", x)
x = c(); x[:8] = b"V32-BIOS"; put("c_isbios", x)
x = c(); w32(x, 8, 2); put("c_newer", x)
x = c(); w32(x, 12, 1); put("c_newerrev", x)
x = c(); w32(x, 22*4, 257); put("c_manytex", x)
x = c(); w32(x, 23*4, 1025); put("c_manysnd", x)
x = c(); w32(x, 24*4, 132); put("c_progstart", x)
x = c(); w32(x, 26*4, r32(x, 26*4) + 4); put("c_vidstart", x)
x = c(); w32(x, 28*4, r32(x, 28*4) + 4); put("c_audstart", x)
x = c(); w32(x, 29*4, r32(x, 29*4) + 4); put("c_filesize", x)
x = c(); x[128:136] = b"V32-XXXX"; put("c_vbinsig", x)
x = c(); w32(x, 130*4 + 0, 0); put("c_vbinzero", x)
x = c(); w32(x, 34*4, 128*1024*1024 + 1); put("c_vbinhuge", x)
x = bi(); x[:8] = b"V32-CART"; put("b_iscart", x)
x = bi(); w32(x, 22*4, 0); put("b_notex", x)
x = bi(); w32(x, 23*4, 2); put("b_twosnd", x)
x = bi(); w32(x, 34*4, 0); put("b_vbinzero", x)
# texture header of the BIOS: right after the program ROM
n = r32(bios, 34*4); t = 128 + 12 + 4*n
x = bi(); x[t:t+8] = b"V32-XXXX"; put("b_texsig", x)
x = bi(); w32(x, t+8, 0); put("b_texwide0", x)
x = bi(); w32(x, t+8, 1025); put("b_texwide1025", x)
x = bi(); w32(x, t+12, 0); put("b_texhigh0", x)
s = t + 16 + 4 * r32(bios, t+8) * r32(bios, t+12)
x = bi(); x[s:s+8] = b"V32-XXXX"; put("b_sndsig", x)
x = bi(); w32(x, s+8, 0); put("b_snd0", x)
x = bi(); w32(x, s+8, 1024*1024+1); put("b_sndhuge", x)
PY

pass=0; fail=0
check() {    # check KIND(b|c) NAME
    local kind=$1 name=$2 f="$W/bad_$2.v32" rb rs
    if [ "$kind" = b ]; then
        timeout 60 "$TOOLS/refrun" "$f" "$W/g01_rng.v32" 2 >/dev/null 2>&1; rb=$?
        timeout 60 "$SIM" -q -r --fast -n -B "$f" --max-frames=2 "$W/g01_rng.v32" </dev/null >/dev/null 2>&1; rs=$?
    else
        timeout 60 "$TOOLS/refrun" "$W/bios.v32" "$f" 2 >/dev/null 2>&1; rb=$?
        timeout 60 "$SIM" -q -r --fast -n -B "$W/bios.v32" --max-frames=2 "$f" </dev/null >/dev/null 2>&1; rs=$?
    fi
    local refrej=0 simrej=0
    [ $rb -eq 3 ] && refrej=1
    [ $rs -eq 21 ] && simrej=1
    if [ $refrej -eq $simrej ]; then pass=$((pass+1)); else fail=$((fail+1)); echo "FAIL $name: refrun exit $rb, v32sim exit $rs"; fi
}
for f in "$W"/bad_c_*.v32; do n=$(basename "$f" .v32); check c "${n#bad_}"; done
for f in "$W"/bad_b_*.v32; do n=$(basename "$f" .v32); check b "${n#bad_}"; done
echo "romcheck: $pass passed, $fail failed"
[ $fail -eq 0 ]
