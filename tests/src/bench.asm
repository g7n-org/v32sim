; bench.asm: what v32lua -g would write for bench.lua (hand made for the v32sim tests).
; Frames are PUSH BP / MOV BP,SP; the runtime helper has no source (;@src -).
;@lang lua
;@src bench.lua:2
__function_main:
    push BP
    mov  BP, SP
    isub SP, 2               ; two locals: BP-1 = t, BP-2 = i
    mov  R0, 0
    mov  [BP-1], R0
;@src bench.lua:7
    mov  R0, 1
    mov  [BP-2], R0
__for_head:
    mov  R0, [BP-2]
    ile  R0, 3
    jf   R0, __for_done
;@src bench.lua:8
    mov  R1, [BP-2]
    push R1
    mov  R1, [BP-1]
    push R1
    call __function_add
    iadd SP, 2
    mov  [BP-1], R0
;@src bench.lua:7
    mov  R0, [BP-2]
    iadd R0, 1
    mov  [BP-2], R0
    jmp  __for_head
__for_done:
;@src bench.lua:10
    mov  R1, 4
    push R1
    call __function_fact
    iadd SP, 1
;@src bench.lua:11
    mov  R0, [BP-1]
    call __runtime_print
;@src bench.lua:12
    mov  SP, BP
    pop  BP
    hlt

;@src bench.lua:3
__function_add:
    push BP
    mov  BP, SP
    isub SP, 1
;@src bench.lua:4
    mov  R0, [BP+2]
    mov  R1, [BP+3]
    iadd R0, R1
    mov  [BP-1], R0
;@src bench.lua:5
    mov  R0, [BP-1]
;@src bench.lua:6
    mov  SP, BP
    pop  BP
    ret

; fact(n) is recursive: n <= 1 -> 1, else n * fact(n-1). It has no entry in bench.lua
; (line 10 calls it); its lines are those of a second file, fact.lua.
;@src fact.lua:1
__function_fact:
    push BP
    mov  BP, SP
;@src fact.lua:2
    mov  R0, [BP+2]
    ile  R0, 1
    jf   R0, __fact_rec
;@src fact.lua:3
    mov  R0, 1
    mov  SP, BP
    pop  BP
    ret
__fact_rec:
;@src fact.lua:5
    mov  R1, [BP+2]
    isub R1, 1
    push R1
    call __function_fact
    iadd SP, 1
;@src fact.lua:6
    mov  R1, [BP+2]
    imul R0, R1
    mov  SP, BP
    pop  BP
    ret

;@src -
__runtime_print:
    mov  R5, R0
    mov  R6, R5
    ret
