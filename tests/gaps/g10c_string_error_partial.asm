; G10: an error part way through a string op leaves the words already done in place, with the
; registers as they were after the last completed word (the handler saves them).
;@ frames=8 dump=300@0x100 name=movs_dest_rom
;@ frames=8 dump=300@0x100 name=sets_runs_off_ram
  mov R13, 0x400
  mov R12, 1
  mov R11, 4
  sets
  mov R13, 0x000FFFFC     
  mov R12, 0x400
  mov R11, 10
  movs
  hlt
