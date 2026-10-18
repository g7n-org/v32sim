# v32sim

**v32sim**  is  a text-based  debugger  and  simulator for  the  Vircon32
Fantasy  Console.  Its  purpose  is   to  make  the  machine  observable:
single-step  the  CPU  instruction  by instruction,  inspect  and  modify
registers, memory, and IOPorts,  set breakpoints and watchpoints, profile
execution,  and  drive  the  machine from  command  files  for  automated
testing.

It is not an emulator in the audiovisual sense: there is no screen and no
sound. It  models the  CPU, the  memory system,  and the  IOPorts closely
enough to  study, debug, and hack  Vircon32 software — and  to automate
cart testing.

## Current State

- All 64 CPU instructions are implemented (not all exhaustively tested yet)
- Registers (R0-R15 with their aliases, plus system registers IP/IR/IV) and
  the four memory pages (RAM, BIOS, CART, MEMCARD) behave as the hardware does
- System errors (invalid memory/port access, division by zero, etc.) record
  the machine state into R0-R3, wipe SP/BP, and redirect execution to the
  BIOS error handler at `0x10000000`
- Functional IOPort groups: TIM, RNG, INP (gamepads), CAR, MEM, and most GPU
  ports (textures, regions, drawing parameters)
- SPU ports exist and store/retrieve values, but have no sound engine behind
  them (no audio playback)
- Command front end built on **flex/bison** (`lexer.l`, `parser.y`): commands
  may be abbreviated, `#` starts a comment, and syntax errors report the
  offending token and column
- Register watchpoints (`watch`), breakpoints, display list, labels, profiling
- Experimental **Lua mode** (`--langmode=lua`): memory words are inspected for
  Lua value tags (strings, tables, functions, nil/true/false) when displaying
  with `/B` or `/s`
- **C mode** (`--langmode=C`) selects C-style label handling; the C source line
  is *not* displayed yet (see TODO.md)
- Labels are loaded from debug files only when a `--*-debug` option is given
  or the debug file is found next to the V32 file (current directory)

## Table of Contents

- [Project Layout](#project-layout)
- [Requirements](#requirements)
- [Building](#building)
- [Installing](#installing)
- [Usage](#usage)
- [Memory Map](#memory-map)
- [The Debugger](#the-debugger)
- [Commands](#commands)
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
│   └── input.c           prompt loop, readline, command-file input
├── inc/                  all headers (command.h, version.h, ...)
├── obj/                  objects, dependency files, and the generated
│                         lexer.c / parser.c / parser.h (generated)
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

POSIX regex is no longer used.

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
| `break [0xMEM\|LABEL]` | set an execution breakpoint (address or label) |
| `unbreak #` | remove breakpoint by index |
| `continue` | resume execution until next trigger |
| `step` | execute current instruction, stop at next |
| `next` | execute current instruction, step over subroutines |
| `print/fmt XYZ` | one-time display of XYZ (register, memory, IOPort) |
| `display/fmt XYZ [LABEL]` | add a displaylist item shown at each stop |
| `undisplay #` | remove displaylist item by index |
| `label [0xMEM_ADDR LABEL]` | list or set a label for a memory offset |
| `unlabel #` | remove label by index |
| `watch [REG OP VALUE [LABEL]]` | set a watchpoint on a register (no arguments: list) |
| `watchlist` | list active watchpoints |
| `unwatch #\|LABEL` | remove a watchpoint |
| `backtrace` | list subroutine calls, most recent first |
| `profile` | show the profiling report (requires `-p`) |
| `inventory` | system resource overview |
| `gamepad [# [COMMAND]]` | view/select/simulate gamepad state (`gamepad 1` selects pad 1) |
| `load cart:path` / `load memc:path` | load a component at runtime |
| `unload bios\|cart\|memc` | unload a memory page |
| `replace [IP:0xADDR] [IR:0x...] [IV:0x...]` | patch the instruction (and immediate) at IP/ADDR in memory |
| `set NAME=VALUE` | set settings, registers, memory, or IOPorts |
| `ignore` | skip current instruction *(currently behaves like continue)* |
| `version` | show the simulator version |
| `help [COMMAND]` / `?` | command help |
| `quit` | exit the simulator |

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

`watch  REG  OP  VALUE  [LABEL]`  breaks  when  the  register  meets  the
condition: `OP` is one  of `=`, `==`, `!=`, `<`, `>`, `<=`,  `>=`; `REG` is any
of  R0-R15,  BP, SP,  CR,  SR,  DR, IP,  IR,  IV; `VALUE` is any integer
(hex, binary, octal, decimal); the LABEL is optional, e.g.
`watch SP <= 0x20000000 stack_underflow`. Remove with `unwatch #` or
`unwatch LABEL`; `watchlist` (or `watch` alone) lists them.
Register watchpoints only, for now.

### Display List

The display list  re-renders each time the simulator stops.  Items may be
registers (dereferenceable with `[...]`),  memory addresses or ranges, or
IOPorts (numeric like `0x205` or symbolic like `GPU_SelectedTexture`). If
no label is given for an IOPort item, its symbolic name is used.

### Labels

`label 0x10000040 main` associates a name with an offset; labels can then
be used  for breakpoints  (`-b main`  or `break  main`). Labels  are also
loaded  automatically from  assembler/C debug  files (`--bios-asm-debug`,
etc.), which are searched next to the corresponding V32 file.

### Set

```
set color=true      set deref=true       set debug=true
set verbose=true    set errorchk=true    set profile=true
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
that is not loaded. `load bios:path` is parsed but not supported yet.

## IOPORTS

All IOPorts exist. Not all are functionally accurate:

- **TIM, RNG, INP, CAR, MEM**: behave as expected
- **SPU**: storage/retrieval only, no sound engine
- **GPU**: functional, especially texture/region management — `GPU_ClearColor`,
  `GPU_SelectedTexture`, `GPU_SelectedRegion`, `GPU_Command` (ClearScreen,
  DrawRegion, DrawRegionZoomed, DrawRegionRotated, DrawRegionRotozoomed),
  `GPU_MultiplyColor`, `GPU_ActiveBlending` (Alpha/Add/Subtract),
  `GPU_DrawingPointX/Y`, `GPU_DrawingScaleX/Y`, `GPU_DrawingAngle`,
  `GPU_RegionMinX/MinY/MaxX/MaxY`, `GPU_RegionHotspotX/Y`,
  `GPU_RemainingPixels` (read-only)

All textures  present in a V32  file are loaded; `inventory`  lists them.
From the  prompt, texture/region ports  can be manipulated via  `set` and
inspected via `print`/`display`.

## Known Limitations and Planned Features

- **SPU**: no audio functionality (ports only)
- **Screenshots**: the `screenshot` command (PNG output via the GD library)
  is planned but not yet implemented; GD is not currently a build dependency
- **MEMCARD**: loaded read-only — changes are not written back to the file;
  the V32-MEMC header is not yet validated
- **`watch`** supports registers and immediate values only
- GPU has no framebuffer, so nothing is rendered; VSND data is not loaded
- **`ignore`** does not yet skip the instruction; it currently behaves like
  `continue`
- **`load bios:`** is parsed but not yet functional
- Lua and C display modes (`-L`) are experimental
