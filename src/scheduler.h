#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"
#include "process.h"

// --- Configuration ---
#define DEFAULT_TIME_QUANTUM  10
#define AGING_THRESHOLD       50
#define MAX_PRIORITY          20

// --- Scheduler State Structure ---
typedef struct {
    uint32_t current_quantum;
    uint32_t time_quantum;
    uint32_t ticks;
    uint32_t context_switches;
} scheduler_t;

extern scheduler_t scheduler;

// --- Core Scheduler API ---
void scheduler_init(void);
pcb_t* scheduler_next(void);
void scheduler_tick(void);
void scheduler_set_quantum(uint32_t quantum);
uint32_t scheduler_get_quantum(void);
uint32_t scheduler_get_switches(void);

// --- Context Switching and Aging ---
void scheduler_context_switch(void);
void scheduler_apply_aging(void);

// --- Statistics ---
void scheduler_print_stats(void);

#endif