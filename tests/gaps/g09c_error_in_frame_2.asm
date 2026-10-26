; G9: the error is in frame 3: wait twice first.
;@ frames=6
  wait
  wait
  mov R1, 1
  mov R2, 0
  imod R1, R2
  hlt
