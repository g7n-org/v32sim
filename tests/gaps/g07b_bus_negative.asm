; G7: address -1 = 0xFFFFFFFF is device 3 (the memory card), offset 0x0FFFFFFF: there is no card,
;@ frames=4
; so this is an ordinary InvalidMemoryRead.
  mov R5, 77
  mov R5, [0xFFFFFFFF]
  hlt
