#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"

/* ---------- CONFIG ---------- */
#define MAX_PROCESSES 16
#define MAX_MESSAGES  8

/* ---------- PROCESS STATES ---------- */
typedef enum {
    PROC_UNUSED = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_SLEEPING,
    PROC_TERMINATED
} proc_state_t;

/* ---------- IPC MESSAGE ---------- */
typedef struct {
    uint32_t sender_pid;
    uint32_t value;
} message_t;

/* ---------- PROCESS CONTROL BLOCK ---------- */
typedef struct pcb {
    uint32_t pid;
    proc_state_t state;

    uint32_t *stack_base;
    uint32_t *stack_ptr;

    uint32_t priority;
    uint32_t age;

    /* IPC */
    message_t msg_queue[MAX_MESSAGES];
    uint32_t msg_count;

} pcb_t;

/* ---------- GLOBAL PROCESS TABLE ---------- */
extern pcb_t proc_table[MAX_PROCESSES];
extern pcb_t *current_proc;

/* ---------- API ---------- */

/* Init */
void process_init(void);

/* Creation / termination */
int  process_create(void (*entry)(void), uint32_t priority);
void process_exit(void);

/* State management */
void process_set_state(int pid, proc_state_t state);
proc_state_t process_get_state(int pid);

/* Utilities */
pcb_t* process_get(int pid);
int process_current_pid(void);

/* IPC */
int process_send(int dest_pid, uint32_t value);
int process_receive(uint32_t *out_value);

#endif