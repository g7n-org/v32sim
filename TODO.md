# TODO

Status as of version `20261018-dev`.

## DONE (removed from this list)

  * **Command parsing** - regex front end replaced by `lexer.l`, `parser.y`
    and `command.c`.
  * **Version** - `VERSION` in `inc/version.h`; `--version`/`-V`.
  * **`ignore`** - skips the current instruction.
  * **Watchpoints** - registers, IOPorts and memory words, compared with a
    number, a register, an IOPort or a memory word; edge-triggered, checked
    after every instruction and on state changes.
  * **GPU framebuffer and `screenshot`** - software renderer, PNG writer.
  * **SPU and VSND** - channel/sound model and loading of VSND data; no audio.
  * **MEMC** - header and size validated, changes written back.
  * **TIM leap years**.
  * **Debug file location** - directory of the V32 file, current directory,
    `obj/`, `../obj/`, and `--debug-dir`; loaded automatically (`-n` disables).
  * **Assembly and C overlay** - `--source`, `set source`, `set mode`.
  * **SIGSEGV** (and SIGBUS/SIGFPE/SIGABRT) state dump.
  * **Save / restore**.
  * **Live assembly** - `asm` (flex/bison, verified against the whole BIOS).
  * **CPU audit** against DesktopEmulator: `SETS` stored the register number
    instead of the register's value, `SHL` used an arithmetic right shift,
    `MOVS`/`SETS`/`CMPS` counted CR differently, `ACOS`/`ATAN2`/`LOG`/`POW`
    did not raise their errors, the error-handler reset SP/BP to 0 and R1
    held the instruction's own address, and `print/D` showed float immediates
    converted instead of reinterpreted.

## STILL LACKING

  * **`load bios:PATH`** - parsed, reports "not supported".
  * **Audio** - the SPU has no output; a WAV capture of the mixed channels
    would be a natural first step.
  * **`replace`** patches memory but not the instruction already fetched for
    the current step (`asm` refreshes it; decide whether `replace` should).
  * **Debug-file loader** - `load_labels()` is hand-rolled; it could be moved
    onto flex.
  * **Disassembly of floats** - immediates print with 3 decimals, which does
    not round-trip through `asm`; consider printing shortest-exact values.
  * **Entry point** - should the default start be the cartridge
    (`0x20000000`) rather than the BIOS?
  * **Tests** - no automated suite. The scenario scripts used for the
    flex/bison change, the BIOS disassemble/assemble round trip, and the
    save/restore round trip are good starting points.
  * **Cycle accuracy** - `MOVS`/`SETS`/`CMPS` run to completion as one step
    (the console runs one word per cycle).
