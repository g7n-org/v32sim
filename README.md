# v32sim

**v32sim**  is  a text-based  debugger  and  simulator for  the  Vircon32
Fantasy  Console.  Its  purpose  is   to  make  the  machine  observable:
single-step  the  CPU  instruction  by instruction,  inspect  and  modify
registers, memory, and IOPorts,  set breakpoints and watchpoints, profile
execution,  and  drive  the  machine from  command  files  for  automated
testing.

It is not a player: there is  no window and  no audio output. It models
the CPU, the memory system, the IOPorts, a software GPU (so `screenshot`
can write the screen to a PNG), and an SPU (channel and sound state, but
nothing is played)  closely enough to study,  debug, and hack Vircon32
software — and to automate cart testing.

## Current State

- All 64 CPU instructions are implemented and are checked by running the same
  cartridges on v32sim and on the stock console logic (`tests/run_gaps.sh`,
  `run_fuzz.sh`, `run_romcheck.sh`; see `V32SIMGAPS.md`): the timer and RNG, the
  stack, the memory bus, error timing, one word per cycle for `MOVS`/`SETS`/`CMPS`,
  GPU port clamping and ROM validation match the console. Only host-undefined
  behavior (shift counts >= 32, `INT_MIN / -1`) and pixel-level rendering differ
- Registers (R0-R15 with their aliases, plus system registers IP/IR/IV) and
  the four memory pages (RAM, BIOS, CART, MEMCARD) behave as the hardware does
- System errors (invalid memory/port access, division by zero, etc.) record
  the machine state into R0-R3, reset SP/BP to the last word of RAM, and
  redirect execution to the BIOS error handler at `0x10000000`
- IOPort groups: TIM (with leap years), RNG, INP (gamepads), CAR, MEM, GPU, SPU
- **GPU**: a software renderer with a 640x360 framebuffer: clear, draw, zoom,
  rotate, roto-zoom, blending (alpha/add/subtract), multiply color, and the
  per-frame pixel budget (`GPU_RemainingPixels`); `screenshot` writes it to a PNG
- **SPU**: 16 channels with sound selection, volume, speed, loop and position
  state advance every frame, and VSND data from the BIOS and cartridge is
  loaded; there is no audio output
- **Memory card**: the `V32-MEMC` header and size are validated; changes reach
  the file at the end of every frame (so a killed simulator loses at most one
  frame), as the DesktopEmulator does. A card formatted for **v32dev** keeps a
  live mailbox, so programs using the v32dev library (storage, serial, ...)
  talk to a real `v32dev.py serve` host process (see *Memory Card and v32dev*)
- **v32io keyboard**: a `v32kbd` on any gamepad port, fed from the prompt
  (`kbd type`, `kbd key`) or from the host (a terminal, pipe, FIFO or file),
  see *The v32io Keyboard*
- **Watchpoints** on registers, IOPorts, and memory words, compared with a
  number or with another register/IOPort/memory word; edge-triggered
- **Save / restore** of the whole machine state
- **Live assembler**: `asm` assembles one instruction and patches it into
  memory, or runs it once
