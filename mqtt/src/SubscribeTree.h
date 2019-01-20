#ifndef MQTT_BROKER_SUBSCRIBE_TREE_H
#define MQTT_BROKER_SUBSCRIBE_TREE_H

#include "Define.h"
#include "Session.h"

/******************************************************************
 *                       订阅树 的相关声明
 ******************************************************************/

/**
 * 用于描述 订阅 的所属客户端
 */
typedef struct _ClientListNode {
    Session *pSession;                  // 会话指针
    int qos;                            // 订阅的服务质量
    SubscribeListNode *pListNode;       // 在订阅链表中的位置（用户订阅的取消等操作需要）
    struct _ClientListNode *pNext;
    struct _ClientListNode *pPre;
} ClientListNode;

/**
 * 订阅树结点
 */
typedef struct _SubscribeTreeNode {
    char *pCurrentTopicName;            // 当前层的主题
    int currentTopicNameLength;
    int clientNum;                      // 该主题过滤器对应的客户端有多少个
    ClientListNode *pClientHead;        // 主题过滤器所属的客户端的头结点
    /* 维护订阅树：左孩子、右兄弟 */
    struct _SubscribeTreeNode *pParent;
    struct _SubscribeTreeNode *pChild;
    struct _SubscribeTreeNode *pBrother;
} SubscribeTreeNode;

/**
 * 创建一个新的订阅结点
 * 注：注：使用 mm 获取内存
 */
SubscribeTreeNode *
createSubscribeTreeNode(char *pCurrentTopicName, int currentTopicNameLength);

/**
 * 往订阅树中插入一个 主题过滤器
 */
SubscribeTreeNode *
insertSubscribeTreeNode(SubscribeTreeNode *pSubscribeTreeRoot, char *pTopic, int topicLength, Session *pSession, int qos,
                        SubscribeListNode *pListNode, char *pReturnCode);

/**
 * 从订阅树中删除指定 主题过滤器
 */
void deleteSubscribeTreeNodeByTopicFilter(SubscribeTreeNode *pSubscribeTreeRoot, char *pTopic, int topicLength,
                                          Session *pSession);

/**
 * 从订阅树中删除指定结点
 */
void
deleteSubscribeTreeNodeByAddress(SubscribeTreeNode *pSubscribeTreeRoot, SubscribeTreeNode *pNode, Session *pSession);

/**
 * 删除整个订阅树
 */
void deleteSubscribeTree(SubscribeTreeNode *pSubscribeTreeRoot);

/**
 * 从订阅树中检索出所有订阅指定主题的客户端
 */
void findAllClientInSubscribeTree(SubscribeTreeNode *pSubscribeTreeRoot, char *pTopic, int topicLength, HashTable *pHashTable);

#endif // MQTT_BROKER_SUBSCRIBE_TREE_H





