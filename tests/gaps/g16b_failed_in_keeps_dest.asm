; G16: so does an IN from a port that does not exist (no labels after a numeric port).
;@ frames=4
  mov R5, 77
  in R5, 0x7FF
  hlt
