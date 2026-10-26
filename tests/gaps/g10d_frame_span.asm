; G10: a long SETS goes on across frame boundaries (3M cycles per frame): each word costs a cycle.
;@ frames=6 dump=300@0x100
  mov R4, 0
__again:
  mov R13, 0x1000
  mov R12, 1
  mov R11, 900000
  sets
  iadd R4, 1
  mov R5, R4
  ilt R5, 4
  jt R5, __again
  in R3, TIM_FrameCounter
  mov [0x100], R3
  in R3, TIM_CycleCounter
  mov [0x101], R3
  mov R0, [0x1000]
  mov [0x102], R0
  hlt
