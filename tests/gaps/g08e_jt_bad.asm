; G8: a taken conditional jump to a bad address (the untaken one before it is harmless).
;@ frames=4
  mov R0, 1
  mov R1, 0
  jt R1, 0x00800000
  jt R0, 0x00800004
  hlt
