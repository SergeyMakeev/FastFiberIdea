
.code

; generate function table entry in .pdata and unwind information in
switch_context PROC FRAME
    .endprolog

    ; rcx - first arg (from)
    ; rdx - second arg (to)
    ; r8 - third arg (env)

    ; save current context
    ; ------------------------------------------------------------
    pop rax                ; pop current function return address

    ; save nonvolatile registers
    mov [rcx+0], r12
    mov [rcx+8], r13
    mov [rcx+16], r14
    mov [rcx+24], r15
    mov [rcx+32], rdi
    mov [rcx+40], rsi
    mov [rcx+48], rbx
    mov [rcx+56], rbp

    mov [rcx+64], rsp      ; stack
    mov [rcx+72], rax      ; rip

    ; load NT_TIB
    mov  r10,  gs:[030h]
    
    mov  rax,  [r10+8]   ; stack base = FIELD_OFFSET(NT_TIB, StackBase)
    mov [rcx+80], rax      

    mov  rax,  [r10+16]   ; stack limit = FIELD_OFFSET(NT_TIB, StackLimit)
    mov [rcx+88], rax      

    mov  rax,  [r10+01478h]   ; TIB deallocation stack
    mov [rcx+96], rax      


    ; restore new context
    ; ------------------------------------------------------------

    ; save context_env_t to NT_TIB::ArbitraryUserPointer
    mov [r10+40], r8      ; user pointer = FIELD_OFFSET(NT_TIB, ArbitraryUserPointer)

    mov rax, [rdx+80]      ; restore TIB stack base
    mov [r10+8], rax

    mov rax, [rdx+88]      ; restore TIB stack limit
    mov [r10+16], rax

    mov rax, [rdx+96]      ; restore TIB deallocation stack
    mov [r10+01478h], rax


    mov rsp, [rdx+64]      ; update stack pointer

    mov rax, [rdx+72]      
    jmp rax                ; jump to a new address

switch_context ENDP
END