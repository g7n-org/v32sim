; G15: before any immediate has been fetched by the cartridge R3 still holds the BIOS JMP's one.
;@ frames=4
  mov R1, R3
  mov R2, 0
  idiv R1, R2
  hlt
