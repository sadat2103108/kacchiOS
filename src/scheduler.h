/* scheduler.h - Process scheduler with aging and time quantum */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"
#include "process.h"

/* ---------- SCHEDULER CONFIG ---------- */
#define DEFAULT_TIME_QUANTUM  10    /* Default time slice in milliseconds */
#define AGING_THRESHOLD       50    /* Increment priority after N ticks */
#define MAX_PRIORITY          20    /* Maximum priority value (lower = higher priority) */

/* ---------- SCHEDULER STATE ---------- */
typedef struct {
    uint32_t current_quantum;       /* Remaining time for current process */
    uint32_t time_quantum;          /* Configurable time quantum */
    uint32_t ticks;                 /* System ticks counter */
    uint32_t context_switches;      /* Count of context switches */
} scheduler_t;

extern scheduler_t scheduler;

/* ---------- API ---------- */

/* Initialize scheduler */
void scheduler_init(void);

/* Select next process to run (Round Robin with Aging) */
pcb_t* scheduler_next(void);

/* Update scheduler state (called on timer tick) */
void scheduler_tick(void);

/* Set time quantum (in ticks) */
void scheduler_set_quantum(uint32_t quantum);

/* Get current quantum */
uint32_t scheduler_get_quantum(void);

/* Get total context switches */
uint32_t scheduler_get_switches(void);

/* Perform context switch */
void scheduler_context_switch(void);

/* Apply aging to increase priority of waiting processes */
void scheduler_apply_aging(void);

/* Print scheduler statistics */
void scheduler_print_stats(void);

#endif