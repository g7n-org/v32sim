; G8: an instruction whose immediate word lies past the end of the ROM: R1 = IP + 2, R2 = the new
;@ frames=4
; instruction word, R3 = the old immediate (here 6 from the MOV).
  mov R6, 6
  integer 0x0A000000
