; G10: CMPS with a huge count ends at the first difference, so it does not run off the end of RAM.
;@ frames=8 dump=300@0x200
  mov R13, 0x400
  mov R12, 3
  mov R11, 8
  sets
  mov R13, 0x500
  mov R12, 3
  mov R11, 8
  sets
  mov R3, 4
  mov [0x505], R3
  mov R13, 0x400
  mov R12, 0x500
  mov R11, 0x7FFFFFFF
  cmps R0
  mov [0x200], R0
  mov [0x201], R11
  mov [0x202], R12
  mov [0x205], R13
  in R4, TIM_CycleCounter
  mov [0x203], R4
  in R4, TIM_FrameCounter
  mov [0x204], R4
  hlt
