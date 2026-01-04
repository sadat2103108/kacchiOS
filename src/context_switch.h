#ifndef CONTEXT_SWITCH_H
#define CONTEXT_SWITCH_H

#include "types.h"

// --- Context Switch Assembly Functions ---
extern void context_switch_asm(uint32_t **current_sp, uint32_t **next_sp);
extern void save_context(void);
extern void restore_context(void);

#endif