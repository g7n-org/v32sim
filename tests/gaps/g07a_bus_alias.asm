; G7: bits 30 and 31 of an address are ignored by the memory bus: 0x40000020 is RAM 0x20,
;@ frames=4
; 0x90000000 is the BIOS (word 0), 0xE0000000 the cartridge (word 0).
  mov R0, 0x5555
  mov [0x40000020], R0
  mov R1, [0x00000020]
  mov [0x21], R1
  mov R2, [0x90000000]
  mov [0x22], R2
  mov R3, [0xE0000000]
  mov [0x23], R3
  mov R0, 0x6666
  mov [0xC0000024], R0
  mov R4, [0x24]
  mov [0x25], R4
  hlt
