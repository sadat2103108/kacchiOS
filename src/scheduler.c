/* scheduler.c - Process scheduler with round-robin, aging, and context switching */
#include "scheduler.h"
#include "memory.h"
#include "serial.h"
#include "string.h"
#include "context_switch.h"

scheduler_t scheduler;

/* Initialize scheduler */
void scheduler_init(void)
{
    scheduler.current_quantum = DEFAULT_TIME_QUANTUM;
    scheduler.time_quantum = DEFAULT_TIME_QUANTUM;
    scheduler.ticks = 0;
    scheduler.context_switches = 0;

    serial_puts("[scheduler] initialized with quantum=");
    serial_put_num(DEFAULT_TIME_QUANTUM);
    serial_puts("ms\n");
}

/* Find next ready process using round-robin with priority consideration */
pcb_t* scheduler_next(void)
{
    pcb_t *best = NULL;
    uint32_t best_priority = MAX_PRIORITY + 1;

    /* First pass: find READY process with highest priority (lowest value) */
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (proc_table[i].state == PROC_READY)
        {
            if (proc_table[i].priority < best_priority)
            {
                best_priority = proc_table[i].priority;
                best = &proc_table[i];
            }
        }
    }

    /* If no READY process, return NULL */
    if (!best)
    {
        return NULL;
    }

    return best;
}

/* Called on each timer tick */
void scheduler_tick(void)
{
    scheduler.ticks++;

    if (current_proc)
    {
        scheduler.current_quantum--;

        /* Quantum expired or process blocked - context switch */
        if (scheduler.current_quantum == 0 && current_proc->state == PROC_RUNNING)
        {
            current_proc->state = PROC_READY;
            scheduler_context_switch();
        }
    }

    /* Apply aging every AGING_THRESHOLD ticks */
    if (scheduler.ticks % AGING_THRESHOLD == 0)
    {
        scheduler_apply_aging();
    }
}

/* Perform context switch - save current state and load next process */
void scheduler_context_switch(void)
{
    pcb_t *next = scheduler_next();

    if (!next)
    {
        serial_puts("[scheduler] no READY process available\n");
        return;
    }

    if (current_proc)
    {
        serial_puts("[scheduler] switch from PID ");
        serial_put_num(current_proc->pid);
        serial_puts(" to PID ");
        serial_put_num(next->pid);
        serial_puts("\n");
        
        /* Call assembly routine to switch contexts */
        context_switch_asm(&current_proc->stack_ptr, &next->stack_ptr);
    }
    else
    {
        serial_puts("[scheduler] starting first process PID ");
        serial_put_num(next->pid);
        serial_puts("\n");
        
        /* Load next process's stack pointer and restore registers */
        context_switch_asm(0, &next->stack_ptr);
    }

    /* Update current process */
    current_proc = next;
    current_proc->state = PROC_RUNNING;
    scheduler.current_quantum = scheduler.time_quantum;
    scheduler.context_switches++;
}

/* Increase priority of waiting processes (aging) */
void scheduler_apply_aging(void)
{
    uint32_t aged_count = 0;

    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (proc_table[i].state == PROC_READY && proc_table[i].pid != 0)
        {
            proc_table[i].age++;

            /* Increase priority (lower value = higher priority) every 10 aging cycles */
            if (proc_table[i].age % 10 == 0 && proc_table[i].priority > 1)
            {
                proc_table[i].priority--;
                aged_count++;
            }
        }
    }

    if (aged_count > 0)
    {
        serial_puts("[scheduler] aging applied, ");
        serial_put_num(aged_count);
        serial_puts(" processes promoted\n");
    }
}

/* Set configurable time quantum */
void scheduler_set_quantum(uint32_t quantum)
{
    if (quantum > 0 && quantum <= 100)
    {
        scheduler.time_quantum = quantum;
        scheduler.current_quantum = quantum;

        serial_puts("[scheduler] time quantum set to ");
        serial_put_num(quantum);
        serial_puts("ms\n");
    }
    else
    {
        serial_puts("[scheduler] invalid quantum value\n");
    }
}

/* Get current quantum */
uint32_t scheduler_get_quantum(void)
{
    return scheduler.time_quantum;
}

/* Get context switch count */
uint32_t scheduler_get_switches(void)
{
    return scheduler.context_switches;
}

/* Print scheduler statistics */
void scheduler_print_stats(void)
{
    serial_puts("\n========== SCHEDULER STATISTICS ==========\n");
    serial_puts("System ticks: ");
    serial_put_num(scheduler.ticks);
    serial_puts("\n");

    serial_puts("Context switches: ");
    serial_put_num(scheduler.context_switches);
    serial_puts("\n");

    serial_puts("Current quantum: ");
    serial_put_num(scheduler.time_quantum);
    serial_puts("ms\n");

    serial_puts("Current process PID: ");
    if (current_proc)
        serial_put_num(current_proc->pid);
    else
        serial_puts("none");
    serial_puts("\n");

    serial_puts("\nReady processes:\n");
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (proc_table[i].state == PROC_READY && proc_table[i].pid != 0)
        {
            serial_puts("  PID ");
            serial_put_num(proc_table[i].pid);
            serial_puts(": priority=");
            serial_put_num(proc_table[i].priority);
            serial_puts(", age=");
            serial_put_num(proc_table[i].age);
            serial_puts("\n");
        }
    }
    serial_puts("=========================================\n\n");
}
