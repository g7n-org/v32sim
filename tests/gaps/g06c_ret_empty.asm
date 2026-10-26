; G6: RET with an empty stack pops RAM's last word (0) into IP, then SP passes the end of RAM:
;@ frames=4
; StackUnderflow (5), with R1 = the IP as popped (0).
  ret
  hlt
