; G6: PUSH with SP in ROM. The console writes the word and the bus refuses: InvalidMemoryWrite (1),
;@ frames=4
; not StackOverflow (that is only SP < 0).
  mov R5, 77
  mov SP, 0x10000010
  push R5
  hlt
