; G16: a store to a bad address, then the registers: nothing else changes.
;@ frames=4
  mov R5, 77
  mov R6, 0x00800000
  mov [R6+4], R5
  hlt
