#!/usr/bin/env python3
"""Random instruction programs for the differential runner (tests/run_fuzz.sh).

  fuzz_gen.py OUTDIR COUNT SEED

Each program seeds R0-R10 with interesting values, runs a few dozen random instructions
from the CPU's repertoire (integer and float ALU, comparisons, moves, memory through odd
addresses, the stack, strings, ports, jumps and calls) and ends by storing R0-R13 and SP
at RAM[0x120..]. An error lands in tests/gaps/bios.asm's handler, which stores what the CPU
reported; both emulators' RAM must match word for word.

Host-undefined behaviour (G12) is avoided on purpose: shift counts stay below 32, the
divisor of IDIV/IMOD is never -1, and floats stay small enough for CFI.
"""
import random, sys, os

out, count, seed = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
os.makedirs(out, exist_ok=True)

INTS = [0, 1, 2, 3, -1, -2, 7, 100, 255, 256, 0x7FFF, 0xFFFF, 0x10000, 0x7FFFFFFF, -0x7FFFFFFF, 12345, -12345, 31, 32]
FLOATS = [0.0, 1.0, -1.0, 0.5, -0.5, 2.0, 3.14159, 100.0, -100.0, 1000.5, 0.001, 1e6, -1e6]
ADDRS = [0x400, 0x401, 0x1000, 0x2000, 0x3FFFFF, 0x400000, 0x10000000, 0x10000004, 0x20000000,
         0x20000001, 0x30000000, 0x30000001, 0x40000500, 0x7FFFFFFF, -1, 0x500, 0x501]
PORTS = ["TIM_FrameCounter", "TIM_CycleCounter", "RNG_CurrentValue",
         "GPU_RemainingPixels", "GPU_ClearColor", "GPU_MultiplyColor", "GPU_ActiveBlending", "GPU_SelectedTexture",
         "GPU_SelectedRegion", "GPU_DrawingPointX", "GPU_DrawingPointY", "GPU_DrawingScaleX", "GPU_DrawingScaleY",
         "GPU_DrawingAngle", "GPU_RegionMinX", "GPU_RegionMinY", "GPU_RegionMaxX", "GPU_RegionMaxY",
         "GPU_RegionHotspotX", "GPU_RegionHotspotY", "SPU_SelectedSound", "SPU_SelectedChannel", "SPU_SoundLength",
         "SPU_SoundPlayWithLoop", "SPU_ChannelState", "SPU_ChannelAssignedSound", "SPU_ChannelVolume",
         "SPU_ChannelSpeed", "SPU_ChannelLoopEnabled", "SPU_ChannelPosition", "SPU_GlobalVolume",
         "INP_SelectedGamepad", "INP_GamepadConnected", "INP_GamepadLeft", "INP_GamepadRight", "INP_GamepadUp",
         "INP_GamepadDown", "INP_GamepadButtonStart", "INP_GamepadButtonA", "INP_GamepadButtonB",
         "INP_GamepadButtonX", "INP_GamepadButtonY", "INP_GamepadButtonL", "INP_GamepadButtonR",
         "CAR_Connected", "CAR_ProgramROMSize", "CAR_NumberOfTextures", "CAR_NumberOfSounds",
         "MEM_Connected"]
SCRATCH = list(range(0, 11))

def reg(rng): return "R%d" % rng.choice(SCRATCH)
def imm(rng): return str(rng.choice(INTS))
def fimm(rng): return repr(rng.choice(FLOATS))

