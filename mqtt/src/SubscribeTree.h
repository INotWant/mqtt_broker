#ifndef MQTT_BROKER_SUBSCRIBE_TREE_H
#define MQTT_BROKER_SUBSCRIBE_TREE_H

#include "Define.h"
#include "Session.h"

/******************************************************************
 *                       ������ ���������
 ******************************************************************/

/**
 * �������� ���� �������ͻ���
 */
typedef struct _ClientListNode {
    Session *pSession;                  // �Ựָ��
    int qos;                            // ���ĵķ�������
    SubscribeListNode *pListNode;       // �ڶ��������е�λ�ã��û����ĵ�ȡ���Ȳ�����Ҫ��
    struct _ClientListNode *pNext;
    struct _ClientListNode *pPre;
} ClientListNode;

/**
 * ���������
 */
typedef struct _SubscribeTreeNode {
    char *pCurrentTopicName;            // ��ǰ�������
    int currentTopicNameLength;
    int clientNum;                      // �������������Ӧ�Ŀͻ����ж��ٸ�
    ClientListNode *pClientHead;        // ��������������Ŀͻ��˵�ͷ���
    /* ά�������������ӡ����ֵ� */
    struct _SubscribeTreeNode *pParent;
    struct _SubscribeTreeNode *pChild;
    struct _SubscribeTreeNode *pBrother;
} SubscribeTreeNode;

/**
 * ����һ���µĶ��Ľ��
 * ע��ע��ʹ�� mm ��ȡ�ڴ�
 */
SubscribeTreeNode *
createSubscribeTreeNode(char *pCurrentTopicName, int currentTopicNameLength);

/**
 * ���������в���һ�� ���������
 */
SubscribeTreeNode *
insertSubscribeTreeNode(SubscribeTreeNode *pSubscribeTreeRoot, char *pTopic, int topicLength, Session *pSession, int qos,
                        SubscribeListNode *pListNode, char *pReturnCode);

/**
 * �Ӷ�������ɾ��ָ�� ���������
 */
void deleteSubscribeTreeNodeByTopicFilter(SubscribeTreeNode *pSubscribeTreeRoot, char *pTopic, int topicLength,
                                          Session *pSession);

/**
 * �Ӷ�������ɾ��ָ�����
 */
void
deleteSubscribeTreeNodeByAddress(SubscribeTreeNode *pSubscribeTreeRoot, SubscribeTreeNode *pNode, Session *pSession);

/**
 * ɾ������������
 */
void deleteSubscribeTree(SubscribeTreeNode *pSubscribeTreeRoot);

/**
 * �Ӷ������м��������ж���ָ������Ŀͻ���
 */
void findAllClientInSubscribeTree(SubscribeTreeNode *pSubscribeTreeRoot, char *pTopic, int topicLength, HashTable *pHashTable);

#endif // MQTT_BROKER_SUBSCRIBE_TREE_H





