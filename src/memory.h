#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

#define KERNEL_HEAP_SIZE  (64 * 1024)
#define KERNEL_STACK_SIZE 4096

// --- Memory Manager API ---
void memory_init(void);
void* kmalloc(uint32_t size);
void kfree(void *ptr);
void* alloc_stack(void);
void free_stack(void *stack);
void memory_print_stats(void);

#endif