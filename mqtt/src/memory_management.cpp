#include "memory_management.h"
#include "memory_management_api.h"
#include "memory_fragment.h"
#include "memory_external.h"

struct memory_management management;
struct memory_management *p_management = &management;

int memory_management_init(int fragment_num, int fragments_size[], int fragments_count[]) {
    p_management->fragment_num = fragment_num;
    p_management->external_allocate_num = 0;
    p_management->fragments_size = (int *) malloc(sizeof(int) * p_management->fragment_num);
    if (!p_management->fragments_size) {
        return ERR_NOMEM;
    }
    p_management->fragments_count = (int *) malloc(sizeof(int) * p_management->fragment_num);
    if (!p_management->fragments_count) {
		free(p_management->fragments_size);
        return ERR_NOMEM;
    }
    p_management->p_fragments = (struct memory_fragment **) malloc(
            sizeof(struct memory_fragment *) * p_management->fragment_num);
    if (!p_management->p_fragments) {
		free(p_management->fragments_size);
		free(p_management->fragments_count);
        return ERR_NOMEM;
    }

    for (int i = 0; i < p_management->fragment_num; i++) {
        p_management->fragments_size[i] = fragments_size[i];
        p_management->fragments_count[i] = fragments_count[i];
        p_management->p_fragments[i] = (struct memory_fragment *) malloc(sizeof(struct memory_fragment));
        int error_value = memory_fragment_init(p_management->p_fragments[i], fragments_size[i], fragments_count[i]);
        if (error_value == ERR_NOMEM) {
			p_management->fragment_num = i;
			memory_management_clear();
            return ERR_NOMEM;
        }
    }
    p_management->p_external = 0;
    p_management->semM = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
    printf("[Info]: init memory management success!\n");
    return ERR_SUCCESS;
}

int memory_management_allocate(int size, void **pp_memory) {
    int index = get_fragment_index(size);
    if (index != -1) {
        // 在指定内存池中分配
        int error_value = memory_fragment_allocate(p_management->p_fragments[index], pp_memory);
        if (error_value == ERR_SUCCESS) {
            return ERR_SUCCESS;
        }
    }
    // 在外面分配（包括指定内存池已满的情况）
    struct memory_external *p_external = (struct memory_external *) malloc(sizeof(struct memory_external));
    if (!p_external) {
        return ERR_NOMEM;
    }
    int error_value = memory_external_init(p_external, size);
    if (error_value == ERR_NOMEM) {
		free(p_external);
        return ERR_NOMEM;
    }
    // 加锁
    semTake(p_management->semM, WAIT_FOREVER);
    if (p_management->p_external == 0) {
        p_management->p_external = p_external;
    } else {
        struct memory_external *lp_external = get_last_memory_external(p_management->p_external);
        lp_external->next = p_external;
        p_external->pre = lp_external;
    }
    p_management->external_allocate_num += 1;
    *pp_memory = p_external->p_memory;
    // 释放
    semGive(p_management->semM);
    return ERR_SUCCESS;
}


int memory_management_free(void **pp_memory, int size) {
    int index = get_fragment_index(size);
    if (index != -1 && memory_in_fragment(p_management->p_fragments[index], *pp_memory) == true) {
        // 在指定内存池中（并且在开辟的内存空间中，不在是因为申请时指定内存池已满）
        memory_fragment_free(p_management->p_fragments[index], *pp_memory);
    } else {
        // 加锁
		semTake(p_management->semM, WAIT_FOREVER);
        struct memory_external *p_external = find_memory_external(p_management->p_external, *pp_memory);
        if (!p_external) {
            // 释放
			semGive(p_management->semM);
            return ERR_ILL_ADDR;
        } else {
            if (p_external->pre == 0) {
                p_management->p_external = p_external->next;
                if (p_external->next != 0) {
                    p_external->next->pre = 0;
                }
            } else {
                p_external->pre->next = p_external->next;
                if (p_external->next != 0) {
                    p_external->next->pre = p_external->pre;
                }
            }
            // 释放
			semGive(p_management->semM);
            memory_external_clear(p_external);
        }
    }
    *pp_memory = 0;
	return ERR_SUCCESS;
}

