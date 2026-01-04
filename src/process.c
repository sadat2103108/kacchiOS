#include "process.h"
#include "memory.h"
#include "serial.h"

pcb_t proc_table[MAX_PROCESSES];
pcb_t *current_proc = 0;

static uint32_t next_pid = 1;
static uint32_t process_count = 0;

/* Find free process table slot */
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

    /* Push registers in reverse order of restoration (EAX, EBX, ECX, EDX, ESI, EDI, EBP) */
    *(--sp) = 0;                        /* EAX */
    *(--sp) = 0;                        /* EBX */
    *(--sp) = 0;                        /* ECX */
    *(--sp) = 0;                        /* EDX */
    *(--sp) = 0;                        /* ESI */
    *(--sp) = 0;                        /* EDI */
    *(--sp) = 0;                        /* EBP */
    *(--sp) = (uint32_t)entry;          /* Return address (process entry point) */

    return sp;
}

/* ---------- API IMPLEMENTATION ---------- */

void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        proc_table[i].state = PROC_UNUSED;
        proc_table[i].pid = 0;
        proc_table[i].msg_count = 0;
        proc_table[i].priority = 10;
        proc_table[i].age = 0;
    }

    process_count = 0;
    serial_puts("[process] initialized (max=");
    serial_put_num(MAX_PROCESSES);
    serial_puts(" processes)\n");
}

/* Create a new process with priority */
int process_create(void (*entry)(void), uint32_t priority) {
    int slot = find_free_slot();
    if (slot < 0) {
        serial_puts("[process] FAIL: process table full\n");
        return -1;
    }

    void *stack = alloc_stack();
    if (!stack) {
        serial_puts("[process] FAIL: no memory for stack\n");
        return -1;
    }

    pcb_t *p = &proc_table[slot];

    p->pid = next_pid++;
    p->state = PROC_READY;
    p->priority = priority < 1 ? 1 : (priority > 20 ? 20 : priority);
    p->age = 0;

    p->stack_base = (uint32_t*)stack;
    p->stack_ptr  = init_stack(stack, entry);

    p->msg_count = 0;

    process_count++;

    serial_puts("[process] created PID ");
    serial_put_num(p->pid);
    serial_puts(" (priority=");
    serial_put_num(p->priority);
    serial_puts(")\n");

    return p->pid;
}

/* Terminate current process */
void process_exit(void) {
    if (!current_proc) {
        serial_puts("[process] ERROR: no current process\n");
        return;
    }

    serial_puts("[process] exit PID ");
    serial_put_num(current_proc->pid);
    serial_puts(" (state=TERMINATED)\n");

    current_proc->state = PROC_TERMINATED;
    free_stack(current_proc->stack_base);

    if (process_count > 0)
        process_count--;

    /* Scheduler will pick next */
}

/* State transition with validation */
void process_set_state(int pid, proc_state_t state) {
    pcb_t *p = process_get(pid);
    if (!p) {
        serial_puts("[process] ERROR: invalid PID\n");
        return;
    }

    p->state = state;

    /* Log state transitions */
    serial_puts("[process] PID ");
    serial_put_num(pid);
    serial_puts(" state changed\n");
}

proc_state_t process_get_state(int pid) {
    pcb_t *p = process_get(pid);
    if (!p)
        return PROC_UNUSED;
    return p->state;
}

/* ---------- UTILITIES ---------- */

/* Get PCB by PID */
pcb_t* process_get(int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (proc_table[i].pid == (uint32_t)pid)
            return &proc_table[i];
    }
    return 0;
}

/* Get current process PID */
int process_current_pid(void) {
    if (!current_proc)
        return -1;
    return current_proc->pid;
}

/* Get process count */
uint32_t process_count_active(void) {
    uint32_t count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (proc_table[i].state != PROC_UNUSED)
            count++;
    }
    return count;
}

/* List all processes */
void process_list(void) {
    serial_puts("\n========== PROCESS TABLE ==========\n");
    
    uint32_t count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (proc_table[i].state != PROC_UNUSED) {
            count++;
            serial_puts("PID ");
            serial_put_num(proc_table[i].pid);
            serial_puts(": state=");
            
            /* Print state name */
            switch (proc_table[i].state) {
                case PROC_READY:      serial_puts("READY"); break;
                case PROC_RUNNING:    serial_puts("RUNNING"); break;
                case PROC_BLOCKED:    serial_puts("BLOCKED"); break;
                case PROC_SLEEPING:   serial_puts("SLEEPING"); break;
                case PROC_TERMINATED: serial_puts("TERMINATED"); break;
                default:              serial_puts("UNKNOWN");
            }
            serial_puts(", priority=");
            serial_put_num(proc_table[i].priority);
            serial_puts("\n");
        }
    }
    
    serial_puts("Total processes: ");
    serial_put_num(count);
    serial_puts("\n");
    serial_puts("===================================\n\n");
}

/* ---------- IPC IMPLEMENTATION ---------- */

/* Send message to another process */
int process_send(int dest_pid, uint32_t value) {
    if (!current_proc) {
        serial_puts("[IPC] ERROR: no current process\n");
        return -1;
    }

    pcb_t *dest = process_get(dest_pid);
    if (!dest || dest->state == PROC_UNUSED) {
        serial_puts("[IPC] ERROR: invalid destination PID\n");
        return -1;
    }

    if (dest->msg_count >= MAX_MESSAGES) {
        serial_puts("[IPC] ERROR: message queue full\n");
        return -1;
    }

    dest->msg_queue[dest->msg_count].sender_pid = current_proc->pid;
    dest->msg_queue[dest->msg_count].value = value;
    dest->msg_count++;

    serial_puts("[IPC] message sent from PID ");
    serial_put_num(current_proc->pid);
    serial_puts(" to PID ");
    serial_put_num(dest_pid);
    serial_puts("\n");

    return 0;
}

/* Receive message from message queue */
int process_receive(uint32_t *out_value) {
    if (!current_proc) {
        serial_puts("[IPC] ERROR: no current process\n");
        return -1;
    }

    if (current_proc->msg_count == 0) {
        serial_puts("[IPC] no message available\n");
        return -1;
    }

    *out_value = current_proc->msg_queue[0].value;

    /* Shift queue */
    for (uint32_t i = 1; i < current_proc->msg_count; i++)
        current_proc->msg_queue[i - 1] = current_proc->msg_queue[i];

    current_proc->msg_count--;

    serial_puts("[IPC] received message value=");
    serial_put_num(*out_value);
    serial_puts("\n");

    return 0;
}
