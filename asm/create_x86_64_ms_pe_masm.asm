; standard C library function
EXTERN  _exit:PROC

.code

; generate function table entry in .pdata and unwind information in
create_context PROC FRAME
    .endprolog

    ; rcx - first arg (context)
    ; rdx - second arg (func ptr)
    ; r8 - third arg (stack ptr)

    ; first arg of save_context() == pointer to context_t

    mov [rcx+0], r12
    mov [rcx+8], r13
    mov [rcx+16], r14
    mov [rcx+24], r15
    mov [rcx+32], rdi
    mov [rcx+40], rsi
    mov [rcx+48], rbx
    mov [rcx+56], rbp
    mov [rcx+64], r8    ; rsp
    mov [rcx+72], rdx   ; rip
    ret

create_context ENDP
END