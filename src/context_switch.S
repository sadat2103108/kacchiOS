// --- CPU Context Switching Assembly ---
.section .text
.global context_switch_asm
.global save_context
.global restore_context

// --- Main Context Switch ---
.align 4
context_switch_asm:
    pushl %ebp
    movl %esp, %ebp
    
    movl 8(%ebp), %eax
    movl 12(%ebp), %edx
    
    // Save registers to stack
    pushl %ebx
    pushl %ecx
    pushl %esi
    pushl %edi
    
    // Save current ESP
    movl %esp, (%eax)
    
    // Load next process ESP
    movl (%edx), %esp
    
    // Restore registers from new stack
    popl %edi
    popl %esi
    popl %ecx
    popl %ebx
    
    // Return
    popl %ebp
    ret

// --- Save Current Context ---
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

// --- Restore Context ---
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