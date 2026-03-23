# TODO

## ENTRY POINT

Should default entry point be at `CART` start of `0x20000000`?

## SIGNALS (SIGINT)

Trap SIGINT (breaks when in runmode)

## TRANSITION REGISTERS TO USE ACCESS FUNCTIONS

Like memory  and ioports, have  registers also  use a `_get`  and `_set`;
could be helpful when watchpoints are implemented.

## SAVE / RESTORE COMMANDS IN PROMPT

Save states!

The idea  is to  save to file  the current state  of RAM,  registers, and
ioports. Probably also  tie in some check against CART  and MEMC state so
that you cannot restore a save state for an entirely different cartridge.

Perhaps some simple checksum of the CART? Could do that in load_memory()!

## IMPLEMENT OTHER IOPORTS

  * `SPU_` - currently zero functionality

### GPU PORTS

Texture, Region,  and Region  definition ports  are now  functioning. The
VTEX data  is now properly  being loaded  into memory, stripping  out the
headers.

## MEMC

Functional MEMC support is online, but  changes will be lost on simulator
exit. The  `unload_memory()` function  needs to be  updated to  save MEMC
data back to the loaded memcfile.

I will likely need to store `memcfile`  in `mem_t` to have it on hand for
such operations.

## VSND DATA

No VSND  data is  currently being  loaded into  memory. Support  for that
still needs to be added to `load_memory()`.

The infrastructure is in  place to do that, via my  work on VTEX loading,
which is a largely similar process.

## FINISH IMPLEMENTING TIM_TIMEDAY

Currently does not handle leap years/days.

## VARIED DEBUG FILE LOCATION CHECK

Currently  only   checks  current  directory,  should   also  check  obj/
directory:

  * `.` - current only location it checks (where .v32 is read from)
  * `obj/` - should also check
  * `../obj/` - should also check
  * implement a command-line argument to specify

## WATCHPOINTS

Idea for watchpoints:

  * linked list ID'ing register, memory, or ioport.
  * comparison operator (standard `==`, `!=`, `<`, `<=`, `>`, and `>=`)
  * value to watch for:
    * immediate value
	* register contents
	* memory address contents
	* ioport contents

The  various  `_get` and  `_set`  functions  for registers,  memory,  and
ioports need  to perform  a sweep through  the watchlist,  performing any
checks for the entries that pertain to them.

## ASSEMBLY CODE OVERLAY

With the  offsets, line numbers, and  labels loaded from the  debug file,
look up the actual line of assembly and display it instead of our decoded
output.

## C CODE OVERLAY

Access the .asm.debug to also get the  lines from the C file, and display
the corresponding line of C code.

## LIVE ASSEMBLY INJECTION

Specify actual  assembly code,  that gets encoded  into machine  code and
then executed.
