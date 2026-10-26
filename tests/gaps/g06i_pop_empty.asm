; G6: POP with an empty stack: the word is read (R5 = 0), then StackUnderflow (5).
;@ frames=4
  mov R5, 77
  pop R5
  hlt
