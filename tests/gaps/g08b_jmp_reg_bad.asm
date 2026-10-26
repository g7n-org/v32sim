; G8: the same through a register (JMP R0 has no immediate: R3 keeps the one from the MOV before).
;@ frames=4
  mov R0, 0x00900000
  jmp R0
  hlt
