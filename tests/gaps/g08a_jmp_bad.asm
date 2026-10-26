; G8: a jump to an unreadable address is not an error by itself: the next instruction FETCH fails,
;@ frames=4
; with R1 = target + 1, R2 = the JMP's instruction word, R3 = the JMP's immediate.
  mov R5, 5
  jmp 0x00800000
  hlt
