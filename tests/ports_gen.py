#!/usr/bin/env python3
"""One-port programs for the differential runner (G17): for each control-bus port of every
device, a read and a few writes, so that the read-only/write-only flags, the value limits
and the error codes of every port are compared with the console's.

  ports_gen.py OUTDIR
"""
import sys, os
out = sys.argv[1]
os.makedirs(out, exist_ok=True)
n = 0
for dev in range(8):
    for local in list(range(0, 0x31)) + [0x7F, 0xFF]:
        port = (dev << 8) | local
        progs = [("in", "  in R0, 0x%X\n" % port)]
        for v in ("0", "1", "-1", "3", "0x7FFFFFFF", "2.5", "300"):
            progs.append(("out_" + v.replace("-", "m").replace(".", "p"), "  mov R1, %s\n  out 0x%X, R1\n" % (v, port)))
        for name, body in progs:
            with open(os.path.join(out, "pt_%03X_%s.asm" % (port, name)), "w") as f:
                f.write(";@ frames=3 dump=64@100\n  mov R0, 77\n" + body +
                        "  mov [0x120], R0\n  mov [0x121], R1\n  in R2, TIM_FrameCounter\n  mov [0x122], R2\n  hlt\n")
            n += 1
print(n, "programs")
