; G2, G3, G4: frame counter, cycle counter and clock as seen by the program. Four words per
; frame: the cycle counter (second instruction of the frame, so 2 on the console), the
; frame counter, the time and the date. The clock starts 100 s into day 100 of 2026.
;@ frames=130 dump=520@40 date=2026,100 time=100 name=mid-year
;@ frames=130 dump=520@40 date=2026,0 time=0 name=day-zero
  mov R1, 0x40
  mov R5, 130
__frame:
  in R0, TIM_CycleCounter
  mov [R1], R0
  in R0, TIM_FrameCounter
  mov [R1+1], R0
  in R0, TIM_CurrentTime
  mov [R1+2], R0
  in R0, TIM_CurrentDate
  mov [R1+3], R0
  iadd R1, 4
  isub R5, 1
  wait
  jt R5, __frame
  hlt
