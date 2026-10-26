; G6: CALL with SP = 0: the push decrements SP below 0, StackOverflow (4); IP not yet changed.
;@ frames=4
  mov SP, 0
  call __f
  hlt
__f:
  ret
