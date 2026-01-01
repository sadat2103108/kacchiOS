#include "process.h"
#include "memory.h"
#include "serial.h"


pcb_t proc_table[MAX_PROCESSES];
pcb_t *current_proc = 0;

static uint32_t next_pid = 1;



static int find_free_slot(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (proc_table[i].state == PROC_UNUSED)
            return i;
    }
    return -1;
}

/* Initialize stack so first context switch "returns" into entry */
static uint32_t* init_stack(void *stack_top, void (*entry)(void)) {
    uint32_t *sp = (uint32_t*)stack_top;

    /* Fake return address */
    *(--sp) = (uint32_t)entry;

    /* Fake register frame (popal compatible) */
    for (int i = 0; i < 8; i++)
        *(--sp) = 0;

    return sp;
}

/* ---------- API IMPLEMENTATION ---------- */

void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        proc_table[i].state = PROC_UNUSED;
        proc_table[i].pid = 0;
        proc_table[i].msg_count = 0;
    }

    serial_puts("[process] initialized\n");
}

/* Create a new process */
int process_create(void (*entry)(void), uint32_t priority) {
    int slot = find_free_slot();
    if (slot < 0)
        return -1;

    void *stack = alloc_stack();
    if (!stack)
        return -1;

    pcb_t *p = &proc_table[slot];

    p->pid = next_pid++;
    p->state = PROC_READY;
    p->priority = priority;
    p->age = 0;

    p->stack_base = stack;
    p->stack_ptr  = init_stack(stack, entry);

    p->msg_count = 0;

    serial_puts("[process] created PID ");
    serial_putc('0' + p->pid);
    serial_puts("\n");

    return p->pid;
}

/* Terminate current process */
void process_exit(void) {
    serial_puts("[process] exit PID ");
    serial_putc('0' + current_proc->pid);
    serial_puts("\n");

    current_proc->state = PROC_TERMINATED;
    free_stack(current_proc->stack_base);

    /* Scheduler will pick next */
}

/* State transition */
void process_set_state(int pid, proc_state_t state) {
    pcb_t *p = process_get(pid);
    if (p)
        p->state = state;
}

proc_state_t process_get_state(int pid) {
    pcb_t *p = process_get(pid);
    if (!p)
        return PROC_UNUSED;
    return p->state;
}

/* Utilities */
pcb_t* process_get(int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (proc_table[i].pid == (uint32_t)pid)
            return &proc_table[i];
    }
    return 0;
}

int process_current_pid(void) {
    if (!current_proc)
        return -1;
    return current_proc->pid;
}

/* ---------- IPC ---------- */

int process_send(int dest_pid, uint32_t value) {
    pcb_t *dest = process_get(dest_pid);
    if (!dest || dest->state == PROC_UNUSED)
        return -1;

    if (dest->msg_count >= MAX_MESSAGES)
        return -1;

    dest->msg_queue[dest->msg_count].sender_pid = current_proc->pid;
    dest->msg_queue[dest->msg_count].value = value;
    dest->msg_count++;

    return 0;
}

int process_receive(uint32_t *out_value) {
    if (current_proc->msg_count == 0)
        return -1;

    *out_value = current_proc->msg_queue[0].value;

    /* Shift queue */
    for (uint32_t i = 1; i < current_proc->msg_count; i++)
        current_proc->msg_queue[i - 1] = current_proc->msg_queue[i];

    current_proc->msg_count--;
    return 0;
}
