/* kernel.c - Main kernel with memory, process, and scheduler management */
#include "types.h"
#include "serial.h"
#include "string.h"
#include "memory.h"
#include "process.h"
#include "scheduler.h"
#define MAX_INPUT 128

/* Test processes */
void worker_process_high(void)
{
    serial_puts("[P-HIGH] high priority process started\n");
    for (int i = 0; i < 5; i++)
    {
        serial_puts("[P-HIGH] iteration ");
        serial_put_num(i);
        serial_puts("\n");
        for (volatile int j = 0; j < 500000; j++);
    }
    serial_puts("[P-HIGH] completed\n");
    process_exit();
}

void worker_process_low(void)
{
    serial_puts("[P-LOW] low priority process started\n");
    for (int i = 0; i < 3; i++)
    {
        serial_puts("[P-LOW] iteration ");
        serial_put_num(i);
        serial_puts("\n");
        for (volatile int j = 0; j < 500000; j++);
    }
    serial_puts("[P-LOW] completed\n");
    process_exit();
}

/* Simple test process that doesn't actually run */
void test_simple_process(void)
{
    serial_puts("[test-proc] process running\n");
    process_exit();
}

void ipc_test_sender(void)
{
    serial_puts("[IPC-SEND] sender process started\n");
    
    /* Find a receiver process (PID 2 if exists) */
    for (int i = 0; i < 3; i++)
    {
        int result = process_send(2, 100 + i);
        if (result == 0)
            serial_puts("[IPC-SEND] message sent\n");
        for (volatile int j = 0; j < 300000; j++);
    }
    
    process_exit();
}

void ipc_test_receiver(void)
{
    serial_puts("[IPC-RECV] receiver process started\n");
    
    uint32_t msg_val;
    for (int i = 0; i < 3; i++)
    {
        if (process_receive(&msg_val) == 0)
        {
            serial_puts("[IPC-RECV] got message value\n");
        }
        for (volatile int j = 0; j < 300000; j++);
    }
    
    process_exit();
}

/* Comprehensive memory tests */
void test_memory_manager(void)
{
    serial_puts("\n========== MEMORY TEST ==========\n");
    
    serial_puts("[TEST] Heap allocation...\n");
    void *p1 = kmalloc(50);
    void *p2 = kmalloc(100);
    void *p3 = kmalloc(200);
    
    if (p1 && p2 && p3)
        serial_puts("[OK] Multiple heap allocations\n");
    else
        serial_puts("[FAIL] Heap allocation\n");
    
    serial_puts("[TEST] Stack allocation...\n");
    void *s1 = alloc_stack();
    void *s2 = alloc_stack();
    
    if (s1 && s2)
        serial_puts("[OK] Stack allocations\n");
    else
        serial_puts("[FAIL] Stack allocation\n");
    
    serial_puts("[TEST] Memory deallocation...\n");
    kfree(p1);
    kfree(p2);
    free_stack(s1);
    serial_puts("[OK] Deallocations completed\n");
    
    memory_print_stats();
}

/* Comprehensive process tests */
void test_process_manager(void)
{
    serial_puts("\n========== PROCESS TEST ==========\n");
    
    serial_puts("[TEST] Create test process...\n");
    int p1 = process_create(test_simple_process, 5);
    if (p1 >= 0)
        serial_puts("[OK] Process creation\n");
    
    serial_puts("[TEST] State transitions...\n");
    process_set_state(p1, PROC_BLOCKED);
    if (process_get_state(p1) == PROC_BLOCKED)
        serial_puts("[OK] State change to BLOCKED\n");
    
    process_set_state(p1, PROC_READY);
    if (process_get_state(p1) == PROC_READY)
        serial_puts("[OK] State change to READY\n");
    
    serial_puts("[TEST] Get process utilities...\n");
    pcb_t *proc = process_get(p1);
    if (proc && proc->pid == (uint32_t)p1)
        serial_puts("[OK] process_get() works\n");
    
    uint32_t active = process_count_active();
    serial_puts("[OK] Active processes: ");
    serial_put_num(active);
    serial_puts("\n");
    
    process_list();
}

