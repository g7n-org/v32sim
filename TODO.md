# TODO

Status as of version `20261018-dev`, after the move of the command front
end to flex/bison.

## DONE (removed from this list)

  * **Registers use access functions** - `reg_get()` / `reg_set()` exist
    in `register.c`.
  * **Watchpoints** - register watchpoints (`==`, `!=`, `<`, `<=`, `>`,
    `>=` against an immediate) work: `watch`, `watchlist`, `unwatch`.
    The label is optional.
  * **Command parsing** - regex front end replaced by `lexer.l`,
    `parser.y` (parse) and `command.c` (execute).
  * **Version** - `VERSION` in `inc/version.h`; `--version`/`-V`, banner,
    usage, `version` prompt command, `make version`.

## STILL LACKING

### Commands

  * **`ignore`** still behaves like `continue`. It should skip the
    current instruction (advance `rom_offset` past the instruction and
    its immediate, without executing). About three lines in the
    `INPUT_IGNORE` case of `prompt()`.
  * **`screenshot`** - parsed, prints "not implemented yet". Needs the
    GPU framebuffer (see below) and a PNG writer (libgd, or libpng).
  * **`load bios:PATH`** - parsed, reports "not supported".
  * **Save / restore state** - save RAM, registers and IOPorts to a file;
    store a checksum of the CART (computed in `load_memory()`) so a state
    cannot be restored against a different cartridge.
  * **`replace`** patches memory but not the instruction already fetched
    for the current step (takes effect on the next arrival at that
    address). Decide whether that is wanted or should re-fetch.

### Watchpoints

  * Register-only. Memory and IOPort watches are not implemented.
  * Value is an immediate only; comparing against another register,
    memory or IOPort contents is not implemented.
  * Checked once per instruction in the main loop, not inside
    `reg_set()`/`memory_set()`/`ioports_set()`, so a value that changes
    and changes back within one instruction is missed.

### Hardware

  * **SPU** - ports store and return values; no sound engine.
  * **GPU rendering** - texture, region and drawing ports work, but there
    is no framebuffer. Needed for `screenshot`. VTEX data is stored
    stripped of headers; a VTEX -> PNG path (see `vircon2png` in the
    Vircon32 DevTools, libpng / libgd `gdImageCreateFromPngPtr()`) is
    still to do.
  * **VSND** - sound data is not loaded into memory by `load_memory()`
    (`num_vsnd` is read; the offsets are skipped). Same approach as VTEX.
  * **MEMC write-back** - MEMCARD changes are lost on exit.
    `unload_memory()` should save them to the memcfile (keep `memcfile`
    in `mem_t`). The V32-MEMC header is not validated.
  * **TIM_TIMEDAY** - day-of-year ignores leap years
    (`update_sys.c`, "compensate for leap years").

### Debug information

  * **Debug file location** - only the current directory is searched.
    Also try `obj/` and `../obj/`, and add a command-line option.
  * **Assembly overlay** - labels, offsets and line numbers are read from
    the debug file, but the matching source line is not displayed in
    place of the decoded instruction.
  * **C overlay** - `.asm.debug` carries the C line numbers; show the C
    line for the current instruction. `-L C` currently only
    selects the C-style label files; no C source is displayed.
  * The CSV debug-file loader (`load_labels()`) is hand-rolled; it could
    be moved onto flex now that the build has it.

### Live assembly injection

  Specify assembly text, encode it, and execute or patch it in. This is
  now practical: `lexer.l`/`parser.y` give the build the machinery, and
  `assembly.c` has a header describing the plan (the old regex design is
  kept there, commented out, as a sketch). `gbtov32`
  (`doc/v32asm_reference.md`) is the reference for the instruction
  syntax and operand modes. The decoder (`decode.c`) is the inverse.

### Miscellaneous

  * **Entry point** - should the default start be the cartridge
    (`0x20000000`) rather than the BIOS?
  * **SIGSEGV** - only SIGINT is trapped. A SIGSEGV handler should print
    some simulator state (IP, instruction, last access) before dying.
  * **Tests** - there is no automated test suite. The old-vs-new
    scenario scripts used for the flex/bison change (step, next,
    continue, breakpoints, watchpoints, replace against
    `cart/09_hello.v32`) are a good starting point.
  * **Instruction coverage** - all 64 opcodes are implemented, but not
    all are exhaustively tested, and port modes/quantities have not been
    fully cross-checked against the Vircon32 reference.
