; G6: POP with SP in ROM. The read succeeds (R5 gets the BIOS word), SP is then past RAM:
;@ frames=4
; StackUnderflow (5) is raised after the register was written.
  mov R5, 77
  mov SP, 0x10000010
  pop R5
  hlt