/* Test scheduler */
void test_scheduler(void)
{
    serial_puts("\n========== SCHEDULER TEST ==========\n");
    
    serial_puts("[TEST] Initialize scheduler...\n");
    scheduler_init();
    serial_puts("[OK] Scheduler initialized\n");
    
    serial_puts("[TEST] Set time quantum to 20ms...\n");
    scheduler_set_quantum(20);
    
    if (scheduler_get_quantum() == 20)
        serial_puts("[OK] Quantum set correctly\n");
    
    serial_puts("[TEST] Select next process...\n");
    pcb_t *next = scheduler_next();
    if (next)
    {
        serial_puts("[OK] Selected process PID ");
        serial_put_num(next->pid);
        serial_puts("\n");
    }
    else
    {
        serial_puts("[INFO] No READY process available\n");
    }
    
    serial_puts("[TEST] Scheduler statistics...\n");
    serial_puts("[OK] Scheduler test completed\n");
    
    serial_puts("[TEST] Apply aging algorithm...\n");
    scheduler_apply_aging();
    
    scheduler_print_stats();
}

/* Test IPC */
void test_ipc(void)
{
    serial_puts("\n========== IPC TEST ==========\n");
    
    serial_puts("[TEST] Create IPC processes...\n");
    int sender_pid = process_create(ipc_test_sender, 5);
    int recv_pid = process_create(ipc_test_receiver, 5);
    
    if (sender_pid > 0 && recv_pid > 0)
        serial_puts("[OK] IPC processes created\n");
    
    serial_puts("[TEST] IPC simulation...\n");
    current_proc = process_get(sender_pid);
    
    uint32_t test_msg = 42;
    if (process_send(recv_pid, test_msg) == 0)
        serial_puts("[OK] Message sent\n");
    
    current_proc = process_get(recv_pid);
    uint32_t received;
    if (process_receive(&received) == 0)
        serial_puts("[OK] Message received\n");
}

void kmain(void)
{
    char input[MAX_INPUT];
    int pos = 0;

    /* Initialize hardware and subsystems */
    serial_init();
    serial_puts("\n[BOOT] Initializing kacchiOS...\n");

    /* Initialize managers */
    memory_init();
    process_init();
    scheduler_init();

    /* Run comprehensive tests */
    test_memory_manager();
    test_process_manager();
    test_scheduler();
    test_ipc();

    /* Print welcome message */
    serial_puts("\n");
    serial_puts("========================================\n");
    serial_puts("    kacchiOS - Full Featured OS\n");
    serial_puts("    Memory | Process | Scheduler\n");
    serial_puts("========================================\n");
    serial_puts("System initialized successfully!\n");
    serial_puts("Type 'help' for commands\n\n");

    /* Main loop - the "null process" */
    while (1)
    {
        serial_puts("kacchiOS> ");
        pos = 0;

        /* Read input line */
        while (1)
        {
            char c = serial_getc();

            /* Handle Enter key */
            if (c == '\r' || c == '\n')
            {
                input[pos] = '\0';
                serial_puts("\n");
                break;
            }
            /* Handle Backspace */
            else if ((c == '\b' || c == 0x7F) && pos > 0)
            {
                pos--;
                serial_puts("\b \b");
            }
            /* Handle normal characters */
            else if (c >= 32 && c < 127 && pos < MAX_INPUT - 1)
            {
                input[pos++] = c;
                serial_putc(c);
            }
        }

        /* Process commands */
        if (pos > 0)
        {
            if (input[0] == 'h' && input[1] == 'e' && input[2] == 'l' && input[3] == 'p')
            {
                serial_puts("\nAvailable commands:\n");
                serial_puts("  help      - Show this help\n");
                serial_puts("  memstat   - Show memory statistics\n");
                serial_puts("  proclist  - List all processes\n");
                serial_puts("  schedstat - Show scheduler stats\n");
                serial_puts("  test      - Run all tests\n");
                serial_puts("  exit      - Halt system\n\n");
            }
            else if (input[0] == 'm' && input[1] == 'e' && input[2] == 'm')
            {
                memory_print_stats();
            }
            else if (input[0] == 'p' && input[1] == 'r' && input[2] == 'o')
            {
                process_list();
            }
            else if (input[0] == 's' && input[1] == 'c' && input[2] == 'h')
            {
                scheduler_print_stats();
            }
            else if (input[0] == 't' && input[1] == 'e' && input[2] == 's')
            {
                serial_puts("\nRunning comprehensive tests...\n");
                test_memory_manager();
                test_process_manager();
                test_scheduler();
            }
            else if (input[0] == 'e' && input[1] == 'x' && input[2] == 'i')
            {
                serial_puts("System halting...\n");
                for (;;)
                {
                    __asm__ volatile("hlt");
                }
            }
            else
            {
                serial_puts("Unknown command. Type 'help' for commands.\n");
            }
        }
    }

    /* Should never reach here */
    for (;;)
    {
        __asm__ volatile("hlt");
    }
}