; G15 (not in V32SIMGAPS): R3 receives the CPU's immediate register, which still holds the last
;@ frames=4
; immediate word fetched; an instruction without an immediate does not clear it.
  mov R6, 0xABCD
  mov R1, 1
  mov R2, 0
  idiv R1, R2
  hlt
