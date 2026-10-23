# TODO

Status as of version `20261018-dev` plus the v32dev / v32io work.

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
  * **MEMC** - header and size validated; changes written at every frame end;
    live **v32dev mailbox** (heartbeat, host area read back each frame,
    commit-ordered writes); `mailbox` command.
  * **v32io keyboard** - `v32kbd` on a gamepad port, `kbd` command,
    `--v32kbd`, `--kbd-input` (terminal / pipe / FIFO / file).
  * **Unattended runs** - `--fast`, `--frame-us`, `--max-frames`, `-x`.
  * **`load bios:PATH`**, **`replace`** refreshes IR/IV, **float disassembly**
    prints shortest round-trip values.
  * **Tests** - `tests/run_tests.sh` (v32dev suite, v32kbd, persistence).
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

  * **Audio** - the SPU has no output; a WAV capture of the mixed channels
    would be a natural first step.
  * **Debug-file loader** - `load_labels()` is hand-rolled; it could be moved
    onto flex.
  * **Entry point** - should the default start be the cartridge
    (`0x20000000`) rather than the BIOS?
  * **Text output** - a terminal for v32nix/V32DOS needs the screen as text:
    either the v32dev `serial:stdio` channel (works now, with the host tool),
    or capturing BIOS-font `DrawRegion` calls into a character grid.
  * **v32mouse** - the protocol is not emulated (only the keyboard).
  * **Cycle accuracy** - `MOVS`/`SETS`/`CMPS` run to completion as one step
    (the console runs one word per cycle).
