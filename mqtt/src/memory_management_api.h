#ifndef MEMORY_MANAGEMENT_API_H
#define MEMORY_MANAGEMENT_API_H

/**
 * �ڴ�����ʼ��
 * @param fragment_num �ڴ�صĸ���
 * @param fragments_size �����ڴ���е��ڴ��С
 * @param fragments_count �����ڴ���е��ڴ�����
 * @return �������
 */
int memory_management_init(int fragment_num, int fragments_size[], int fragments_count[]);

/**
 * �����ڴ�
 * @param size ��Ҫ�����ڴ�Ĵ�С
 * @param pp_memory ���ڴ洢���뵽���ڴ��׵�ַ
 * @return �������
 */
int memory_management_allocate(int size, void **pp_memory);

/**
 * �ڴ�黹�����ڴ����
 * @param pp_memory  ָ����Ҫ�ͷŵ��ڴ��ָ��
 * @param size  ������ڴ��С
 */
int memory_management_free(void **pp_memory, int size);

/**
 * �ͷ�������Դ
 * ����ȷ�������ôˣ�
 */
void memory_management_clear();

/**
 * ��ȡ�ּ��ڴ�ص�����
 * @return �ּ�����
 */
int get_fragments_num();

/**
 * ��ȡÿ���ڴ�����ڴ�Ĵ�С
 */
void get_fragments_size(int fragments_size[]);

/**
 * ��ȡÿ���ڴ�����ڴ������
 */
void get_fragments_count(int fragments_count[]);

/**
 * ��ȡ������ڴ����ڵ��ڴ�ص�����
 * ����ֵ -1 ����ʾ���ڸ����ڴ���У�����֮ǰ�Ѿ�����ɹ���
 * @param size ��������ڴ�Ĵ�С
 * @return ������
 */
int get_fragment_index(int size);

/**
 * ��ȡ�ڴ���ܵķ����ڴ�Ĵ���
 * @return �ܵı��������Ŀ
 */
int get_fragments_allocate_number();

/**
 * �����ڴ�����ܹ�������ڴ��С
 * @return ռ�õ��ڴ��С
 */
int memory_management_occupied();

/**
 * ��ӡ�ڴ������Ϣ
 */
void memory_management_print();

#endif //MEMORY_MANAGEMENT_API_H
