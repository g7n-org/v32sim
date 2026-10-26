; G4: the date at midnight. Days count from 0. A year has 365 days, or 366 when it is a
; multiple of 4 and not of 100, or a multiple of 400.
; Each frame: the time, then the date; 70 frames cross the end of the day (frame 60).
;@ frames=70 dump=140@40 date=2023,364 time=86399 name=2023-end
;@ frames=70 dump=140@40 date=2024,364 time=86399 name=2024-leap-not-last
;@ frames=70 dump=140@40 date=2024,365 time=86399 name=2024-leap-last
;@ frames=70 dump=140@40 date=2000,365 time=86399 name=2000-400-rule
;@ frames=70 dump=140@40 date=1900,364 time=86399 name=1900-not-leap
;@ frames=70 dump=140@40 date=2100,364 time=86399 name=2100-not-leap
;@ frames=70 dump=140@40 date=2026,363 time=86399 name=ordinary-day
  mov R1, 0x40
  mov R5, 70
__frame:
  in R0, TIM_CurrentTime
  mov [R1], R0
  in R0, TIM_CurrentDate
  mov [R1+1], R0
  iadd R1, 2
  isub R5, 1
  wait
  jt R5, __frame
  hlt
