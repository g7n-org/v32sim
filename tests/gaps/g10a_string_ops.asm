; G10: MOVS / SETS / CMPS execute one word per cycle and CMPS stops at the first difference.
; DR = R13, SR = R12, CR = R11. The results are dumped from RAM[0x200].
;@ frames=8 dump=300@0x200 name=basic
  mov R13, 0x400
  mov R12, 7
  mov R11, 16
  sets
  mov R13, 0x500
  mov R12, 0x400
  mov R11, 16
  movs
  mov R13, 0x500
  mov R12, 0x400
  mov R11, 16
  cmps R0
  mov [0x200], R0
  mov [0x201], R11
  mov [0x202], R12
  mov [0x203], R13
  mov R3, 9
  mov [0x503], R3
  mov R13, 0x500
  mov R12, 0x400
  mov R11, 16
  cmps R0
  mov [0x204], R0
  mov [0x205], R11
  mov [0x206], R12
  mov [0x207], R13
  in R4, TIM_CycleCounter
  mov [0x208], R4
; count zero or negative: the reference still does one word, then stops
  mov R13, 0x600
  mov R12, 5
  mov R11, 0
  sets
  mov [0x209], R13
  mov [0x20A], R11
  mov R11, -4
  sets
  mov [0x20B], R13
  mov [0x20C], R11
  mov R12, 0x400
  mov R11, -4
  movs
  mov [0x20D], R13
  mov [0x20E], R12
  mov [0x20F], R11
  mov R11, -4
  cmps R1
  mov [0x210], R1
  mov [0x211], R11
  in R4, TIM_CycleCounter
  mov [0x212], R4
  hlt
