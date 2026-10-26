; G7: the offset is checked against the device size, not the absolute address: the word just past
;@ frames=4
; the end of RAM is invalid however the high bits are set.
  mov R5, 77
  mov R5, [0x40400000]
  hlt
