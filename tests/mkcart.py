#!/usr/bin/env python3
"""mkcart.py [--bios] IN.vbin OUT.v32 : wrap a VBIN (v32as output) in a V32 ROM file, enough to
run it on v32sim (the real packer is packrom). A cartridge has no textures and no sounds; a BIOS
has the one 1x1 texture and the one short sound the console requires."""
import struct, sys
args = sys.argv[1:]
bios = False
if args and args[0] == "--bios":
    bios = True
    args = args[1:]
vbin = open(args[0], "rb").read()
vtex = b"V32-VTEX" + struct.pack("<II", 1, 1) + b"\xff\xff\xff\xff" if bios else b""
vsnd = b"V32-VSND" + struct.pack("<I", 2) + struct.pack("<hhhh", 100, -100, 200, -200) if bios else b""
title = b"v32sim test".ljust(64, b"\0")
hdr = (b"V32-BIOS" if bios else b"V32-CART") + struct.pack("<II", 1, 0) + title
hdr += struct.pack("<II", 1, 0)                       # ROM version, revision
n = 1 if bios else 0
hdr += struct.pack("<II", n, n)                       # textures, sounds
prog = 128
video = prog + len(vbin)
audio = video + len(vtex)
hdr += struct.pack("<IIIIII", prog, len(vbin), video, len(vtex), audio, len(vsnd))
hdr = hdr.ljust(128, b"\0")
assert len(hdr) == 128
open(args[1], "wb").write(hdr + vbin + vtex + vsnd)
