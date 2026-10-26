; G6: CALL with SP just above a ROM address: InvalidMemoryWrite (1).
;@ frames=4
  mov SP, 0x10000020
  call __f
  hlt
__f:
  ret
