; G16: POP with a valid read but a following underflow keeps the loaded value.
;@ frames=4
  mov R5, 77
  mov SP, 0x003FFFFF
  mov R0, 0x4321
  mov [0x003FFFFF], R0
  pop R5
  hlt
