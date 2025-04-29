global isr_stub_table

extern irq_handler

%macro push_scratch_regs 0
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
%endmacro

%macro pop_scratch_regs 0
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
%endmacro

%macro isr_err_stub 1
isr_stub_%+%1:
    push_scratch_regs
    mov rdi, %1
    jmp isr_err
%endmacro

%macro isr_no_err_stub 1
isr_stub_%+%1:
    push_scratch_regs
    mov rdi, %1
    jmp isr_no_err
%endmacro

section .text
bits 64
isr_err:
    ; read error code off stack
    mov rsi, [rsp + 72]
    ; shift stack to remove error code
    lea rax, [rsp + 64]
shift_stack:
    mov rbx, [rax]
    mov [rax + 8], rbx
    sub rax, 8
    cmp rax, rsp
    jnz shift_stack
    add rsp, 8
isr_no_err:
    call irq_handler
    pop_scratch_regs
    iretq

{{HANDLER_STUBS}}

isr_stub_table:
%assign i 0 
%rep    256
    dq isr_stub_%+i
%assign i i+1 
%endrep
