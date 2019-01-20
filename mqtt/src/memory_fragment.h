#ifndef MEMORY_FRAGMENT_H
#define MEMORY_FRAGMENT_H

// for test
#ifdef TEST_SEM
#include "taskLib.h"
#include "sysLib.h"
#endif

#include "memory_management.h"

int memory_fragment_init(struct memory_fragment *p_fragment, int size, int count) {
    p_fragment->size = size;
    p_fragment->count = count;
    p_fragment->p_memory = malloc(size * count);
    if (!p_fragment->p_memory) {
        return ERR_NOMEM;
    }
    p_fragment->is_allocated = (char *)malloc(count);
    if (!p_fragment->is_allocated) {
		free(p_fragment->p_memory);
        return ERR_NOMEM;
    }
    for (int i = 0; i < count; i++) {
        p_fragment->is_allocated[i] = false;
    }
    p_fragment->remain_count = count;
    p_fragment->allocate_num = 0;
    p_fragment->semM = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
    return ERR_SUCCESS;
}

int memory_fragment_allocate(struct memory_fragment *p_fragment, void **pp_memory) {

    // for test
    #ifdef TEST_SEM
    printf("Before\n");
    #endif

    // 加锁
    semTake(p_fragment->semM, WAIT_FOREVER);

    // for test 
    #ifdef TEST_SEM
    printf("After1\n");
    taskDelay(sysClkRateGet() * 3);
    printf("After2\n");
    #endif

    if (p_fragment->remain_count == 0) {
        // 释放
		semGive(p_fragment->semM);
        return ERR_BUFF_FILL;
    } else {
        for (int i = 0; i < p_fragment->count; i++) {
            if (p_fragment->is_allocated[i] == false) {
                *pp_memory = (char *)p_fragment->p_memory + (i * p_fragment->size);
                p_fragment->is_allocated[i] = true;
                p_fragment->remain_count -= 1;
                p_fragment->allocate_num += 1;
                break;
            }
        }
    }
    // 释放
    semGive(p_fragment->semM);
    return ERR_SUCCESS;
}

bool memory_in_fragment(struct memory_fragment *p_fragment, void *p_memory) {
    return p_memory >= p_fragment->p_memory &&
           p_memory < ((char *)p_fragment->p_memory + p_fragment->size * p_fragment->count)
           ? true : false;
}

void memory_fragment_free(struct memory_fragment *p_fragment, void *p_memory) {
    char *sp_memory = (char *)p_fragment->p_memory;
    int index = (int) (((char *) p_memory - sp_memory) / p_fragment->size);
    // 加锁
    semTake(p_fragment->semM, WAIT_FOREVER);
    p_fragment->is_allocated[index] = false;
    p_fragment->remain_count += 1;
    // 释放
    semGive(p_fragment->semM);
}

void memory_fragment_clear(struct memory_fragment *p_fragment) {
    free(p_fragment->p_memory);
    free(p_fragment->is_allocated);
    semDelete(p_fragment->semM);
    free(p_fragment);
}

#endif  // MEMORY_FRAGMENT_H
