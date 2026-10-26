#!/bin/bash
# Generated differential suites against the stock console logic:
#
#   tests/run_fuzz.sh <tools dir> [fuzz seeds, default "1 2 3"] [programs per seed, default 300]
#
# fuzz_gen.py: random instruction programs; ports_gen.py: one program per control-bus port
# (read and several writes). Same oracle as run_gaps.sh.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
TOOLS=${1:?tools dir}
SEEDS=${2:-"1 2 3"}
N=${3:-300}
D=$(mktemp -d)
rc=0
for sd in $SEEDS; do
    mkdir -p "$D/fz$sd"; python3 "$HERE/fuzz_gen.py" "$D/fz$sd" "$N" "$sd"
    echo "fuzz seed $sd:"; GAPS_DIR="$D/fz$sd" bash "$HERE/run_gaps.sh" "$TOOLS" || rc=1
done
mkdir -p "$D/ports"; python3 "$HERE/ports_gen.py" "$D/ports"
echo "ports:"; GAPS_DIR="$D/ports" bash "$HERE/run_gaps.sh" "$TOOLS" || rc=1
exit $rc
