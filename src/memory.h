#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

/* Size of kernel memory pool (64 KB) */
#define KERNEL_HEAP_SIZE  (64 * 1024)

/* Stack size per process (4 KB) */
#define KERNEL_STACK_SIZE 4096

/* Initialize memory manager */
void memory_init(void);

/* Heap allocation with metadata tracking and optimization */
void* kmalloc(uint32_t size);

/* Heap deallocation with tracking */
void kfree(void *ptr);

/* Stack allocation for processes */
void* alloc_stack(void);

/* Stack deallocation with tracking */
void free_stack(void *stack);

/* Print memory statistics and usage */
void memory_print_stats(void);

#endif
