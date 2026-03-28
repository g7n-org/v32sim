# v32sim

A  text-based  Vircon32  simulator/debugger,  predominantly  for  use  in
debugging/hacking/studying the system.

The aim  is to implement enough  of the Vircon32 Fantasy  Console so that
code execution can be studied. Obviously, lacking an actual screen output
(or any of the other devices, like audio), it is not meant as any sort of
replacement.

At this  point, all  instructions have been  implemented (but  many still
need testing);  registers and memory are  present and should behave  in a
manner similar to how the system operates.

## IOPORTS

All IOPorts are available within the simulator. However, not all ports in
each category are functionally accurate.

For example: the SPU ports are not involved in any processing. Any values
written to them or read from them are just that: standalone values. There
will be no change to that data.

Some other  ports do  have established functionality:  all the  TIM, RNG,
INP, CAR, and MEM ports should actually behave as expected.

Furthermore, some of  the GPU ports are now  functional, especially those
related to textures, regions, and their definitions. All textures present
in a V32 file will be loaded into memory in the simulator.

### TEXTURES AND REGIONS

Outside of  BIOS and CART code  that manipulates the GPU  ports, from the
simulator prompt  one can  manipulate the texture  and region  GPU ports,
through the use of the `set` command and `display` or `print` commands.

Also,  the `inventory`  command  will display  the currently  established
texture information (resolution, offsets) in the BIOS and CART.

Many GPU ports should be fully functional:

  * GPU_ClearColor
  * GPU_SelectedTexture
  * GPU_SelectedRegion
  * the MinX/Y, MaxX/Y, and HotspotX/Y ports

The simulator functionally supports distinct  regions per texture, and of
course many  textures (based  on how  many are present  in the  CART when
loaded- BIOS is fixed).

VTEX data is also properly loaded into memory.

The other GPU ports  may well be able to be read  from/written to, but if
there is deeper functionality, they currently  just serve as a storage of
information (nothing is yet done with that information).

### GAMEPADS

As there is no actual gamepad support integrated into the simulator,  any
and all gamepad transactions are controlled via the `gamepad` command  at
the simulator prompt:

  * `gamepad` - will list the current selected gamepad's values
  * `gamepad1 select` - will select gamepad 1
  * `gamepad1 connect` -  will connect gamepad 1 (allows changes)
  * `gamepad1 disconnect` -  disconnects gamepad 1
  * `gamepad left` - toggle `left` on the selected gamepad
  * `gamepad L` - toggles `L` button on the selected gamepad

## MEMCARD

MEMCARD support is  increasingly functional: there is  now a command-line
argument to  load a MEMCARD on  simulator start; the prompt's  `load` and
`unload` commands can transact MEMCARDs.  And MEMCARD data is loaded into
memory at  the appropriate place.  But: any  changes are not  yet written
back out to the MEMCARD file on disk.

No checks  are currently  done to  ensure the  file loaded  is in  fact a
proper MEMCARD (V32-MEMC header is not  currently checked for, nor is the
required file size  verified). But, it is locked in  to reading the exact
filesize as indicated in the Vircon32 specifications.

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
 -E, --entry-point=OFFSET  set simulator entry point
 -e, --errorcheck          enable runtime error checking
 -M, --memcfile=FILE       load this file as a MEMCARD
 -n, --no-debug            do not process any debug files
 -r, --run                 do not enable single-step mode
 -w, --watch-for=OPCODE    run until OPCODE is encountered
 -v, --verbose             enable more verbose output
 -h, --help                display this information

