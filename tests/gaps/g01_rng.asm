; G1: RNG_CurrentValue. The console runs a minstd generator that starts at 1; a write of 0
; is ignored, any other write is stored with its sign bit cleared. Writing 0x80000000
; therefore stores 0, which sticks (the console does not guard against it).
;@ frames=3 dump=24@20
  mov R1, 0x20
  mov R2, 6
  call __read           ; from reset: 1, 48271, 182605794, 1291394886, 1914720637, 2078669041
  out RNG_CurrentValue, 12345
  mov R2, 3
  call __read           ; 12345, 595905495, 1558181227
  out RNG_CurrentValue, 0
  mov R2, 2
  call __read           ; the write was ignored: the sequence just goes on
  out RNG_CurrentValue, -5
  mov R2, 3
  call __read           ; stored as 0x7FFFFFFB
  out RNG_CurrentValue, 0x7FFFFFFF
  mov R2, 3
  call __read           ; 2147483647, then 0 forever
  out RNG_CurrentValue, 0x80000000
  mov R2, 3
  call __read           ; sign bit cleared: 0
  out RNG_CurrentValue, 7
  mov R2, 2
  call __read
  hlt

__read:
  in R0, RNG_CurrentValue
  mov [R1], R0
  iadd R1, 1
  isub R2, 1
  jt R2, __read
  ret
