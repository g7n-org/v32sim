; G8: RET to a bad address: the pop succeeds, the next fetch fails.
;@ frames=4
  mov R0, 0x00A00000
  push R0
  ret
  hlt