def gen(rng, n):
    lines = []
    for r in SCRATCH:
        lines.append("  mov R%d, %d" % (r, rng.choice(INTS)))
    labels = 0
    for i in range(n):
        k = rng.randrange(100)
        d, s = reg(rng), reg(rng)
        if k < 22:
            op = rng.choice(["iadd", "isub", "imul", "imin", "imax", "and", "or", "xor"])
            lines.append("  %s %s, %s" % (op, d, rng.choice([s, imm(rng)])))
        elif k < 26:
            op = rng.choice(["isgn", "iabs", "not", "bnot"])
            lines.append("  %s %s" % (op, d))
        elif k < 29:
            lines.append("  shl %s, %d" % (d, rng.randrange(0, 32)))
        elif k < 33:
            # division: never by -1 (INT_MIN / -1 is host-defined); zero is allowed (an error)
            dv = rng.choice([0, 1, 2, 3, 7, -3, 100])
            lines.append("  %s %s, %d" % (rng.choice(["idiv", "imod"]), d, dv))
        elif k < 41:
            op = rng.choice(["ieq", "ine", "igt", "ige", "ilt", "ile"])
            lines.append("  %s %s, %s" % (op, d, rng.choice([s, imm(rng)])))
        elif k < 47:
            lines.append("  mov %s, %s" % (d, fimm(rng)))
            lines.append("  %s %s, %s" % (rng.choice(["fadd", "fsub", "fmul", "fmin", "fmax"]), d, rng.choice([reg(rng), fimm(rng)])))
        elif k < 50:
            lines.append("  mov %s, %s" % (d, fimm(rng)))
            lines.append("  %s %s" % (rng.choice(["fsgn", "fabs", "flr", "ceil", "round", "sin", "cfi", "cfb"]), d))
        elif k < 52:
            lines.append("  mov %s, %s" % (d, rng.choice(["2.0", "0.5", "-0.5", "0.0", "1.5", "1.0", "-1.0", "3.0", "100.0"])))
            op = rng.choice(["fdiv", "fmod", "pow", "atan2"])
            if op in ("atan2", "pow"):
                lines.append("  mov R11, %s" % rng.choice(["0.0", "2.0", "-1.5"]))
                lines.append("  %s %s, R11" % (op, d))
            else:
                lines.append("  %s %s, %s" % (op, d, rng.choice(["0.0", "2.0", "-1.5", "0.5", "3.0"])))
            lines.append("  %s %s" % (rng.choice(["log", "acos"]), reg(rng)))
        elif k < 55:
            lines.append("  %s %s, %s" % (rng.choice(["feq", "fne", "fgt", "fge", "flt", "fle"]), d, rng.choice([s, fimm(rng)])))
        elif k < 58:
            lines.append("  %s %s" % (rng.choice(["cif", "cib"]), d))
        elif k < 66:
            a = rng.choice(ADDRS) if rng.random() < 0.5 else rng.choice([0x400, 0x401, 0x500, 0x1000])
            form = rng.randrange(6)
            lines.append("  mov R12, %d" % a)           # address register (R12/R13 are the string regs)
            if form == 0:   lines.append("  mov %s, [R12]" % d)
            elif form == 1: lines.append("  mov [R12], %s" % s)
            elif form == 2: lines.append("  mov %s, [%d]" % (d, a))
            elif form == 3: lines.append("  mov [%d], %s" % (a, s))
            elif form == 4: lines.append("  mov %s, [R12+%d]" % (d, rng.choice([0, 1, 4, 0x100])))
            else:           lines.append("  mov [R12+%d], %s" % (rng.choice([0, 1, 4, 0x100]), s))
        elif k < 71:
            if rng.random() < 0.5:
                lines.append("  push %s" % rng.choice([s, "SP"]))
            else:
                lines.append("  pop %s" % rng.choice([d, "SP"]) if rng.random() < 0.1 else "  pop %s" % d)
        elif k < 76:
            op = rng.choice(["movs", "sets", "cmps"])
            lines.append("  mov R13, %d" % rng.choice([0x600, 0x700, 0x3FFFFF, 0x10000000, 0x20000000, 0x10000010]))
            lines.append("  mov R12, %d" % rng.choice([0x600, 0x700, 0x3FFFFE, 0x10000001, 0x20000002]))
            lines.append("  mov R11, %d" % rng.choice([0, 1, 2, 5, 40, -3, 100000]))
            lines.append("  %s" % (op if op != "cmps" else "cmps %s" % d))
            lines.append("  mov %s, %d" % ("R11", 0))
        elif k < 83:
            p = rng.choice(PORTS)
            if rng.random() < 0.5:
                lines.append("  in %s, %s" % (d, p))
            else:
                lines.append("  out %s, %s" % (p, rng.choice([s, imm(rng)])))
        elif k < 90:
            labels += 1
            op = rng.choice(["jt", "jf"])
            lines.append("  %s %s, __L%d" % (op, d, labels))
            lines.append("  %s %s, %s" % (rng.choice(["iadd", "isub"]), reg(rng), imm(rng)))
            lines.append("__L%d:" % labels)
        elif k < 93:
            labels += 1
            lines.append("  call __F%d" % labels)
            lines.append("  jmp __E%d" % labels)
            lines.append("__F%d:" % labels)
            lines.append("  %s %s, %s" % (rng.choice(["iadd", "imul", "xor"]), reg(rng), imm(rng)))
            lines.append("  ret")
            lines.append("__E%d:" % labels)
        elif k < 95:
            lines.append("  mov R12, %d" % rng.choice([0x400, 0x20000000]))
            lines.append("  lea %s, [R12+%d]" % (d, rng.choice([0, 3, 0x100])))
        elif k < 97:
            lines.append("  mov %s, %d" % (d, rng.choice(ADDRS)))
            lines.append("  jmp %s" % d) if rng.random() < 0.25 else lines.append("  mov [%s], %s" % (d, s))
        else:
            lines.append("  wait")
    for r in range(14):
        lines.append("  mov [%d], R%d" % (0x120 + r, r))
    lines.append("  mov R0, SP")
    lines.append("  mov [0x12E], R0")
    lines.append("  in R0, TIM_FrameCounter")
    lines.append("  mov [0x12F], R0")
    lines.append("  hlt")
    return "\n".join(lines) + "\n"

rng = random.Random(seed)
for i in range(count):
    body = gen(rng, rng.randrange(15, 45))
    with open(os.path.join(out, "fz_%04d.asm" % i), "w") as f:
        f.write(";@ frames=6 dump=64@100\n")
        f.write(body)
