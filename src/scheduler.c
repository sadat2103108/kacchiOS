// --- Scheduler Implementation ---
#include "scheduler.h"
#include "memory.h"
#include "serial.h"
#include "string.h"
#include "context_switch.h"

scheduler_t scheduler;

// --- Initialization ---
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

// --- Process Selection ---
pcb_t* scheduler_next(void)
{
    pcb_t *best = NULL;
    uint32_t best_priority = MAX_PRIORITY + 1;

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

    if (!best)
    {
        return NULL;
    }

    return best;
}

// --- Timer Tick Handler ---
void scheduler_tick(void)
{
    scheduler.ticks++;

    if (current_proc)
    {
        scheduler.current_quantum--;

        if (scheduler.current_quantum == 0 && current_proc->state == PROC_RUNNING)
        {
            current_proc->state = PROC_READY;
            scheduler_context_switch();
        }
    }

    if (scheduler.ticks % AGING_THRESHOLD == 0)
    {
        scheduler_apply_aging();
    }
}

// --- Context Switching ---
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
        
        context_switch_asm(&current_proc->stack_ptr, &next->stack_ptr);
    }
    else
    {
        serial_puts("[scheduler] starting first process PID ");
        serial_put_num(next->pid);
        serial_puts("\n");
        
        context_switch_asm(0, &next->stack_ptr);
    }

    current_proc = next;
    current_proc->state = PROC_RUNNING;
    scheduler.current_quantum = scheduler.time_quantum;
    scheduler.context_switches++;
}

// --- Priority Aging ---
void scheduler_apply_aging(void)
{
    uint32_t aged_count = 0;

    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (proc_table[i].state == PROC_READY && proc_table[i].pid != 0)
        {
            proc_table[i].age++;

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

// --- Configuration ---
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

uint32_t scheduler_get_quantum(void)
{
    return scheduler.time_quantum;
}

uint32_t scheduler_get_switches(void)
{
    return scheduler.context_switches;
}

// --- Statistics ---
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
