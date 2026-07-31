# v32sim

A text-based Vircon32 simulator/debugger, predominantly for use in debugging, hacking, and studying the system.

The aim is to implement enough of the Vircon32 Fantasy Console so that code execution can be studied. Obviously, lacking an actual screen output (or any of the other devices, like audio), it is not meant as any sort of replacement, but can be surprisingly functional, even for use in automating activities for testing.

At this point, all instructions have been implemented (but many still need testing); registers and memory are present and should behave in a manner similar to how the system operates. Probably the least functional aspect is anything SPU-related. The ports exist, but are hooked to no tangible functionality beyond being a store and retrieval of information.

## Table of Contents

- [IOPORTS](#ioports)
- [Usage](#usage)
- [Dependencies](#dependencies)
- [Arguments](#arguments)
- [Commands](#commands)
- [Command Reference](#command-reference)
- [Gamepad Simulation](#gamepad-simulation)
- [MEMCARD Support](#memcard-support)
- [GPU and Texture Support](#gpu-and-texture-support)
- [Profiling](#profiling)
- [Screenshots](#screenshots)

## IOPORTS

All IOPorts are available within the simulator. However, not all ports in each category are functionally accurate.

For example: the SPU ports are not involved in any processing. Any values written to them or read from them are just that: standalone values. There will be no change to that data.

Some other ports do have established functionality: all the TIM, RNG, INP, CAR, and MEM ports should actually behave as expected.

Furthermore, some of the GPU ports are now functional, especially those related to textures, regions, and their definitions. All textures present in a V32 file will be loaded into memory in the simulator.

### TEXTURES AND REGIONS

Outside of BIOS and CART code that manipulates the GPU ports, from the simulator prompt one can manipulate the texture and region GPU ports, through the use of the `set` command and `display` or `print` commands.

Also, the `inventory` command will display the currently established texture information (resolution, offsets) in the BIOS and CART.

Many GPU ports should be fully functional:

- `GPU_ClearColor`
- `GPU_SelectedTexture`
- `GPU_SelectedRegion`
- `GPU_Command` (with support for `GPUCommand_ClearScreen`, `GPUCommand_DrawRegion`, `GPUCommand_DrawRegionZoomed`, `GPUCommand_DrawRegionRotated`, `GPUCommand_DrawRegionRotozoomed`)
- `GPU_MultiplyColor`
- `GPU_ActiveBlending` (with modes: `GPUBlendingMode_Alpha`, `GPUBlendingMode_Add`, `GPUBlendingMode_Subtract`)
- `GPU_DrawingPointX`, `GPU_DrawingPointY`
- `GPU_DrawingScaleX`, `GPU_DrawingScaleY`
- `GPU_DrawingAngle`
- `GPU_RegionMinX`, `GPU_RegionMinY`, `GPU_RegionMaxX`, `GPU_RegionMaxY`
- `GPU_RegionHotspotX`, `GPU_RegionHotspotY`
- `GPU_RemainingPixels` (read-only)

The other GPU ports may well be able to be read from/written to, but if there is deeper functionality, they currently just serve as a storage of information (nothing is yet done with that information).

## USAGE

```
Usage: v32sim [OPTION]... [CARTFILE.v32]
Debugger/Simulator for Vircon32 Fantasy Console
Mandatory arguments to long options are mandatory for short options too.
 -B, --biosfile=FILE       load this BIOS V32 file as BIOS
 -b, --break=OFFSET|LABEL  set breakpoint at OFFSET/LABEL
 -C, --command-file=FILE   load this file with sim commands
 -c, --colors              enable colorized output
 -d, --deref-addr          output address of dereference
 -e, --errorcheck          enable runtime error checking
     --bios-asm-debug=FILE load BIOS asm labels from FILE
     --bios-c-debug=FILE   load BIOS C labels from FILE
     --cart-asm-debug=FILE load CART asm labels from FILE
     --cart-c-debug=FILE   load CART C labels from FILE
 -E, --entry-point=OFFSET  set simulator entry point
 -M, --memcfile=FILE       load this file as a MEMCARD
 -n, --no-debug            do not process any debug files
 -p, --profile             enable instruction profiling
 -r, --run                 do not enable single-step mode
 -S, --bios-start          break at BIOS code start
 -w, --watch-for=OPCODE    run until OPCODE is encountered
 -v, --verbose             enable more verbose output
 -h, --help                display this information
FILE   is any path plus the filename desired
OFFSET is the full 32-bit/4-byte memory address (hex)
OPCODE is the full 32-bit/4-byte instruction hex
```

The simulator will, by default, use the Vircon32 `StandardBios.v32` from the standard location. This can be changed with the `--biosfile` argument, should you wish to use a different BIOS.

To disable the "debugger", run with `--run`, it will just be a simulator.

## DEPENDENCIES

### GNU READLINE

GNU readline is used for input prompt processing, allowing for shell-like input management (cursor keys, CTRL keys, command history, etc.).

**NOTE:** on macOS, there does not seem to be a separate `history` library; a tweak has been added to the Makefile to avoid an error while building on macOS.

### POSIX REGEX

The simulator currently makes extensive use of the POSIX Regular Expressions functions.

### MATH

For some of the higher level math operations, the math library is used.

### GD LIBRARY

For screenshot generation, the GD graphics library is required to output PNG files.

## ARGUMENTS

### BIOSFILE

By default, `v32sim` attempts to read `StandardBios.v32` from the standard Vircon32 ComputerSoftware install location on the system.

If desired, you can specify an alternate BIOS that the simulator will use at startup.

### COMMAND-FILE

While having the ability to set display list items in the simulator is nice, extensive debugging sessions would incur significant startup costs by constantly having to specify your display list items over and over again.

To facilitate matters, a command-file can be specified, which is merely a plain text file, with **one valid prompt command per line**. Unlike what was previously documented, command-files support **all valid prompt commands**, not just display list items.

### COLORS

Specifying colors will colorize some of the output, in an attempt to help highlight important information and distinguish various data items.

### DEREF-ADDR

To aid in debugging and study: setting `--deref-addr` will take any dereferencing or indexed dereferencing instruction, and display the resulting value, alongside the instruction.

### DEBUG

Mostly for `v32sim` development, the `--debug` argument causes more output to be generated, mostly on internal simulator operations, to help explore problems in simulator operation.

### ENTRY-POINT

Set the offset of where the simulator will start processing (overriding the system default BIOS entry point of `0x10000004`).

Intended to be used to force start from the `CART` at `0x20000000`, it should be used carefully.

### ERRORCHECK

Enabling `errorcheck` will cause the simulator to perform a test evaluation before each instruction execution, validating whether or not it is a resource-legitimate transaction. If it detects a problem, it will provide notification on instruction rendering (if colors are enabled, it will be in red vs the normal yellow), and between exclamation points the type of system error that is about to occur (if you execute the instruction).

### PROFILE

Enable instruction and subroutine profiling. The simulator will do a global tally of instructions executed (and which ones, how many times each one has been executed), along with what and how many times a subroutine has been called.

Additionally, localized instruction tallies per subroutine will be reported at subroutine return (especially for nexting over a subroutine call).

At the single-step prompt, running `profile` will display the current profiling report, breaking down the global tallies of everything.

This processing only happens when profiling is enabled. It CAN be toggled during runtime as well via the prompt's `set` command.

### RUN

Do not provide the debugger prompt: just simulate the indicated CART.

### WATCH-FOR

A different sort of breakpoint. More a hacked "watchpoint" on the system IR register on the lookout for the indicated word of machine code. Useful for stopping execution at a particular instruction (assuming it is not a common instruction like a MOV).

## COMMANDS

`v32sim`, when processing has been stopped, will present a `v32sim>` prompt allowing for per-instruction control and reporting of system details.

This is where `v32sim` can be considered a **DEBUGGER**, in that here we can *single step* through the code, instruction at a time, printing out various register, memory, IOPort values. There's even a *displaylist* functionality where you can have things automatically displayed each step, just like in GDB.

### Command Reference


| Command                                 | Description                                                                     |
| --------------------------------------- | ------------------------------------------------------------------------------- |
| `break [0xMEM|LABEL]`                   | set an execution breakpoint (memory address or label)                           |
| `continue`                              | Resume execution until next trigger (breakpoint, HLT, watchpoint)               |
| `print XYZ`                             | One-time display of XYZ (register, memory, IOPort)                              |
| `display XYZ [LABEL]`                   | Add display list item to show XYZ each stop                                     |
| `undisplay #`                           | Remove display list item by index                                               |
| `label [0xMEM_ADDR LABEL]`              | List or set a label for a memory offset                                         |
| `unlabel #`                             | Remove label by index                                                           |
| `load bios:path`                        | Load BIOS file at runtime                                                       |
| `load cart:path`                        | Load CART file at runtime                                                       |
| `load memc:path`                        | Load MEMCARD file at runtime                                                    |
| `unload bios`                           | Unload BIOS from memory                                                         |
| `unload cart`                           | Unload CART from memory                                                         |
| `unload memc`                           | Unload MEMCARD from memory                                                      |
| `next`                                  | Execute current instruction, skip over subroutines                              |
| `step`                                  | Execute current instruction, stop at next                                       |
| `backtrace`                             | Display list of subroutine calls (most recent first)                            |
| `profile`                               | Show profiling report (requires profiling enabled)                              |
| `inventory`                             | Display system resource overview (CARTs/MEMCARDs loaded, space usage)           |
| `ignore`                                | Skip current instruction without processing (IP/IR/IV advance, no cycle update) |
| `replace IP:0xADDR IR:0xINSTR IV:0xIMM` | Replace instruction and immediate at address                                    |
| `replace IR:0xINSTR`                    | Replace instruction register only                                               |
| `replace IR:0xINSTR IV:0xIMM`           | Replace instruction and immediate registers                                     |
| `set NAME=VALUE`                        | Set system feature, register, memory, or IOPort                                 |
| `unbreak #`                             | Remove breakpoint by index                                                      |
| `watch REG OP VALUE [LABEL]`            | Set watchpoint on register with condition                                       |
| `watchlist`                             | Display all active watchpoints                                                  |
| `unwatch #|LABEL`                       | remove the indicated watchlist member                                           |
| `help` / `?`                            | Display command help                                                            |
| `quit`                                  | Exit the simulator                                                              |


### Breakpoints

Running the `break` command, providing either an established **label** or valid **offset** will add a breakpoint to the breaklist. This will cause the simulator to trigger a break to the debugger prompt upon encountering this location.

Running `break` by itself will display the current breaklist. Use `unbreak #` to remove a breakpoint by its index number.

### Watchpoints

The `watch` command allows you to set conditional breakpoints on registers. When the register's value meets the specified condition, execution will break and notify you.

**Syntax:** `watch REG OP VALUE [LABEL]`

- **REG:** Any register (R0-R15, BP, SP, CR, SR, DR, IP, IR, IV)
- **OP:** Comparison operator (=, !=, &lt;, &gt;, &lt;=, &gt;=)
- **VALUE:** Hex value (0x00000000-0xFFFFFFFF)
- **LABEL:** Optional label for the watchpoint

**Examples:**

```
watch SP <= 0x20000000 stack_underflow
watch R0 != 0x00000000
watch BP > 0x2000FFFF stack_overflow
```

Use `watchlist` to view all active watchpoints and `unwatch` to remove them by index or label.

### Display List

The display list shows accumulated values **EACH time** the system stops for input. This supports:

- **Registers:** R0-R15, CR, SR, DR, BP, SP, IP, IR, IV (can be dereferenced with `[]`)
- **Memory:** Any 4-byte address (e.g., `0xAABBCCDD`), can be dereferenced
- **Memory Ranges:** Address ranges (e.g., `0xAABBCCD0-0xAABBCCD3`), can be dereferenced
- **IOPorts:** Any valid IOPort address or symbolic name (e.g., `0xABC`, `GPU_SelectedTexture`)

**Formatting Suffixes:** Append `/N` to display in a specific format:

- `/X` - Uppercase hexadecimal (default)
- `/x` - Lowercase hexadecimal
- `/u` - Unsigned int (decimal)
- `/o` - Octal
- `/f` - Floating point (decimal)
- `/D` - Decode as instruction
- `/d` - Signed int (decimal)
- `/B` - Boolean
- `/b` - Binary

**Example:** `display R0/X my_register`

### Labels

The `label` command allows you to associate human-readable names with memory offsets. This is useful for:

- Setting breakpoints by name
- Navigating code more easily
- Making display/output more readable

**Syntax:**

```
label                    # List all labels
label 0x10000040 main    # Set label 'main' at offset 0x10000040
```

Use `unlabel #` to remove a label by its index.

### Load/Unload

The `load` and `unload` commands allow you to dynamically load and unload system components at runtime:

**Syntax:**

```
load bios:path/to/biosfile.v32
load cart:path/to/cartfile.v32
load memc:path/to/memcfile.v32

unload bios
unload cart
unload memc
```

Primarily useful for swapping MEMCARDs during a session, but can also be used to load/unload BIOS or CART files.

### Replace

The `replace` command provides a "Game Genie"-style functionality, allowing temporary modification of instruction execution without altering memory:

**Syntax:**

```
replace IP:0xADDR IR:0xINSTRUCT IV:0xIMMEDIATE
replace IR:0xINSTRUCT
replace IR:0xINSTRUCT IV:0xIMMEDIATE
```

This replaces the indicated system register(s) with the specified value(s) for a single execution, making it a temporary change rather than a permanent memory modification (like `set` would do).

### Set

The `set` command configures system features and resources during runtime:

**System Settings:**

```
set color=true
set debug=true
set verbose=true
set deref=true
set errorchk=true
set profile=true
```

**Registers:**

```
set R4=0x4004
set IP=0x10000040
```

**Memory:**

```
set 0x00224466=0x71
```

**IOPorts:**

```
set 0x205=7
set GPU_SelectedTexture=-1
set GPU_ClearColor=0xFF0000FF
```

**Screenshot Settings:**

```
set screengrid=true
set boundbox=true
set noextra=true
```

Running `set` with no arguments displays the current settings and their status.

### Backtrace

The `backtrace` command displays a list of subroutine calls from most recent to least recent, helping you understand the call stack and how you arrived at the current execution point.

### Inventory

The `inventory` command provides a system resources overview, displaying:

- Currently loaded CARTs and their status
- MEMCARDs loaded and their usage
- Available space in each memory region
- Texture information for BIOS and CART

### Ignore

The `ignore` command skips the current instruction entirely without any processing. No cycle-count update, no registers altered. Useful for avoiding a known problematic instruction while keeping the session going.

**NOTE:** Could cause runtime problems depending on what is ignored.

### Screenshots

The `screenshot` command outputs the current state of the Vircon32 screen to a PNG file.

**Syntax:**

```
screenshot                    # Default filename: v32sim.DATESTAMP.png
screenshot my_screenshot.png  # Custom filename
```

**Enhancement Settings:**  
The appearance of screenshots can be enhanced with these settings:

- `screengrid` - Toggle grid overlay
- `boundbox` - Toggle bounding box display
- `noextra` - Toggle extra information display

### Profiling

When profiling is enabled (via `-p` or `set profile=true`), the simulator tracks:

- Global instruction execution counts
- Per-instruction breakdown (all 64 opcodes)
- Subroutine call counts and timing
- Localized instruction tallies per subroutine

Run `profile` at the prompt to display the current profiling report.

### Gamepad Simulation

As there is no actual gamepad support integrated into the simulator, all gamepad transactions are controlled via the `gamepad` command at the simulator prompt:


| Command               | Description                                    |
| --------------------- | ---------------------------------------------- |
| `gamepad`             | List current selected gamepad's values         |
| `gamepad1 select`     | Select gamepad 1                               |
| `gamepad1 connect`    | Connect gamepad 1 (allows changes)             |
| `gamepad1 disconnect` | Disconnect gamepad 1                           |
| `gamepad left`        | Toggle `left` on the selected gamepad          |
| `gamepad right`       | Toggle `right` on the selected gamepad         |
| `gamepad up`          | Toggle `up` on the selected gamepad            |
| `gamepad down`        | Toggle `down` on the selected gamepad          |
| `gamepad A`           | Toggle `A` button on the selected gamepad      |
| `gamepad B`           | Toggle `B` button on the selected gamepad      |
| `gamepad X`           | Toggle `X` button on the selected gamepad      |
| `gamepad Y`           | Toggle `Y` button on the selected gamepad      |
| `gamepad L`           | Toggle `L` button on the selected gamepad      |
| `gamepad R`           | Toggle `R` button on the selected gamepad      |
| `gamepad start`       | Toggle `start` button on the selected gamepad  |
| `gamepad select`      | Toggle `select` button on the selected gamepad |


### MEMCARD Support

MEMCARD support is increasingly functional:

- Command-line argument to load a MEMCARD on simulator start (`-M` or `--memcfile`)
- Prompt's `load` and `unload` commands can transact MEMCARDs at runtime
- MEMCARD data is loaded into memory at the appropriate location

**Limitation:** Any changes made to MEMCARD data during simulation are **not** written back out to the MEMCARD file on disk. The file remains read-only for the duration of the session.

No checks are currently done to ensure the file loaded is in fact a proper MEMCARD (V32-MEMC header is not currently checked for, nor is the required file size verified). However, it is locked in to reading the exact file size as indicated in the Vircon32 specifications.

### Help

The `help` command (or `?`) displays a summary of all available commands with brief descriptions. For detailed help on a specific command, use `help <command>` or refer to this documentation.

### Quit

The `quit` command exits the simulator immediately.
