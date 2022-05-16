
.code

; generate function table entry in .pdata and unwind information in
save_context PROC FRAME
    .endprolog

    ; first arg of save_context() == pointer to context_t
    mov  rax, rcx

    mov [rax], rbp
    mov [rax+8], rsp
    mov [rax+16], rsi
    mov [rax+24], rdi

    pop rcx                ; pop function return address
    mov [rax+32], rcx      ; save function return address
    push rcx               ; push it back
    ret
save_context ENDP
END