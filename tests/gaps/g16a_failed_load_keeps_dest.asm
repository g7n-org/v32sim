; G16 (not in V32SIMGAPS): a load that fails leaves the destination register alone.
;@ frames=4
  mov R5, 77
  mov R5, [0x00800000]
  hlt