void memory_management_clear() {
    // 清除各级内存池
    for (int i = 0; i < p_management->fragment_num; i++) {
        memory_fragment_clear(p_management->p_fragments[i]);
    }
    free(p_management->fragments_size);
    free(p_management->fragments_count);
    free(p_management->p_fragments);
    p_management->fragments_size = 0;
    p_management->fragments_count = 0;
    p_management->p_fragments = 0;
    p_management->fragment_num = 0;
    // 清除非内存池中的
    p_management->external_allocate_num = 0;
    while (p_management->p_external != 0) {
        struct memory_external *np_external = p_management->p_external->next;
        memory_external_clear(p_management->p_external);
        p_management->p_external = np_external;
    }
    semDelete(p_management->semM);
}

int get_fragments_num() {
    return p_management->fragment_num;
}

void get_fragments_size(int fragments_size[]) {
    for (int i = 0; i < p_management->fragment_num; i++) {
        fragments_size[i] = p_management->fragments_size[i];
    }
}

void get_fragments_count(int fragments_count[]) {
    for (int i = 0; i < p_management->fragment_num; i++) {
        fragments_count[i] = p_management->fragments_count[i];
    }
}

int get_fragment_index(int size) {
    for (int i = 0; i < p_management->fragment_num; i++) {
        if (p_management->fragments_size[i] >= size) {
            return i;
        }
    }
    return -1;
}

int get_fragments_allocate_number() {
    int sum = 0;
    for (int i = 0; i < p_management->fragment_num; ++i) {
        sum += p_management->p_fragments[i]->allocate_num;
    }
    return sum;
}

int memory_management_occupied() {
    int sum = 0;
    for (int i = 0; i < p_management->fragment_num; ++i) {
        sum += p_management->fragments_size[i] * p_management->fragments_count[i];
    }
    struct memory_external *cp_external = p_management->p_external;
    while (cp_external != 0) {
        sum += cp_external->size;
        cp_external = cp_external->next;
    }
    return sum;
}

void memory_management_print() {
    printf("%8s%8s%8s%8s%8s%9s\n", "========", "========", "========", "========", "========", "=========");
    printf("%8s%8s%8s%8s%8s%9s\n", "------- ", " ------ ", " memory ", "  pools ", " ------ ", "-------- ");
    printf("%-8s%-8s%-8s%-8s%-8s%-9s\n", "level", "bytes", "count", "used", "remain", "alloc_num");
    if (p_management->fragment_num == 0) {
        printf("%8s%8s%8s%8s%8s%8s\n", "   --   ", "   --   ", "   --   ", "   --   ", "   --   ", "   --   ");
    } else {
        for (int i = 0; i < p_management->fragment_num; ++i) {
            struct memory_fragment *p_fragment = *(p_management->p_fragments + i);
            printf("%-8d%-8d%-8d%-8d%-8d%-8d\n", (i + 1), p_fragment->size, p_fragment->count,
                   (p_fragment->count - p_fragment->remain_count), p_fragment->remain_count, p_fragment->allocate_num);
        }
    }
    printf("%8s%8s%8s%8s%8s%9s\n", "------- ", " ------ ", " out of", " pools  ", " ------ ", "-------- ");
    printf("%8s%8s%8s%8s%8s%8s\n", "   --   ", "   index", "        ", "        ", "bytes   ", "   --   ");
    struct memory_external *cp_external = p_management->p_external;
    if (cp_external == 0) {
        printf("%8s%8s%8s%8s%8s%8s\n", "   --   ", "    -   ", "        ", "        ", "   -    ", "   --   ");
    } else {
        int count = 0;
        while (cp_external != 0) {
            printf("%8s%8d%8s%8s%-8d%8s\n", "   --   ", ++count, "        ", "        ", cp_external->size, "   --   ");
            cp_external = cp_external->next;
        }
    }
    printf("%8s%8s%8s%8s%-8d%8s\n", "   --   ", "alloc_nu", "m       ", "        ", p_management->external_allocate_num,
           "   --   ");
    printf("%8s%8s%8s%8s%8s%9s\n", "------- ", " ------ ", " --- tot", "al ---  ", "------  ", "-------- ");
    printf("%8s%8s%8s%8d\n", "Occupied", " Memory:", "        ", memory_management_occupied());
    printf("%8s%8s%8s%8d\n", "Allocate", " Number:", "        ",
           p_management->external_allocate_num + get_fragments_allocate_number());
    printf("%8s%8s%8s%8s%8s%9s\n", "========", "========", "========", "========", "========", "=========");
}