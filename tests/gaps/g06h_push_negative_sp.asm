; G6: PUSH with SP below 0: StackOverflow (4).
;@ frames=4
  mov R3, 5
  mov SP, -5
  push R3
  hlt
