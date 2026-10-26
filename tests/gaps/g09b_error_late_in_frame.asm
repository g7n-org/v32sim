; G9: same, after a long loop: the error happens late in frame 1, the handler runs in frame 2.
;@ frames=5
  mov R4, 10000
__loop:
  isub R4, 1
  jt R4, __loop
  mov R1, 1
  mov R2, 0
  idiv R1, R2
  hlt
