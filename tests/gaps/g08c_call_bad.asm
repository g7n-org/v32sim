; G8: CALL to a bad address: the return address is pushed, then the fetch fails.
;@ frames=4
  call 0x00800000
  hlt
