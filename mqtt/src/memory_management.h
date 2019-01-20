#ifndef MEMORY_MANAGEMENT_H
#define MEMORY_MANAGEMENT_H

#define bool char
#define true 1
#define false 0

// #define TEST_SEM

#include "vxWorks.h"
#include "memLib.h"
#include "stdLib.h"
#include "stdio.h"
#include "semLib.h"

/**
 *  ÿ���ڴ�صĳ���
 */
struct memory_fragment {
    int size;
    int count;
    int remain_count;
    int allocate_num;
    void *p_memory;
    bool *is_allocated;
    SEM_ID semM;
};

/**
 *  ���������ڴ�����ڴ���޷����㣬�������ⲿ�ĳ���
 *  һ�������Ӧһ���������������һ������
 */
struct memory_external {
    int size;
    void *p_memory;
    struct memory_external *pre;
    struct memory_external *next;
};

/**
 * �ڴ����
 */
struct memory_management {
    int fragment_num;
    int *fragments_size;
    int *fragments_count;
    struct memory_fragment **p_fragments;
    struct memory_external *p_external;
    int external_allocate_num;
    SEM_ID semM;
};

/**
 * �������
 */
enum error_value {
    ERR_SUCCESS = 0,
    ERR_NOMEM = 1,
    ERR_BUFF_FILL = 2,
	ERR_ILL_ADDR = 3,
};

#endif //MEMORY_MANAGEMENT_H
