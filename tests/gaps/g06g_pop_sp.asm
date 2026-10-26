; G6: POP SP loads SP from the stack, and only then increments it.
;@ frames=4
  mov R0, 0x1234
  push R0
  pop SP
  mov [0x20], SP
  hlt
