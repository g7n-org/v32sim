# v32sim

A  text-based  Vircon32  simulator/debugger,  predominantly  for  use  in
debugging/studying the system.

The aim  is to implement enough  of the Vircon32 Fantasy  Console so that
code execution can be studied. Obviously, lacking an actual screen output
(or any of the other devices, like audio), it is not meant as any sort of
replacement.

At this  point, many/most? instructions have  been implemented; registers
and memory are present  and should behave in a manner  similar to how the
system operates.

Many  IOPorts  are  present,  if  only  for  simple  reporting  or  basic
operability.  The  TIM_,   RNG_,  and  CAR_  ports   should  actually  be
fully functional.

## USAGE

```
Usage: v32sim [OPTION]... CARTFILE.v32
Debugger/Simulator for Vircon32 Fantasy Console

Mandatory arguments to long options are mandatory for short options too.

 -B, --biosfile=FILE    load this BIOS V32 file as BIOS
 -i, --index-math       output index math after decode
 -r, --run              do not enable single-step mode
 -s, --seek-to=OFFSET   run until OFFSET is encountered
 -v, --verbose          enable more verbose output
 -h, --help             display this information
```

The simulator will, by default,  use the Vircon32 `StandardBios.v32` from
the  standard  location.  This  can  be  changed  with  the  `--biosfile`
argument, should you wish to study the BIOS code.

To disable the "debugger", run with `--run`, it will just be a simulator.

## DEPENDENCIES

### GNU READLINE

GNU readline is used for input prompt processing, allowing for shell-like
input management (cursor keys, CTRL keys, command history, etc.).

## ARGUMENTS

## COMMANDS

`v32sim`, when  processing has  been stopped, will  present a  `v32sim> `
prompt  allowing  for per-instruction  control  and  reporting of  system
details.

This is where `v32sim` can be  considered a **DEBUGGER**, in that here we
can *single step*  through the code, instruction at a  time, printing out
various  register, memory,  IOPort values.  There's even  a *displaylist*
functionality  where you  can  have things  automatically displayed  each
step, just like in GDB.

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
  * memory:
    * 0xAABBCCDD: any 4-byte address
  * ioport:
    * 0xABC: any valid IOPort address

An optional **label** can also be provided, which will be suffixed to the
display of that particular display point information.

The system registers  will be displayed *BEFORE* the  instruction (and be
updated to reference that instruction), whereas all the other values will
be displayed  *AFTER* the instruction,  and not yet updated  (because the
instruction hasn't run yet).

### MEMORY

Removed with recent  improvements. Some functionality needs  to be folded
in. Kept for reference.

Running `memory` allows for the display  of a specified memory address, a
range of memory  addresses, or will provide a progressing  list of memory
addresses (and their contents):

  * `memory 0x1000004`: display indicated memory address
  * `memory 0x003FFF0-0x003FFFF`: display indicated range of addresses
  * `memory`: will display 8 instructions (previous 4 through next 4)

### NEXT

Running the `next` command will process the current instruction, skipping
over any subroutine calls, then stop and ask for the next command.

### PRINT

One-time printing of indicated value, any one of:

  * `R0`: identified register
  * `0x003FFFFF`: specified memory address
  * `0x205`: IOPort
  * `IP`: system instruction pointer (supports `IP`, `IR`, and `IV`)

### STEP

Running the  `step` command  will process  the current  instruction, then
stop and ask for the next command.
