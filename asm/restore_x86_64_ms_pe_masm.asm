
.code

restore_context PROC FRAME
    .endprolog

    ; first arg of restore_context() == pointer to context_t
    mov  rax, rcx

    mov rbp, [rax]
    mov rsp, [rax+8]
    mov rsi, [rax+16]
    mov rdi, [rax+24]

    mov rax, [rax+32]
    pop rcx                   ; remove original return address from the stack
    push rax                  ; push new return address to the stack
    ret                       ; 'return' from the function (effeicently jump to saved address)
restore_context ENDP
END