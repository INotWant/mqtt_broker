#ifndef MEMORY_MANAGEMENT_API_H
#define MEMORY_MANAGEMENT_API_H

/**
 * 内存管理初始化
 * @param fragment_num 内存池的个数
 * @param fragments_size 各级内存池中的内存大小
 * @param fragments_count 各级内存池中的内存数量
 * @return 错误代码
 */
int memory_management_init(int fragment_num, int fragments_size[], int fragments_count[]);

/**
 * 申请内存
 * @param size 所要申请内存的大小
 * @param pp_memory 用于存储申请到的内存首地址
 * @return 错误代码
 */
int memory_management_allocate(int size, void **pp_memory);

/**
 * 内存归还给“内存管理”
 * @param pp_memory  指向所要释放的内存的指针
 * @param size  申请的内存大小
 */
int memory_management_free(void **pp_memory, int size);

/**
 * 释放所有资源
 * （请确保最后调用此）
 */
void memory_management_clear();

/**
 * 获取分级内存池的数量
 * @return 分级数量
 */
int get_fragments_num();

/**
 * 获取每级内存池中内存的大小
 */
void get_fragments_size(int fragments_size[]);

/**
 * 获取每级内存池中内存的数量
 */
void get_fragments_count(int fragments_count[]);

/**
 * 获取申请的内存所在的内存池的索引
 * 返回值 -1 ，表示不在各级内存池中（假设之前已经申请成功）
 * @param size 所申请的内存的大小
 * @return 索引号
 */
int get_fragment_index(int size);

/**
 * 获取内存池总的分配内存的次数
 * @return 总的被申请的数目
 */
int get_fragments_allocate_number();

/**
 * 计算内存管理总共申请的内存大小
 * @return 占用的内存大小
 */
int memory_management_occupied();

/**
 * 打印内存管理信息
 */
void memory_management_print();

#endif //MEMORY_MANAGEMENT_API_H
