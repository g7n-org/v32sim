; writes a word to the memory card, then waits forever
  mov R0, 0xC0DE1234
  mov [0x30000010], R0
_spin:
  wait
  jmp _spin
