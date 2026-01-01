/* kernel.c - Main kernel with null process */
#include "types.h"
#include "serial.h"
#include "string.h"
#include "memory.h"
#include "process.h"
#define MAX_INPUT 128

void dummy_process(void)
{
    while (1)
    {
        serial_puts("[dummy] running...\n");
        for (volatile int i = 0; i < 1000000; i++)
            ;
    }
}

void process_test(void)
{
    serial_puts("\n[TEST] Process Manager Test Start\n");

    int p1 = process_create(dummy_process, 3);
    int p2 = process_create(dummy_process, 5);

    if (p1 >= 0 && p2 >= 0)
        serial_puts("[OK] Process creation\n");
    else
    {
        serial_puts("[FAIL] Process creation\n");
        return;
    }

    pcb_t *proc = process_get(p1);
    if (proc && proc->state == PROC_READY)
        serial_puts("[OK] Initial state READY\n");

    process_set_state(p1, PROC_BLOCKED);
    if (proc->state == PROC_BLOCKED)
        serial_puts("[OK] State transition BLOCKED\n");

    /* Simulate running process before exit */
    current_proc = proc;
    process_exit();

    if (proc->state == PROC_TERMINATED)
        serial_puts("[OK] Process termination\n");

    serial_puts("[TEST] Process Manager Test End\n\n");
}

void kmain(void)
{
    char input[MAX_INPUT];
    int pos = 0;

    /* Initialize hardware */
    serial_init();

    memory_init();
    process_init();
    process_test();

    void *a = kmalloc(100);
    void *b = kmalloc(200);
    void *s = alloc_stack();

    if (a && b && s)
        serial_puts("Memory manager OK\n");
    else
        serial_puts("Memory manager FAILED\n");

    /* Print welcome message */
    serial_puts("\n");
    serial_puts("========================================\n");
    serial_puts("    kacchiOS - Minimal Baremetal OS\n");
    serial_puts("========================================\n");
    serial_puts("Hello from kacchiOS!\n");
    serial_puts("Running null process...\n\n");

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
                serial_puts("\b \b"); /* Erase character on screen */
            }
            /* Handle normal characters */
            else if (c >= 32 && c < 127 && pos < MAX_INPUT - 1)
            {
                input[pos++] = c;
                serial_putc(c); /* Echo character */
            }
        }

        /* Echo back the input */
        if (pos > 0)
        {
            serial_puts("You typed: ");
            serial_puts(input);
            serial_puts("\n");
        }
    }

    /* Should never reach here */
    for (;;)
    {
        __asm__ volatile("hlt");
    }
}