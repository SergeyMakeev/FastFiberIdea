; standard C library function
EXTERN  _exit:PROC

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

    ; reserve 16 bytes on context stack to exit_app (when function returns)
    sub  r8, 16


    mov [rax], rbp
    mov [rax+8], r8
    mov [rax+16], rsi
    mov [rax+24], rdi
    mov [rax+32], rdx

    ; TODO something wrong here, why whould I need to add exit_app addrress twice?
    lea  rcx, exit_app
    mov  [r8], rcx
    mov  [r8+8], rcx

    ret

exit_app:
    ; exit code is zero
    xor  rcx, rcx
    ; exit application
    call  _exit
    hlt
make_context ENDP
END