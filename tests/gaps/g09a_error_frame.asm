; G9: a hardware error ends the frame: the handler's first instruction runs at the start of the
;@ frames=4
; next one. (The handler stores TIM_FrameCounter and TIM_CycleCounter a few cycles in.)
  mov R1, 1
  mov R2, 0
  idiv R1, R2
  hlt