FILE   is any path plus the filename desired
OFFSET is the full 32-bit/4-byte memory addres (hex)
OPCODE is the full 32-bit/4-byte instruction hex
```

The simulator will, by default,  use the Vircon32 `StandardBios.v32` from
the  standard  location.  This  can  be  changed  with  the  `--biosfile`
argument, should you wish to use a different BIOS.

To disable the "debugger", run with `--run`, it will just be a simulator.

## DEPENDENCIES

### GNU READLINE

GNU readline is used for input prompt processing, allowing for shell-like
input management (cursor keys, CTRL keys, command history, etc.).

NOTE: on macOS, there does not seem to be a separate `history` library; a
tweak has been added to the Makefile to avoid an error while building  on
macOS.

### POSIX REGEX

The  simulator  currently  makes  extensive  use  of  the  POSIX  Regular
Expressions functions.

### MATH

For some of the higher level math operations, the math library is used.

## ARGUMENTS

### BIOSFILE

By default,  `v32sim` attempts read `StandardBios.v32`  from the standard
Vircon32 ComputerSoftware install location on the system.

If desired, you can specify an alternate BIOS that the simulator will use
at startup.

### COMMAND-FILE

While having  the ability to set  display list items in  the simulator is
nice, extensive debugging sessions  would incur significant startup costs
by constantly  having to specify  your display  list items over  and over
again.

To facilitate matters, a command-file can be specified, which is merely a
plain text  file, with one  valid prompt  command per line  (only display
list items are supported in command-files)

### COLORS

Specifying colors will colorize some of the output, in an attempt to help
highlight important information and distinguish various data items.

### DEREF-ADDR

To  aid in  debugging and  study:  setting `--deref-addr`  will take  any
dereferencing  or  indexed  dereferencing instruction,  and  display  the
resulting value, alongside the instruction.

### DEBUG

Mostly  for  `v32sim` development,  the  `--debug`  argument causes  more
output to be generated, mostly  on internal simulator operations, to help
explore problems in simulator operation.

### ENTRY-POINT

Set the offset  of where the simulator will  start processing (overriding
the system default BIOS entry point of `0x10000004`.

Intended to  be used to force  start from the `CART`  at `0x20000000`, it
should be used carefully.

### ERRORCHECK

Enabling  `errorcheck` will  cause  the simulator  to  a test  evaluation
before  each instruction  execution, validating  whether or  not it  is a
resource-legitimate transaction. If it detects a problem, it will provide
notification on instruction rendering (if  colors are enabled, it will be
in red vs the normal yellow),  and between exclamation points the type of
system error that is about to occur (if you execute the instruction).

### RUN

Do not provide the debugger prompt: just simulate the indicated CART.

### WATCH-FOR

A different sort of breakpoint. More a hacked "watchpoint" on the  system
IR register on the lookout for the indicated word of machine code. Useful
for stopping execution at a particular instruction  (assuming it is not a
common instruction like a MOV).

## COMMANDS

`v32sim`, when  processing has  been stopped, will  present a  `v32sim> `
prompt  allowing  for per-instruction  control  and  reporting of  system
details.

This is where `v32sim` can be  considered a **DEBUGGER**, in that here we
can *single step*  through the code, instruction at a  time, printing out
various  register, memory,  IOPort values.  There's even  a *displaylist*
functionality  where you  can  have things  automatically displayed  each
step, just like in GDB.

### BREAK

Running the `break` command, providing either an established **label** or
valid **offset** will add a breakpoint to the breaklist.

This will cause the simulator to trigger a  break to the debugger  prompt
upon encountering this location.

Running `break` by itself will display the breaklist.

Using `unbreak` with a breaklist value should remove it from the list.

### CONTINUE

Running the `continue` command will resume standard execution (which will
keep going until encountering a HLT).

### DISPLAY

Display list:  show the accumulated list  of values EACH time  the system
stops for input, which supports:

  * register:
    * `R0`-`R15`: general purpose register
    * register aliases supported: `CR`, `SR`, `DR`, `BP`, `SP`
    * all standard registers can be displayed with "regs"
    * system registers (`IP`, `IR`, and `IV`) are also supported
    * can also be dereferenced by wrapping in square brackets
  * memory:
    * `0xAABBCCDD`: any 4-byte address
    * can also be dereferenced by wrapping in square brackets
  * memory range:
    * `0xAABBCCD0-0xAABBCCD3`
    * can also be dereferenced by wrapping in square brackets
  * ioport:
    * `0xABC`: any valid IOPort address
    * `CAT_PortDescription`: any symbolic IOPort name

An optional **label** can also be provided, which will be suffixed to the
display of that particular display point information.

The system registers  will be displayed *BEFORE* the  instruction (and be
updated to reference that instruction), whereas all the other values will
be displayed  *AFTER* the instruction,  and not yet updated  (because the
instruction hasn't run yet).

An optional formatting can be applied to a display list item, simpy by an
indication after the display command itself. Suffixing "/N", where "N" is
the one-letter formatting option, will cause the indicated item to render
in the desired formatting:

  * `/X` - uppercase hexadecimal (also the default, if not other default)
  * `/x` - lowercase hexadecimal
  * `/u` - unsigned int (decimal)
  * `/o` - octal
  * `/f` - floating point (decimal)
  * `/D` - decode as instruction
  * `/d` - signed int (decimal)
  * `/B` - boolean
  * `/b` - binary

NOTE: if you provide no label to a displayed IOPort, it will display  the
symbolic name of the port as the label.

### IGNORE

Cause the current instruction to  be entirely skipped, with no processing
performed.  No  cycle-count update,  no  registers  altered. No  nothing.
Useful for  avoiding a  known problematic  instruction while  keeping the
session going.

### NEXT

Running the `next` command will process the current instruction, skipping
over any subroutine calls, then stop and ask for the next command.

### PRINT

One-time printing of indicated value, any one of:

  * `R0`: identified register
  * `[R0]`: dereferened register
  * `0x003FFFFF`: specified memory address
  * `[0x003FFFFF]`: dereferenced memory address
  * `0x205`: IOPort
  * `IP`: system instruction pointer (supports `IP`, `IR`, and `IV`)

Also supports the formatting suffixes mentioned in `display`.

### REPLACE

Replace the  indicated instruction with  the provided hex  value (machine
code). In some ways, you can think of this like a "Game Genie" mode,  and
the "codes" are the direct machine code/binary words that would be stored
in memory.

There are three variants planned:

  * replace just the instruction (`IR` register contents) at `IP`
  * replace the `IR` and `IV` register contents at `IP`
  * at *specified offset*, replace `IR` and `IV`

NOTE: the full `0x`-prefixed, 4-byte hex value needs to be specified.

In  all cases,  the data  in memory  will be  overwritten, so  any future
encounters of that instruction/immediate value  at the offset (current or
specified) will be updated for  the remainder of that simulator/debugging
session.

### SET

The `set` command can be used to set config values and  system  resources
during simulator runtime:

  * color: set color output (true or false)
  * debug: set debug output (true or false)
  * verbose: set debug output (true or false)
  * deref: set deref_addr (true or false)
  * errorchk: set errorcheck (true or false)

You can set registers to desired content:

`set R4=0x4004`

Same with memory addresses:

`set 0x00224466=0x71`

Or with IOPorts:

`set 0x205=7`
`set GPU_SelectedTexture=-1`

Then, combined with a `print` or `display`, see the results of your work.

This will impact instructions running on the simulator, since you are  in
fact changing the values on the v32 system itself.

Running `set`  with no  arguments will display  the current  settings and
their status.

### STEP

Running the  `step` command  will process  the current  instruction, then
stop and ask for the next command.