- **Source-level debugging** (V32DBG): `sstep`/`snext`/`finish`/`until` step by source
  line, `break`/`tbreak FILE:LINE`, `list`, and `bt` with function names and lines, for
  Lua, C and assembly; see [Source-Level Debugging](#source-level-debugging)
- **Source overlay**: the assembly, C or Lua source line for the current IP is
  printed at the prompt
- Command front end built on **flex/bison** (`lexer.l`, `parser.y`), and a second
  flex/bison pair for the assembler (`asm_lexer.l`, `asm_parser.y`): commands may
  be abbreviated, `#` starts a comment, and syntax errors report the offending
  token and column
- `ignore` skips the current instruction; a **SIGSEGV/SIGBUS/SIGFPE/SIGABRT**
  handler prints the simulator state before dying
- Experimental **Lua mode** (`--langmode=lua`): memory words are inspected for
  Lua value tags (strings, tables, functions, nil/true/false) when displaying
  with `/B` or `/s`
- **Scripting** (`--batch`): symbols (labels and C globals) wherever a number is
  accepted, `run N`, `until "TEXT"`, `expect`, `quit N`, exit statuses, `console FILE`
  and `console --attrs`, signed `watch/d`, and profiles by function; see
  [Scripting](#scripting)
- **Debug files** (labels and line numbers) are loaded automatically (see
  [Debug Files](#debug-files)); `-n` turns that off

## Table of Contents

- [Project Layout](#project-layout)
- [Requirements](#requirements)
- [Building](#building)
- [Installing](#installing)
- [Usage](#usage)
- [Memory Map](#memory-map)
- [The Debugger](#the-debugger)
- [Commands](#commands)
- [Source-Level Debugging](#source-level-debugging)
- [Memory Card and v32dev](#memory-card-and-v32dev)
- [The v32io Keyboard](#the-v32io-keyboard)
- [The Text Console](#the-text-console)
- [Running Unattended](#running-unattended)
- [Scripting](#scripting)
- [Tests](#tests)
- [IOPORTS](#ioports)
- [Known Limitations and Planned Features](#known-limitations-and-planned-features)

## Project Layout

```
.
├── Makefile              build definitions
├── README.md             this file
├── LICENSE
├── TODO.md
├── src/                  all C sources
│   ├── lexer.l           flex scanner for prompt / command-file lines
│   ├── parser.y          bison grammar; fills in a parsed command (cmd_t)
│   ├── command.c         command table, abbreviations, and all executors
│   ├── asm_lexer.l       flex scanner of the live assembler (prefix `asm`)
│   ├── asm_parser.y      bison grammar of the live assembler (prefix `asm`)
│   ├── assembly.c        mnemonic table, symbols, instruction encoding
│   ├── gpu.c             software GPU, framebuffer, PNG screenshot
│   ├── spu.c             SPU channel / sound model, VSND loading
│   ├── savestate.c       save / restore of the machine state
│   ├── source.c          assembly / C / Lua source-line overlay
│   ├── srcdebug.c        source-level debugging: line table, sstep/snext/finish/until,
│                         frames and bt, break FILE:LINE, list
│   ├── watchpoint.c      register / IOPort / memory watchpoints
│   └── input.c           prompt loop, readline, command-file input, debug files
├── inc/                  all headers (command.h, version.h, ...)
├── obj/                  objects, dependency files, and the generated
│                         lexer.c / parser.c / parser.h and the asm_* equivalents
├── bin/                  the compiled v32sim binary (generated)
├── bios/                 BIOS data files
├── cart/                 cartridge data files
└── screenshots/          reference screenshots
```

## Requirements

- A C compiler (GCC or Clang)
- **GNU readline** — prompt line editing, cursor keys, and command history
  (on macOS the history facility is part of readline; on Linux `libhistory`
  is a separate library — the Makefile handles both)
- **flex** and **bison** (any reasonably recent version; the grammar avoids
  version-specific directives, so macOS's stock bison 2.3 / flex 2.5.35 work)
- libm (standard on Linux and macOS)

POSIX regex is no longer used, and PNG output has no library dependency (the
screenshot writer is built in).

## Building

```
make            # default build          -> bin/v32sim
make debug      # -DDEBUG -g build       -> bin/v32sim
make asan       # AddressSanitizer + UBSan build, for hunting memory errors
make version    # print the version string
make clean      # remove obj/ contents (including generated parser) and bin/v32sim
```

Object files  and dependency files  land in  `obj/`; the binary  lands in
`bin/v32sim`. Header  dependencies are  tracked, so  editing a  header in
`inc/` rebuilds every  source that includes it. Object  files are **not**
rebuilt when flags change — run `make clean` when switching between the
default, `debug`, and `asan` builds.

## Installing

```
make install      # installs to ~/bin/bin.<arch>/ (or ~/bin/)
make sysinstall   # installs to /usr/local/bin/
```

## Usage

```
Usage: v32sim [OPTION]... [CARTFILE.v32]
Debugger/Simulator for Vircon32 Fantasy Console
Mandatory arguments to long options are mandatory for short options too.
 -B, --biosfile=FILE       load this BIOS V32 file as BIOS
 -b, --break=OFFSET|LABEL  set breakpoint at OFFSET/LABEL
 -C, --command-file=FILE   load this file with sim commands
 -c, --colors              enable colorized output
 -d, --deref-addr          output address of dereference
 -D, --debug               enable simulator debug output (development)
 -e, --errorcheck          enable runtime error checking
     --bios-asm-debug=FILE load BIOS asm labels from FILE
     --bios-c-debug=FILE   load BIOS C labels from FILE
     --cart-asm-debug=FILE load CART asm labels from FILE
     --cart-c-debug=FILE   load CART C labels from FILE
     --debug-dir=DIR       also look for debug files in DIR (first)
     --source              show the source line for each stop (asm; C with -L C)
 -E, --entry-point=OFFSET  set simulator entry point
 -L, --langmode=MODE       label/source mode: C or lua (experimental)
 -M, --memcfile=FILE       load this file as a MEMCARD
 -n, --no-debug            do not process any debug files
 -p, --profile             enable instruction profiling
 -r, --run                 do not enable single-step mode
 -S, --bios-start          break at BIOS code start
 -w, --watch-for=OPCODE    run until OPCODE is encountered
 -v, --verbose             enable more verbose output
 -V, --version             display version information and exit
 --v32kbd[=PORT]           emulate a v32kbd keyboard on gamepad PORT (default 0)
 --kbd-input=SOURCE        host key source: stdin, fd:N, FIFO/pipe/file (implies --v32kbd)
 --fast                    do not throttle to real time
 --frame-us=N              wall-clock microseconds per frame (default 16666)
 --max-frames=N            stop after N frames
 --fixed-clock[=Y,D,S]     start TIM_CurrentDate/Time at year Y, day D (0 = Jan 1st), S seconds (default 2026,0,0)
 --seed=N                  seed the RNG_CurrentValue generator with N
 -x, --exit-status         exit status tells how the program ended
 -q, --batch               scripting mode: no prompt, banners or instruction lines; stderr for diagnostics
     --profile-by=KIND     address (default) or function: what --profile-top/--profile-out aggregate
     --symbol-file=FILE    load data symbols (v32cc NAME.asm.sym) from FILE
 -h, --help                display this information
FILE   is any path plus the filename desired
OFFSET is the full 32-bit/4-byte memory address (hex)
OPCODE is the full 32-bit/4-byte instruction hex
```

`v32sim --version` prints e.g. `v32sim 20261018-dev`. The version is the
`VERSION` define in `inc/version.h`; it is shown in the banner, the usage text,
the `version` prompt command, and `make version`. Override at build time with
`-DVERSION=\"...\"`.

Examples:

```
v32sim cart.v32                     # start in the debugger
v32sim -r cart.v32                  # just run the cart
v32sim -b main -C setup.cmds cart.v32
v32sim -L lua -r cart.v32           # Lua value display
v32sim -q --fast cart.v32 < t.cmd   # scripted run, see Scripting
```

By  default  the simulator  loads  `StandardBios.v32`  from the  standard
Vircon32 ComputerSoftware install location; override with `--biosfile`.

## Memory Map

| Page | First address | Notes |
|------|--------------|-------|
| RAM  | `0x00000000` | 16MB / 4M words, read-write |
| BIOS | `0x10000000` | entry point at `0x10000004`, error handler at `0x10000000` |
| CART | `0x20000000` | cartridge program ROM |
| MEMC | `0x30000000` | memory card |

## The Debugger

When  not  started  with  `--run`,  the  simulator  stops  at  the  first
instruction  and  presents  the  `v32sim>`  prompt.  From  here  you  can
single-step  (`step`),   step  over  subroutines  (`next`),   run  freely
(`continue`), and  arrange for  execution to  stop again  at breakpoints,
watchpoints, or a watched-for opcode.

- **Ctrl-C (SIGINT)** breaks out of run mode back to the prompt
- **EOF on stdin** (e.g. Ctrl-D, or a closed input pipe) exits cleanly —
  handy when driving v32sim from scripts
- On a **system error**, the machine state is stored in R0-R3, SP/BP are
  wiped, execution is redirected to the BIOS error handler at
  `0x10000000`, and the simulator stops at the prompt to let you inspect
  the fault

### Command Files

`--command-file=FILE` (or  `-C`) feeds  the simulator  a plain  text file
with **one  valid prompt  command per  line** (any  command: breakpoints,
display  list  setup, `continue`,  etc.). Blank lines and lines (or line
endings) beginning with `#` are comments.  This  is  the primary  way  to
automate sessions;  combine it  with `--run` and  redirected stdin/stdout
for scripted testing.

## Commands

Commands are case-insensitive and may be abbreviated to any unambiguous
prefix (`s` = step, `n` = next, `c` = continue, `b` = break, `p` = print,
`se` = set, ...). An empty line repeats the last step/next/continue.

| Command    | Description |
| ---------- | ----------- |
| `break [0xMEM\|LABEL\|FILE:LINE\|LINE]` | set an execution breakpoint (address, label, function or source line; no argument: list them) |
| `tbreak ...` | the same, removed when hit |
| `unbreak #` | remove breakpoint by index |
| `continue` | resume execution until next trigger |
| `step` | execute current instruction, stop at next |
| `next` | execute current instruction, step over subroutines |
| `sstep` / `snext` | run to the next *source line* (`snext` runs calls to their return) |
| `finish` | run until the selected frame's function returns |
| `until [FILE:LINE\|LINE]` | run to a later line of the function (leave a loop), or to that line |
| `list [FILE:LINE\|LINE\|FUNCTION]` | show source around the current line (or that one) |
| `frame [N]` / `up [N]` / `down [N]` | select a stack frame |
| `print/fmt XYZ` | one-time display of XYZ (register, memory, IOPort, symbol) |
| `x/fmt SYMBOL\|0xADDR [COUNT]` | show COUNT words from a symbol (default: the whole C variable) |
| `display/fmt XYZ [LABEL]` | add a displaylist item shown at each stop |
| `undisplay #` | remove displaylist item by index |
| `label [0xMEM_ADDR LABEL]` | list or set a label for a memory offset |
| `unlabel #` | remove label by index |
| `watch [LOC OP VALUE [LABEL]]` | set a watchpoint on a register, IOPort, or memory word (no arguments: list) |
| `watchlist` | list active watchpoints |
| `run N` / `wait N` | continue for N frames, then stop |
| `until "TEXT" [timeout N] [rows N]` | continue until the text console shows TEXT |
| `expect [print] LOC OP VALUE` / `expect console "TEXT"` | check a value or the screen; failure sets exit status 1 |
| `console [FILE]` / `console --attrs[=fg\|bg\|all] [FILE]` | show or write the text screen (with color grids) |
| `unwatch #\|LABEL` | remove a watchpoint |
| `backtrace` | list subroutine calls, most recent first; with Lua/C line information, the BP frames as `#0 f (file:line)` |
| `profile` | show the profiling report (requires `-p`) |
| `inventory` | system resource overview |
| `gamepad [# [COMMAND]]` | view/select/simulate gamepad state (`gamepad 1` selects pad 1) |
| `load cart:path` / `load memc:path` | load a component at runtime |
| `unload bios\|cart\|memc` | unload a memory page |
| `replace [IP:0xADDR] [IR:0x...] [IV:0x...]` | patch the instruction (and immediate) at IP/ADDR in memory |
| `asm [@0xADDR\|-x] INSTRUCTION` | assemble an instruction: patch it in (at IP or ADDR), or run it once with `-x` |
| `screenshot [FILE]` | write the GPU screen to a PNG file |
| `save [FILE]` / `restore FILE` | write / restore the whole machine state |
| `set NAME=VALUE` | set settings, registers, memory, or IOPorts |
| `ignore` | skip the current instruction (it is not executed and the counters do not advance) |
| `version` | show the simulator version |
| `help [COMMAND]` / `?` | command help |
| `quit [N]` | exit the simulator (with exit status N) |

### Formatting Suffixes

`print` and `display` accept a format suffix. Numeric operands are hex: up
to 3 digits is an IOPort, more is a memory address (`print/x 0x205`,
`print GPU_ClearColor`, `print R3`, `print reg`, `print [SP]`):

- `/X` uppercase hexadecimal (default), `/x` lowercase hexadecimal
- `/u` unsigned decimal, `/d` signed decimal, `/o` octal, `/b` binary
- `/f` floating point
- `/B` boolean (TRUE/FALSE)
- `/D` decode the value as an instruction
- `/s` string (one 32-bit word per character; in Lua mode, unboxes tagged
  strings)

### Watchpoints

`watch  LOCATION  OP  VALUE  [LABEL]` breaks when the comparison *becomes*
true. `LOCATION` is a register (R0-R15, BP, SP, CR, SR, DR, IP, IR, IV), an
IOPort (a symbol, or hex with up to 3 digits), or a memory word (hex with more
digits); `OP` is one of `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`; `VALUE` is a
number (hex, binary, octal, decimal), a register, an IOPort symbol, or the
contents of `[0xMEM]`, `[IOPORT]`, or `[REG]`; the LABEL is optional:

```
watch SP <= 0x20000000 stack_underflow
watch 0x00001000 != 0
watch TIM_FrameCounter = 60
watch R1 > [0x00000010]
```

Watchpoints are edge-triggered: a condition that is already true when it is set,
or that stays true, does not break again until it has been false in between.
They are checked after every instruction and whenever the machine state changes
(so a value that changes and changes back within one instruction is seen).
Remove with `unwatch #` or `unwatch LABEL`; `watchlist` (or `watch` alone)
lists them. Comparisons are unsigned 32-bit.

### Live Assembly

`asm` assembles one instruction using the Vircon32 assembler syntax:

```
asm mov R1, 0x10                       # patch the instruction at the current IP
asm @0x20000100 out GPU_Command, GPUCommand_ClearScreen
asm -x iadd R0, -1                     # execute once, memory untouched
```

Operands are registers (`R0`-`R15`, `CR`, `SR`, `DR`, `BP`, `SP`), numbers
(decimal, `0xHEX`, `0bBINARY`, `0OCTAL`, floats), labels from the loaded debug
files, IOPort names, and the `GPUCommand_*`, `GPUBlendingMode_*`,
`SPUCommand_*`, and `SPUChannelState_*` constants. A decimal integer given to a
float instruction is converted (`fadd R4, 1` adds 1.0); hex, binary and octal
are raw bit patterns. A patch may not be longer than the instruction it
replaces; a shorter one is padded with `MOV R0, R0` (a no-op). The assembled
output of every instruction of the BIOS has been checked to match its machine
code word for word.

### Screenshots

`screenshot [FILE]` writes the 640x360 GPU framebuffer to a PNG (default
`v32sim.DATESTAMP.png`). The renderer is a software model of the console's GPU
(nearest-neighbor sampling, the same pixel budget and blending), so a screenshot
shows what the cart has drawn so far this frame.

### Save and Restore

`save [FILE]` writes the machine state (registers, IOPorts, RAM, memory card,
GPU regions and framebuffer, SPU, timers, gamepads; default
`v32sim.DATESTAMP.sav`), and `restore FILE` loads it back. The file is
little-endian with zero runs compressed, and records checksums of the BIOS and
cartridge: it is only restored into a machine with the same ones. Breakpoints,
watchpoints, the display list, labels, and profiling data are part of the
debugging session and are not saved.

### Display List

The display list  re-renders each time the simulator stops.  Items may be
registers (dereferenceable with `[...]`),  memory addresses or ranges, or
IOPorts (numeric like `0x205` or symbolic like `GPU_SelectedTexture`). If
no label is given for an IOPort item, its symbolic name is used.

### Labels

`label 0x10000040 main` associates a name with an offset; labels can then
be used  for breakpoints  (`-b main`  or `break  main`) and as `asm` operands.
Labels are also loaded automatically from assembler/C debug files (see below).

### Debug Files

Next to every V32 file, v32sim looks for its debug file (`NAME.vbin.debug`,
`NAME.asm.debug`, and the C equivalents) in the directory of the V32 file, the
current directory, `obj/` and `../obj/`, and the directory given by
`--debug-dir` (searched first). `--bios-asm-debug` and friends name a file
explicitly. `-n` disables all of this. Debug files hold one `OFFSET,FILE,LINE[,LABEL]`
entry per instruction.

With `--source` (or `set source=true`) the source line of the current
instruction is printed at each stop: assembly lines by default, the C or Lua
lines with `-L src` (any language; `-L C` and `-L lua` are aliases that also keep
their old extra effects) or `set mode = src`. The language label (`ASM`, `C`, `LUA`)
comes from the file's extension. File names in debug files are relative to the directory
the program was built in, so they are tried as given and relative to the
directories the debug and V32 files were found in (and their parents).

### Set

```
set color=true      set deref=true       set debug=true
set verbose=true    set errorchk=true    set profile=true
set source=true     set mode=C           # source overlay, and which source
set R4=0x4004       set IP=0x10000040    # IP is a jump
set 0x00224466=0x71
set GPU_SelectedTexture=-1
```

Booleans accept true/false, on/off, yes/no, 1/0. `set` with no arguments
lists the current settings. Memory and IOPort targets are validated.

### Load/Unload

`load  cart:path`  and  `load   memc:path`  swap  components  at  runtime
(primarily for MEMCARD swapping). `unload bios|cart|memc` deallocates the
page. `load` checks the file can be opened first, and `unload` refuses a page
that is not loaded. `load bios:path` replaces the BIOS (its labels are
reloaded and the fetched instruction is refreshed).

## Source-Level Debugging

The debug files carry the source file and line of every instruction (see the V32DBG notes):
`v32as -g` writes `ADDRESS,FILE,LINE[,FUNCTION]` from the `;@src FILE:LINE` markers of a
Lua (`v32lua -g`) or C program, and the C compiler's side file is matched to addresses as
before. v32sim keeps one sorted line table from whichever it finds and builds the line-level
commands on it. `step` and `next` stay instruction steps; a blank line repeats the last
source step.

| Command | Does |
| ------- | ---- |
| `sstep` | run until the source line changes (a different file, line or function, or a jump back to the start of a line: a new pass of a loop), going **into** calls |
| `snext` | the same, but a `CALL` runs to its return (the stack pointer must be back where it was: recursion safe; no `RET` needs to be seen) |
| `finish` | run until the selected frame returns; works at the first instruction of a function too (see *Frames*) |
| `until` | like `snext`, but only stops on a **later** line of this function (or when it returns): leaves a loop |
| `until FILE:LINE` | run to that line, or until the current function returns |
| `break FILE:LINE`, `break LINE` | the lowest address of the line; `break -a` all of them. A line with no code moves to the next line that has some, and says so |
| `tbreak ...` | one-shot |
| `list`, `list LINE`, `list FILE:LINE`, `list FUNCTION` | ten lines of source, `=>` marking the current one; `list` again continues |
| `bt`, `frame N`, `up`, `down` | the frames, selected frame (used by `list` and `finish`) |

`break LINE` is a **decimal** number below `0x10000000` when line information is loaded;
addresses are written in hex (`break 0x20000012`), as before. `break FUNCTION` sets the
breakpoint on the function's first instruction (not on every line, although the assembler's
entries all carry the function name). FILE may be a base name (`bench.lua`) or any tail of
the path.

Each stop prints one line, `[sstep] bench.lua:4 in add:   local s = a + b` (to stderr in
`--batch` mode, like the other stop reasons; `bt`, `list` and `frame` print to stdout).

### Frames

Both v32cc and v32lua use `PUSH BP; MOV BP,SP` frames, so after the prologue the return
address is at `[BP+1]` and the caller's BP at `[BP]`. The frame of the running function is
recognized from its instructions, so it is also right at its first instruction (nothing
pushed yet), after `PUSH BP`, and at `POP BP; RET`. Code with no source (see below) is
assumed frameless and entered by a `CALL`: the return address is on top of the stack, if
the word there is the address after a `CALL`. The walk ends at an address that is not code,
or at an unreadable or non-increasing BP.

`backtrace` uses these frames when the debug data is about Lua or C (a file that is not
`.asm`); for assembly programs it keeps listing the `CALL` targets, as it always did.
Code built with `-O2` and `omit-frame-pointers` has no chain: `snext` and `until` do not
need one (they use the `CALL` instruction and SP), `finish` and `bt` do.

### Code without source

An entry whose file is `-` (or whose line is 0), e.g. `0x20000060,-,0`, says that the
instructions from that address up to the next entry have no source: a runtime library. It
is what `;@src -` means. `sstep` runs through such code without stopping, `bt` shows
`?? [0x...]` and `finish` returns from it. **`v32as` does not write this entry today** (it
writes nothing for a `;@src -` range), so the last line before the runtime is taken to extend
to the next entry; one line per `;@src -` marker would fix that (see the notes in TODO.md).

## Memory Card and v32dev

A memory card file is `V32-MEMC` followed by 262144 little-endian words. The
simulator keeps it in step with card memory the way the DesktopEmulator does:
every write marks its 256-word page, and the dirty pages are written at the
end of each frame.

**v32dev** (the v32io project's host-device transport) uses the last 16384
words of the card as a mailbox with four channels. The host side is a separate
process (`v32dev.py serve CARD --ch1 storage:disk.img`) that reads and writes
the card *file* while the program runs. For that to work the simulator plays
the emulator's part:

  * the **header** (liveness flag and heartbeat, words 8 and 9 of the region)
    is maintained in memory only and never written to the file
  * the **host-owned area** is read back from the file at the start of every
    frame, and never written; the program cannot clobber what the host wrote
  * of the **cart-owned** parts, the 16 control words of each channel (the ring
    indices) are written after everything else, so a host that sees a new index
    also sees the data it announces
  * a card without a valid header (or with `V32DEV_DISABLE` set) is an ordinary
    card; none of this applies

`mailbox [CHANNEL]` shows the whole state: liveness, heartbeats, ring indices,
descriptors and pending data. `restore` leaves a live mailbox's header and
host area alone.

The v32dev real-cartridge test suite of the v32io project (`dev_rom_test.py`)
passes unchanged against v32sim: see *Tests*.

## The v32io Keyboard

v32io devices look like a gamepad to the program. The keyboard, `v32kbd`, uses
Left/Right as a strobe, Up/Down for press/release, and Start A B X Y L R as a
7-bit key code, one key event per frame. v32sim emulates one on any port:

    v32sim --v32kbd=1 --kbd-input=stdin -B bios.v32 cart.v32
    v32sim --kbd-input=/tmp/keys cart.v32            (a FIFO, pipe or file)

or from the prompt: `kbd attach 1`, `kbd type "ls -l\n"`, `kbd key ctrl+c`,
`kbd key f1 down`, `kbd input FILE`, `kbd` (status), `kbd clear`.

Text becomes key events the way a US keyboard produces them (shift is held
across consecutive characters that need it); UTF-8 characters the BIOS font
has (accents, box drawing...) are sent as v32io *typed characters*. A terminal
used as input is put in raw mode while the program runs: arrow keys, Delete,
F1-F12, Ctrl and Alt combinations are translated, and **Ctrl-]** returns to the
debugger prompt (which gets the terminal back). Text that arrives faster than
one event per frame waits, nothing is lost.

## The Text Console

Characters drawn with the BIOS texture (texture -1, region = character code) are
also kept as text. Two layouts are recognized:

  * the **BIOS font** as `print_at()` lays it out: 10x20 pixel cells, 64x18
  * the **v32c++ terminal** (`libc/v32term.h`): 80x24 cells of 8x15 pixels, drawn
    slightly scaled from regions the library defines on the BIOS texture; a cell's
    "paper" draw blanks it and the glyph draw that follows sets its character

The layout drawn to last is the one shown. `console` prints it (UTF-8; BIOS box
drawing characters become Unicode), `--console` prints it when the run ends, and
ClearScreen empties it.

**Colors.** The GPU multiply color of each draw is kept: the glyph's color is the
cell's foreground and, for the 80x24 terminal, its "paper" draw is the background
(so reverse video works). They are shown with plain ANSI color sequences, no
curses library needed, reduced to the level you choose: `none`, `16`, `256` or
`true` (24 bit), nearest color by RGB distance. `--console-color=MODE` or
`console color MODE` sets it; `auto` follows `NO_COLOR`, `COLORTERM` and `TERM`.
Unset, `--tty` uses auto and the `console` dump is plain text (so it can go to a
file). White text and a black background are the terminal's own defaults.

**Cursor.** A program draws its cursor itself, usually the solid block (BIOS
character 20). `--cursor-color=COLOR` / `console cursor COLOR [CODE...]` recolors
those cells (a name, `#RRGGBB`, or `off`); more character codes, such as a hollow
box outline, can be listed after the color.

### Interactive mode

    v32sim --tty -B bios.v32 v32nix.v32            keyboard on gamepad 0
    v32sim --tty --v32kbd=1 -B bios.v32 cart.v32   keyboard on gamepad 1

`--tty` runs the program with the text console on your terminal (alternate
screen, repainted when the text changes) and your keys going to an emulated
v32kbd, so a text program such as v32nix or V32DOS can be used right there. It
needs a terminal on stdin and stdout. **Ctrl-]** leaves for the debugger prompt
(the terminal is given back; the last screen stays in the scrollback as text);
`continue` returns to the program. Ctrl-C and the other control keys go to the
program, not to v32sim. The program must read a v32kbd on that gamepad port
(`v32kbd_init` / `v32io` library); one that reads the on-screen keyboard of
v32term's default key source will not see your keys.

## Running Unattended

  * `--fast` / `--frame-us=N`: do not sleep to hold 60 frames per second (or
    wait N microseconds per frame). `--max-frames=N` stops after N frames.
  * `-x` / `--exit-status`: the exit status tells how the run ended: 0 the
    program halted, 2 the frame limit was reached, 3 a system error (the run
    stops there instead of entering the BIOS error handler).
  * `-C FILE` commands are read by the prompt (after the first instruction is
    fetched), so `step`, `next` and `continue` act; with `-r` they run once at start.
  * `--dump=N[@ADDR|@LABEL]` (or `ADDR:N`): when the run ends, print `N` words of memory as
    signed integers, one per line, before the final register banner. `ADDR` is
    hex (default: the start of RAM); a label is looked up as given and then as
    `global_LABEL`, so a v32cc global `counter` can be dumped as
    `--dump=1@counter` when its debug file is loaded. Together with `-r -x --fast
    --max-frames=N` this is a headless "run to HLT and dump RAM" mode, as
    `tools/vrun -d N` in v32cc.
  * `--fixed-clock[=YEAR,DAY,SECONDS]` and `--seed=N` make runs reproducible: the
    date/time ports start from a fixed value instead of the host clock, and the
    RNG is seeded at startup. Two runs of the same cartridge then give identical
    RAM dumps (a program that seeds itself from `TIM_CycleCounter` still differs
    between builds of different speed, as the cycle count differs).

### Address profile

`--profile-out=FILE` writes, when the run ends, how many times the instruction at each
address was executed. It needs no prompt, no `-p` and no debug files, and works with
`-r -x --max-frames=N` (a run stopped by the frame limit still writes it) as well as a
run that reaches `HLT`:

```
# v32sim address profile v1
# cycles 403 frames 0
0x00000004 100
0x00000006 100
```

  * One line per executed address (BIOS included), sorted by address; the address is that
    of the instruction's first word. `# cycles` is the number of instructions executed
    (a `WAIT` counts once), so the counts add up to it. With a cartridge asm debug file
    loaded, a third column names the label covering the address.
  * `--profile-format=bin` writes the line `V32PROF1`, then little-endian 32-bit
    `address, count` pairs.
  * `--profile-top=N` prints the N hottest addresses (with labels, when known) to stderr.
  * `--profile-by=function` sums the counts over the addresses of each function (the
    label that covers them in the loaded debug files; BIOS addresses without a label
    are one group): `--profile-top=N` then lists the N hottest functions with their
    share and the number of addresses, and the text `--profile-out` has one line per
    function (`# v32sim function profile v1`, then `0xFIRST COUNT NADDR NAME`). Load
    the C map (`-L C`, or the `.vbin.debug` made by `v32as --map`) to get C names;
    without labels the output says so. The binary format is unchanged.
  * `--fixed-cycles` makes a program's reads of `TIM_CycleCounter` return 0, so code that
    seeds from it behaves the same across builds; with `--fixed-clock` and `--seed` the
    profile is reproducible, byte for byte.

## Scripting

Everything a script needs to drive a headless run and to say what went wrong. A script
is the standard input of the simulator (or `-C FILE`).

### Batch mode

`--batch` (`-q`, `--quiet`) makes stdout hold exactly what the script asked for:

  * no `v32sim>` prompt and no echo of the commands, no `[ADDR] HH HH HH HH OPC`
    instruction line after a stop, no register banner at the end, no end-of-run
    banner (`regs`, `print` and `console` still print, on stdout);
  * acknowledgements (`[watch] ...`, `[console] wrote ...`, why a run stopped) and all
    diagnostics go to stderr, so a script's stdout never has to be filtered;
  * a blank line is skipped and the end of the input ends the run (no endless loop);
  * the first failed `expect`, or an `until` that times out, ends the run.

Exit status (always, `-x` adds the frame-limit and system-error codes): 0 success;
1 an `expect` failed or an `until` timed out; 2 the frame limit was reached (`-x`);
3 a system error (`-x`); 4 a command line could not be parsed (batch mode); `quit N`
overrides all of them. Without `--batch` nothing about the interactive output changes
(scripts that filter the prompt and instruction lines keep working).

### Symbols

A name is accepted wherever a number is: `print`, `display`, `watch` (target and right
side), `expect`, `x`, `break` and `--dump`. A symbol is looked up as a data symbol,
then as a label of the loaded debug files, each as given and then as `global_NAME`
(the assembler's name for a C global), and last ignoring case.

  * Data symbols come from `NAME.asm.sym` (written by `v32cc -g`, with the size of each
    variable) or from the `%define global_NAME ADDRESS` lines of `NAME.asm`, found next
    to the V32 file or as the debug files are (see [Debug Files](#debug-files)).
    `--symbol-file=FILE` names a `.asm.sym` file explicitly. `-n` turns them off.
  * `print counter` shows the word at the address `counter` stands for, with the
    name in the header: `counter(0x00000002): 0x00000096`; `[counter]` is the same;
    `[[ptr]]` follows a pointer; `&counter` is the address itself; `buf+2` and `buf-1`
    add a constant (one `+`/`-` constant is accepted).
  * `x/d buf` lists the words of a variable (the whole array by default),
    `x/D work 10` ten instructions of a function.
  * `watch [counter] == 6`, `watch/d [neg] < 0` and the watchpoint list name the
    variable (`counter`, `buf+2`) instead of an address.

### Waiting and checking

  * `run N` (alias `wait N`): continue for N frames. A breakpoint or watchpoint that
    triggers first stops it earlier; a `continue` afterwards is not limited.
  * `until "TEXT" [timeout N] [rows N]`: continue until the text console shows TEXT
    (anywhere in a row; `^` and `$` anchor to the start and end of a row), looking at
    the last N rows only when `rows` is given. The screen is checked at the end of every
    frame and has to have changed since the command (so what is already there does not
    end the wait). Default timeout: 3600 frames.
  * `expect LOC OP VALUE` (as `watch`, evaluated now; `expect print R0 == 5` also
    works) and `expect console "TEXT" [rows N]`. A passing check is silent in batch mode.
  * `watch/d` (or `watch/D`) compares as signed 32-bit numbers: `watch/d [neg] < 0`.
    The list marks it `[signed]`. Plain `watch` stays unsigned.

### Console to a file

`console FILE` writes the text screen to FILE. `console --attrs[=fg|bg|all] [FILE]`
adds, after the text, `-- fg --` and `-- bg --` grids with the same rows and columns
as the text; every cell is the hex digit of the nearest ANSI colour (0 black, 1 red,
2 green, 3 yellow, 4 blue, 5 magenta, 6 cyan, 7 light gray, 8-F bright versions, F
white), `.` for a blank in the foreground grid. A test can check that a word is red.
`screenshot FILE` writes the pixels as PNG.

### Example

```
$ cat session.cmd
kbd attach 2
until "^MEM:\>$" rows 1 timeout 1500
run 60
kbd type "ver\n"
run 120
expect console "Vircon32 Disk Operating System"
console --attrs=fg screen.txt
quit
$ v32sim --batch --fast -B StandardBios.v32 --v32kbd dos.v32 < session.cmd; echo $?
```

## Tests

`tests/run_tests.sh TOOLCHAIN V32IO_PROJECT` builds the test cartridges with
the Vircon32 DevTools and runs: the v32dev suite through
`tests/v32sim_runner.sh` (a `dev_rom_runner` look-alike: 41 checks of storage
and serial devices, host crash and recovery, plain and unformatted cards), the
v32kbd checks (commands, host input with escape sequences, typed characters,
event order), the disassembly round trip of float immediates, `load bios:`, and
card persistence when the simulator is killed with `kill -9`.

`tests/run_batch_tests.sh [TOOLCHAIN]` builds `tests/batch/prog.c` with `v32cc`, `v32as`
and `packrom` (from TOOLCHAIN or the PATH, with `StandardBios.v32` or `$V32BIOS`) and
checks the scripting features: symbols, `x`, `run`, `until`, `expect`, `quit N`, the exit
statuses, signed watch, `console FILE`/`--attrs`, `--profile-by=function`, and that the
interactive prompt is unchanged.

`tests/run_src_tests.sh [V32AS_DIR]` checks the source-level commands (49 checks) on
`tests/src/bench.asm`, assembly written the way `v32lua -g` writes it, with `v32as` only: the
cartridge is made by `tests/mkcart.py`, and the BIOS is `tests/gaps/bios.asm`.

## IOPORTS

All IOPorts exist and follow the DesktopEmulator implementation:

- **TIM**: date/time (leap years are handled), frame/cycle counters
- **RNG, INP, CAR, MEM**: behave as expected
- **GPU**: `GPU_ClearColor`, `GPU_SelectedTexture`, `GPU_SelectedRegion`,
  `GPU_Command` (ClearScreen, DrawRegion, DrawRegionZoomed, DrawRegionRotated,
  DrawRegionRotozoomed), `GPU_MultiplyColor`, `GPU_ActiveBlending`,
  `GPU_DrawingPointX/Y`, `GPU_DrawingScaleX/Y`, `GPU_DrawingAngle`,
  `GPU_RegionMinX/MinY/MaxX/MaxY`, `GPU_RegionHotspotX/Y`, and
  `GPU_RemainingPixels` render into the framebuffer
- **SPU**: `SPU_GlobalVolume`, `SPU_SelectedSound`, `SPU_SoundLength`
  (read-only), `SPU_SoundPlayWithLoop`, `SPU_SoundLoopStart/End`,
  `SPU_SelectedChannel`, `SPU_ChannelState` (read-only),
  `SPU_ChannelAssignedSound`, `SPU_ChannelVolume`, `SPU_ChannelSpeed`,
  `SPU_ChannelLoopEnabled`, `SPU_ChannelPosition`, `SPU_Command`; channels
  advance 735 samples per frame, but nothing is mixed or played

All textures and sounds present in a V32 file are loaded; `inventory` lists
them. From the prompt, ports can be manipulated via `set` and inspected via
`print`/`display`.

## Known Limitations and Planned Features

- **SPU**: no audio output (no WAV capture either)

- Lua and C display modes (`-L`) are experimental; locals and values (`info locals`, `print NAME` in a frame,
  needing the `;@var` records of V32DBG) are not implemented
- The debug-file reader is hand-written (it could move onto flex)

See TODO.md for the open items.
