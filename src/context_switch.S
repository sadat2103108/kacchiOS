/* context_switch.S - Assembly code for CPU register context switching */
.section .text
.global context_switch_asm
.global save_context
.global restore_context

/* 
 * void context_switch_asm(uint32_t **current_sp, uint32_t **next_sp)
 * Arguments (System V i386 calling convention):
 *   8(%ebp) = pointer to current_sp (pointer to uint32_t*)
 *   12(%ebp) = pointer to next_sp (pointer to uint32_t*)
 */
.align 4
context_switch_asm:
    pushl %ebp
    movl %esp, %ebp
    
    /* Get argument pointers from stack */
    movl 8(%ebp), %eax      /* pointer to current_sp */
    movl 12(%ebp), %edx     /* pointer to next_sp */
    
    /* Save all registers to stack */
    pushl %ebx
    pushl %ecx
    pushl %esi
    pushl %edi
    
    /* Save current ESP to *current_sp */
    movl %esp, (%eax)
    
    /* Load next process's ESP from *next_sp */
    movl (%edx), %esp
    
    /* Restore all registers from new stack */
    popl %edi
    popl %esi
    popl %ecx
    popl %ebx
    
    /* Restore EBP and return */
    popl %ebp
    ret

/*
 * void save_context(void)
 * Saves current CPU state to stack
 * Called from C code to preserve registers
 */
.align 4
save_context:
    pushl %eax
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %edi
    pushl %ebp
    ret

/*
 * void restore_context(void)
 * Restores CPU state from stack
 * Called from C code to resume registers
 */
.align 4
restore_context:
    popl %ebp
    popl %edi
    popl %esi
    popl %edx
    popl %ecx
    popl %ebx
    popl %eax
    ret