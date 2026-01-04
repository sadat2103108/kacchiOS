#include "memory.h"
#include "serial.h"
#include "string.h"

/* Memory allocation tracking */
typedef struct {
    uint32_t size;
    uint8_t is_allocated;
    uint8_t is_stack;
} mem_block_t;

/* Static kernel heap */
static uint8_t kernel_heap[KERNEL_HEAP_SIZE];

/* Memory allocation metadata */
#define MAX_ALLOCS 64
static mem_block_t alloc_metadata[MAX_ALLOCS];
static uint32_t alloc_count = 0;

/* Current allocation offset */
static uint32_t heap_offset = 0;

/* Stack region grows from top of heap downward */
static uint32_t stack_offset = KERNEL_HEAP_SIZE;

/* Memory statistics */
typedef struct {
    uint32_t total_allocated;
    uint32_t total_freed;
    uint32_t heap_allocations;
    uint32_t stack_allocations;
    uint32_t failed_allocations;
} mem_stats_t;

static mem_stats_t mem_stats = {0};

/* Initialize memory manager */
void memory_init(void)
{
    heap_offset = 0;
    stack_offset = KERNEL_HEAP_SIZE;
    alloc_count = 0;

    for (int i = 0; i < MAX_ALLOCS; i++)
    {
        alloc_metadata[i].size = 0;
        alloc_metadata[i].is_allocated = 0;
        alloc_metadata[i].is_stack = 0;
    }

    mem_stats.total_allocated = 0;
    mem_stats.total_freed = 0;
    mem_stats.heap_allocations = 0;
    mem_stats.stack_allocations = 0;
    mem_stats.failed_allocations = 0;

    serial_puts("[memory] initialized (heap=");
    serial_put_num(KERNEL_HEAP_SIZE / 1024);
    serial_puts("KB)\n");
}

/* Align size to 4 bytes for optimization */
static uint32_t align4(uint32_t size)
{
    return (size + 3) & ~3;
}

/* Find and track metadata for allocation */
static int find_metadata_slot(void)
{
    for (int i = 0; i < MAX_ALLOCS; i++)
    {
        if (!alloc_metadata[i].is_allocated)
            return i;
    }
    return -1;
}

/* Optimized heap allocation with metadata tracking */
void* kmalloc(uint32_t size)
{
    if (size == 0)
        return 0;

    size = align4(size);

    /* Check fragmentation and available space */
    if (heap_offset + size >= stack_offset)
    {
        serial_puts("[memory] FAIL: heap exhausted (need ");
        serial_put_num(size / 100);
        serial_puts("B)\n");
        mem_stats.failed_allocations++;
        return 0;
    }

    /* Find metadata slot */
    int meta_idx = find_metadata_slot();
    if (meta_idx < 0)
    {
        serial_puts("[memory] FAIL: metadata table full\n");
        mem_stats.failed_allocations++;
        return 0;
    }

    void *ptr = &kernel_heap[heap_offset];
    
    /* Track allocation */
    alloc_metadata[meta_idx].size = size;
    alloc_metadata[meta_idx].is_allocated = 1;
    alloc_metadata[meta_idx].is_stack = 0;
    alloc_count++;

    /* Update statistics */
    mem_stats.total_allocated += size;
    mem_stats.heap_allocations++;

    heap_offset += size;

    serial_puts("[memory] kmalloc ");
    serial_put_num(size / 100);
    serial_puts("B at ");
    serial_put_num((uint32_t)ptr / 10000);
    serial_puts("\n");

    return ptr;
}

/* Free heap memory with tracking */
void kfree(void *ptr)
{
    if (!ptr)
        return;

    /* Find and mark as free */
    for (int i = 0; i < MAX_ALLOCS; i++)
    {
        if (alloc_metadata[i].is_allocated && !alloc_metadata[i].is_stack)
        {
            alloc_metadata[i].is_allocated = 0;
            mem_stats.total_freed += alloc_metadata[i].size;

            serial_puts("[memory] kfree ");
            serial_put_num(alloc_metadata[i].size / 100);
            serial_puts("B\n");
            return;
        }
    }

    serial_puts("[memory] WARNING: double free or invalid ptr\n");
}

/* Allocate a process stack (top-down) with tracking */
void* alloc_stack(void)
{
    if (stack_offset < heap_offset + KERNEL_STACK_SIZE)
    {
        serial_puts("[memory] FAIL: stack exhausted\n");
        mem_stats.failed_allocations++;
        return 0;
    }

    /* Find metadata slot */
    int meta_idx = find_metadata_slot();
    if (meta_idx < 0)
    {
        serial_puts("[memory] FAIL: metadata table full for stack\n");
        return 0;
    }

    stack_offset -= KERNEL_STACK_SIZE;
    void *ptr = &kernel_heap[stack_offset];

    /* Track allocation */
    alloc_metadata[meta_idx].size = KERNEL_STACK_SIZE;
    alloc_metadata[meta_idx].is_allocated = 1;
    alloc_metadata[meta_idx].is_stack = 1;
    alloc_count++;

    /* Update statistics */
    mem_stats.total_allocated += KERNEL_STACK_SIZE;
    mem_stats.stack_allocations++;

    serial_puts("[memory] alloc_stack ");
    serial_put_num(KERNEL_STACK_SIZE / 1000);
    serial_puts("KB at ");
    serial_put_num((uint32_t)ptr / 10000);
    serial_puts("\n");

    return ptr;
}

/* Free process stack with tracking */
void free_stack(void *stack)
{
    if (!stack)
        return;

    /* Find and mark as free */
    for (int i = 0; i < MAX_ALLOCS; i++)
    {
        if (alloc_metadata[i].is_allocated && alloc_metadata[i].is_stack)
        {
            alloc_metadata[i].is_allocated = 0;
            mem_stats.total_freed += alloc_metadata[i].size;

            serial_puts("[memory] free_stack ");
            serial_put_num(alloc_metadata[i].size / 1000);
            serial_puts("KB\n");
            return;
        }
    }
}

/* Print memory statistics */
void memory_print_stats(void)
{
    serial_puts("\n========== MEMORY STATISTICS ==========\n");
    serial_puts("Total allocated: ");
    serial_put_num(mem_stats.total_allocated / 1000);
    serial_puts("KB\n");

    serial_puts("Total freed: ");
    serial_put_num(mem_stats.total_freed / 1000);
    serial_puts("KB\n");

    serial_puts("Heap allocations: ");
    serial_put_num(mem_stats.heap_allocations);
    serial_puts("\n");

    serial_puts("Stack allocations: ");
    serial_put_num(mem_stats.stack_allocations);
    serial_puts("\n");

    serial_puts("Failed allocations: ");
    serial_put_num(mem_stats.failed_allocations);
    serial_puts("\n");

    serial_puts("Heap used: ");
    serial_put_num(heap_offset / 1000);
    serial_puts("KB / ");
    serial_put_num(KERNEL_HEAP_SIZE / 1000);
    serial_puts("KB\n");

    serial_puts("======================================\n\n");
}
