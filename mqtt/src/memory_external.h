#ifndef MEMORY_EXTERNAL_H
#define MEMORY_EXTERNAL_H

#include "memory_management.h"

int memory_external_init(struct memory_external *p_external, int size) {
    p_external->size = size;
    p_external->p_memory = malloc(size);
    if (!p_external->p_memory) {
        return ERR_NOMEM;
    }
    p_external->pre = 0;
    p_external->next = 0;
    return ERR_SUCCESS;
}

struct memory_external *get_last_memory_external(struct memory_external *p_external) {
    struct memory_external *cp_external = p_external;
    while (cp_external->next != 0) {
        cp_external = cp_external->next;
    }
    return cp_external;
}

struct memory_external *find_memory_external(struct memory_external *p_external, void *p_memory) {
    struct memory_external *cp_external = p_external;
    while (cp_external != 0) {
        if (cp_external->p_memory == p_memory) {
            return cp_external;
        }
        cp_external = cp_external->next;
    }
    return 0;
}

void memory_external_clear(struct memory_external *p_external) {
    free(p_external->p_memory);
    free(p_external);
}

#endif  // MEMORY_EXTERNAL_H
