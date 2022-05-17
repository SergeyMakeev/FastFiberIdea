
.code

; generate function table entry in .pdata and unwind information in
make_context PROC FRAME
    .endprolog

    ; rcx - first arg (ctx)
    ; rdx - second arg (func ptr)
    ; r8 - third arg (stack ptr)
    ; r9 - fourth arg (stack size)

    ; first arg of save_context() == pointer to context_t
    mov  rax, rcx

    mov [rax], rbp
    mov [rax+8], r8
    mov [rax+16], rsi
    mov [rax+24], rdi
    mov [rax+32], rdx
    ret
make_context ENDP
END