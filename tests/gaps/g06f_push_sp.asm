; G6: PUSH SP pushes the value SP had before the push.
;@ frames=4
  push SP
  mov R1, [SP]
  mov [0x20], R1
  mov [0x21], SP
  hlt
