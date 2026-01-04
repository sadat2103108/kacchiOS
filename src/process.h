#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"

// --- Configuration ---
#define MAX_PROCESSES 16
#define MAX_MESSAGES  8

// --- Process States ---
typedef enum {
    PROC_UNUSED = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_SLEEPING,
    PROC_TERMINATED
} proc_state_t;

// --- IPC Message Structure ---
typedef struct {
    uint32_t sender_pid;
    uint32_t value;
} message_t;

// --- Process Control Block ---
typedef struct pcb {
    uint32_t pid;
    proc_state_t state;

    uint32_t *stack_base;
    uint32_t *stack_ptr;

    uint32_t priority;
    uint32_t age;

    message_t msg_queue[MAX_MESSAGES];
    uint32_t msg_count;

} pcb_t;

// --- Global Process Table ---
extern pcb_t proc_table[MAX_PROCESSES];
extern pcb_t *current_proc;

// --- Process Management API ---
void process_init(void);
int  process_create(void (*entry)(void), uint32_t priority);
void process_exit(void);

// --- State Management ---
void process_set_state(int pid, proc_state_t state);
proc_state_t process_get_state(int pid);

// --- Process Utilities ---
pcb_t* process_get(int pid);
int process_current_pid(void);
uint32_t process_count_active(void);
void process_list(void);

// --- Inter-Process Communication ---
int process_send(int dest_pid, uint32_t value);
int process_receive(uint32_t *out_value);

#endif