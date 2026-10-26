; G13: the control bus uses only bits 0-10 of a port number (device = bits 8-10, bits 11 and
; above are ignored), so 0x900 is RNG_CurrentValue and 0x1200 is GPU_Command, with their full
; behaviour. No labels after the numeric ports: the assembler counts such an IN/OUT as two words.
;@ frames=3 dump=16@20
  out RNG_CurrentValue, 99
  in R0, 0x900              ; alias of RNG_CurrentValue: 99 ...
  mov [0x20], R0
  in R0, RNG_CurrentValue   ; ... and then the next of the sequence
  mov [0x21], R0
  in R0, GPU_RemainingPixels
  mov [0x22], R0
  out 0x1200, 0x10          ; alias of GPU_Command: GPUCommand_ClearScreen costs pixels
  in R0, GPU_RemainingPixels
  mov [0x23], R0
  in R0, 0x1201             ; alias of GPU_RemainingPixels
  mov [0x24], R0
  out 0x1204, 0x21          ; alias of GPU_ActiveBlending, a valid mode
  in R0, GPU_ActiveBlending
  mov [0x25], R0
  hlt
