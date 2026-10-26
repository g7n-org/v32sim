; G7: writing 0x90000000 (an alias of the BIOS) is a write to ROM: InvalidMemoryWrite (1).
;@ frames=4
  mov R0, 1
  mov [0x90000000], R0
  hlt
