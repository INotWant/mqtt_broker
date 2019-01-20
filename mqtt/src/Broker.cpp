#include "Broker.h"

Broker broker;
HashTable *pClientHashTable;              // 存放要分发给的所有客户端
HashTable *pCMHashTable;                                    // key: clientId 与 messageId 的拼接（取clientId的低16位做新产生的高16位，messageId的低16位做新产生的低16位）
// value: 未完成的分发数
// 用于在全部分发完成后，释放 message 占用的动态申请的内存
SEM_ID semCMHashTable;                                        // 对	pCMHashTable 的互斥访问

// TODO 1) 所以这里要求所有的报文都要在可变报头之后、有效载荷之前加上 clientId （非 mqtt 标准）
// TODO 2) 考虑到经常查询 Session 链表（O(n)），是否需要把它设计为 Hash 表
// TODO 3) 就像 HashTable 那样是否可以把链表抽出来，写一个通用的链表！
// TODO 4) 为制定统一消息！！！

/******************************************************************
 *                       一些内部函数声明
 ******************************************************************/
void responseConnect(int clientId, UINT8 sp, UINT8 returnCode);

void responseSubscribe(int clientId, unsigned int messageId, const char *returnCodeArray, int topicFilterNum);

void responseUnsubscribe(int clientId, unsigned int messageId);

void responsePingreq(int clientId);

void sendPuback(int clientId, int messageId);

void sendPublishQos0(int clientId, char *pTopic, int topicLength, char *pContent, int contentLength, int key,
                     char *pPublishMessage);

void
sendPublishQos1(Session *pSession, char *pTopic, int topicLength, char *pContent, int contentLength, int messageId,
                int key, char *pPublishMessage);

void
sendPublishQos2(Session *pSession, char *pTopic, int topicLength, char *pContent, int contentLength, int messageId,
                int key, char *pPublishMessage);

void responsePublishQos2(Session *pSession, int messageId);

/**
 * Broker 初始化
 */
int initBroker() {
    // 初始化内存管理
    int fragments_num = 3;
    int fragments_size[] = {16, 128, 512};
    int fragments_count[] = {32, 16, 8};
    int errorValue = memory_management_init(fragments_num, fragments_size, fragments_count);
    if (errorValue) {
        return errorValue;
    }
    // 初始化定时器
    eTimerManagementInit();
    // 初始化 Broker 内部
    broker.msgId = msgQCreate(MAX_MESSAGE_NUM, MAX_MESSAGE_LENGTH, MSG_Q_PRIORITY);
    // 安全初始化
    broker.pSessionHead = NULL;
    broker.pRetainMessageHead = NULL;
    // 为订阅树创建 # 与 / 结点
    char currentTopicName = '#';
    broker.pSubscribeTreeRoot = createSubscribeTreeNode(&currentTopicName, 1);
    currentTopicName = '+';
    broker.pSubscribeTreeRoot->pBrother = createSubscribeTreeNode(&currentTopicName, 1);
    // 初始化 哈希表
    pClientHashTable = hash_table_new();
    pCMHashTable = hash_table_new();
    semCMHashTable = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
    printf("[INFO]:Broker init success!\n");
    return BROKER_ERR_SUCCESS;
}

/**
 * 返回 Broker
 */
Broker *getBroker() {
    return &broker;
}

/**
 * 启动 Broker
 */
void startBroker() {
    printf("[INFO]:Broker start!\n");
    while (True) {
        // 使用动态内存保存 报文，在后面的流程中一定要在处理完该报文时把该内存释放
        char *pMessage;
        memory_management_allocate(MAX_MESSAGE_LENGTH, (void **) &pMessage);
        int mLen = msgQReceive(broker.msgId, (char *) pMessage, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
        if (mLen <= 0) {
            printf("[Error]: message len!\n");
            memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
            continue;
        } else {
            // 拆包
            FixedHeader fixedHeader;
            // 1）获取固定报头
            int byteNum = getFixedHeader(pMessage, &fixedHeader);
            // 2）根据控制报文类型做相应操作
            switch (fixedHeader.messageType) {
                case CONNECT: {
                    // 获取 CONNECT 的可变报头
                    ConnVariableHeader connVariableHeader;
                    byteNum += getConnVariableHeader(pMessage + byteNum, &connVariableHeader);
                    // 获取 CONNECT 的有效载荷
                    ConnPayload connPayload;
                    getConnPayload(pMessage + byteNum, &connPayload);
                    // 查找该会话之前是否存在
                    printf("[INFO]: Get a connect, clientId is %d.\n", connPayload.clientId);
                    Session *pSession = findSession(connPayload.clientId, broker.pSessionHead);
                    boolean spFlag = pSession == NULL ? False : True;
                    if (connVariableHeader.cleanSession) {
                        // 要清理会话状态，如果之前有对应的会话状态，则需要删除它
                        if (pSession != NULL) {
                            deleteSession(broker.pSubscribeTreeRoot, &broker.pSessionHead, pSession);
                        }
                        printf("[INFO]keepAlive:\t%d\n", connVariableHeader.keepAlive);
                        // 创建新的会话，并将新创建的插入到会话状态链表中
                        // 创建新的会话，已经对 lastReqTime 进行设置，下面无需再设置
                        pSession = createSession(connPayload.clientId, connVariableHeader.cleanSession,
                                                 connVariableHeader.keepAlive);
                        insertSession(pSession, &broker.pSessionHead);
                        printf("[INFO]: Create a session for clientID(%d).\n", connPayload.clientId);
                    } else {
                        // 保留该会话状态
                        if (pSession != NULL) {
                            // 获取当前时间戳，修改上次请求时间，使得 客户端变活
                            modifySessionLastReqTime(getCurrentTime(), pSession);
                            printf("[INFO]: Use a old session for clientID(%d).\n", connPayload.clientId);
                        } else {
                            // 创建新的会话，并将新创建的插入到会话状态链表中
                            pSession = createSession(connPayload.clientId, connVariableHeader.cleanSession,
                                                     connVariableHeader.keepAlive);
                            insertSession(pSession, &broker.pSessionHead);
                            printf("[INFO]: Create a session for clientID(%d).\n", connPayload.clientId);
                        }
                    }
                    // 响应客户端
                    responseConnect(connPayload.clientId, spFlag, 0x00);
                    // 善后处理
                    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
                    break;
                }
                case PUBLISH: {
                    // 获取 publish 中固定报头中的一些标志
                    boolean dupFlag = getCharSpecialBit(&fixedHeader.flag, 3);
                    UINT8 qos = getCharSpecialBit(&fixedHeader.flag, 2) * 2;
                    qos += getCharSpecialBit(&fixedHeader.flag, 1);
                    boolean retainFlag = getCharSpecialBit(&fixedHeader.flag, 0);
                    // 获取 publish 中的可变报头
                    PublishVariableHeader publishVariableHeader;
                    int variableHeaderLength = getPublishVariableHeader(pMessage + byteNum, &publishVariableHeader,
                                                                        qos);
                    byteNum += variableHeaderLength;
                    // 获取 publish 中的有效载荷
                    PublishPayload publishPayload;
                    getPublishPayload(pMessage + byteNum, &publishPayload, fixedHeader.remainLength,
                                      variableHeaderLength);
                    // 获取客户端对应的会话状态，并更新会话状态中的上传通信时间
                    Session *pSession = findSession(publishPayload.clientId, broker.pSessionHead);
                    if (pSession != NULL && isInline(pSession)) {
                        // 获取时间戳，更新 lastReqTime
                        pSession->lastReqTime = getCurrentTime();
                        // 0）显示接收到的 publish
                        printf("[INFO]: receive a public, topic is ");
                        for (int i = 0; i < publishVariableHeader.topicLength; ++i) {
                            printf("%c", publishVariableHeader.pTopicHead[i]);
                        }
                        printf(", qos is %d\n", qos);
                        // 1）响应逻辑
                        if (qos == 1) {
                            sendPuback(publishPayload.clientId, publishVariableHeader.messageId);
                        } else if (qos == 2) {
                            // 加锁
                            semTake(pSession->semM, WAIT_FOREVER);
                            if (hash_table_get(pSession->pMessageIdHashTable, publishVariableHeader.messageId) !=
                                NULL) {
                                break;
                            }
                            hash_table_put(pSession->pMessageIdHashTable, publishVariableHeader.messageId, 1);
                            // 释放
                            semGive(pSession->semM);
                            // 使用新的 task 去响应
                            responsePublishQos2(pSession, publishVariableHeader.messageId);
                        }
                        // 2）分发逻辑
                        // 获取所有等待分发的客户端
                        findAllClientInSubscribeTree(broker.pSubscribeTreeRoot, publishVariableHeader.pTopicHead,
                                                     publishVariableHeader.topicLength, pClientHashTable);
                        // 遍历 pClientHashTable 进行分发（注：应把遍历写在 hashTable.h 中（借用 钩子 ））
                        // 随着遍历清空了整个 pClientHashTable
                        int lenClientHashTable = hash_table_len(pClientHashTable);
                        int key = (publishPayload.clientId << 16) + (publishVariableHeader.messageId & 0xFFFF);
                        // 加锁
                        semTake(semCMHashTable, WAIT_FOREVER);
                        hash_table_put(pCMHashTable, key, lenClientHashTable);
                        // 释放
                        semGive(semCMHashTable);
                        for (int i = 0; i < TABLE_SIZE; ++i) {
                            struct kv *p = pClientHashTable->table[i];
                            struct kv *q = NULL;
                            while (p) {
                                q = p->next;
                                // 获取分发的 qos
                                qos = p->value <= qos ? p->value : qos;
                                Session *pASession = (Session *) p->key;
                                if (isInline(pASession)) {
                                    if (qos == 0) {
                                        // 直接发布
                                        sendPublishQos0(pASession->clientId, publishVariableHeader.pTopicHead,
                                                        publishVariableHeader.topicLength, publishPayload.pContent,
                                                        publishPayload.contentLength, key, pMessage);
                                    } else if (qos == 1) {
                                        // 使用新的 task 去发布
                                        sendPublishQos1(pASession, publishVariableHeader.pTopicHead,
                                                        publishVariableHeader.topicLength, publishPayload.pContent,
                                                        publishPayload.contentLength, publishVariableHeader.messageId,
                                                        key, pMessage);
                                    } else if (qos == 2) {
                                        // 使用新的 task 去发布
                                        sendPublishQos2(pASession, publishVariableHeader.pTopicHead,
                                                        publishVariableHeader.topicLength, publishPayload.pContent,
                                                        publishPayload.contentLength, publishVariableHeader.messageId,
                                                        key, pMessage);
                                    } else {
                                        printf("[Error]: Invalid qos in process of processing PUBLISH.\n");
                                    }
                                } else {
                                    printf("[Error]: Client is dead in process of processing PUBLISH.\n");
                                }
                                free(p);
                                p = q;
                            }
                        }
                    } else {
                        printf("[Error]: Invalid clientID (or dead client) in process of processing PUBLISH.\n");
                    }
                    break;
                }
                case SUBSCRIBE: {
                    // 控制固定报头的保留位必须为 0x02
                    if (fixedHeader.flag != 0x02) {
                        printf("[Error]: Illegal flag in process of processing SUBSCRIBE.\n");
                        break;
                    }
                    printf("[INFO]: get a SUBSCRIBE!\n");
                    // 获取 SUBSCRIBE 的可变报头
                    CommonVariableHeader commonVariableHeader;
                    getCommonVariableHeader(pMessage + byteNum, &commonVariableHeader);
                    byteNum += 2;
                    // 获取 SUBSCRIBE 的有效载荷
                    SubscribePayload subscribePayload;
                    getSubscribePayload(pMessage + byteNum, &subscribePayload, fixedHeader.remainLength - 2);
                    // 主题过滤器 & QOS 输出
                    for (int j = 0; j < subscribePayload.topicFilterNum; ++j) {
                        printf("[INFO]: topicFilter is ");
                        for (int i = 0; i < subscribePayload.topicFilterLengthArray[j]; ++i) {
                            printf("%c", subscribePayload.pTopicFilterArray[j][i]);
                        }
                        printf(" ,qos is %d about clientId(%d) in process of processing SUBSCRIBE.\n",
                               subscribePayload.topicFilterQosArray[j], subscribePayload.clientId);
                    }
                    // 获取客户端对应的会话状态，并更新会话状态中的上传通信时间
                    Session *pSession = findSession(subscribePayload.clientId, broker.pSessionHead);
                    if (pSession != NULL && isInline(pSession)) {
                        // 获取时间戳，更新 lastReqTime
                        pSession->lastReqTime = getCurrentTime();
                        // 维护订阅等信息
                        char returnCodeArray[subscribePayload.topicFilterNum];
                        for (int i = 0; i < subscribePayload.topicFilterNum; ++i) {
                            SubscribeListNode *pNewSubscribeListNode = createSubscribeListNode(
                                    getTopicFilterDeep(subscribePayload.pTopicFilterArray[i],
                                                       subscribePayload.topicFilterLengthArray[i]));
                            insertSubscribeListNode(&pSession->pHead, pNewSubscribeListNode);
                            SubscribeTreeNode *pNewSubscribeTreeNode = insertSubscribeTreeNode(
                                    broker.pSubscribeTreeRoot,
                                    subscribePayload.pTopicFilterArray[i],
                                    subscribePayload.topicFilterLengthArray[i],
                                    pSession,
                                    subscribePayload.topicFilterQosArray[i], pNewSubscribeListNode,
                                    &returnCodeArray[i]);
                            pNewSubscribeListNode->pTreeNode = pNewSubscribeTreeNode;
                        }
                        // 响应客户端
                        responseSubscribe(subscribePayload.clientId, commonVariableHeader.messageId, returnCodeArray,
                                          subscribePayload.topicFilterNum);
                    } else {
                        printf("[Error]: Invalid clientID (or dead client) in process of processing SUBSCRIBE.\n");
                    }
                    // 善后处理
                    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
                    break;
                }
                case UNSUBSCRIBE: {
                    // 控制固定报头的保留位必须为 0x02
                    if (fixedHeader.flag != 0x02) {
                        printf("[Error]: Illegal flag in process of processing UNSUBSCRIBE.\n");
                        break;
                    }
                    printf("[INFO]: get a UNSUBSCRIBE!\n");
                    // 获取 UNSUBSCRIBE 的可变报头
                    CommonVariableHeader commonVariableHeader;
                    getCommonVariableHeader(pMessage + byteNum, &commonVariableHeader);
                    byteNum += 2;
                    // 获取 UNSUBSCRIBE 的有效载荷
                    UnsubscribePayload unsubscribePayload;
                    getUnsubscribePayload(pMessage + byteNum, &unsubscribePayload, fixedHeader.remainLength - 2);
                    // 主题过滤器输出
                    for (int j = 0; j < unsubscribePayload.topicFilterNum; ++j) {
                        printf("[INFO]: topicFilter is ");
                        for (int i = 0; i < unsubscribePayload.topicFilterLengthArray[j]; ++i) {
                            printf("%c", unsubscribePayload.pTopicFilterArray[j][i]);
                        }
                        printf(" , about clientID(%d) in process of processing UNSUBSCRIBE.\n",
                               unsubscribePayload.clientId);
                    }
                    // 获取客户端对应的会话状态，并更新会话状态中的上传通信时间
                    Session *pSession = findSession(unsubscribePayload.clientId, broker.pSessionHead);
                    if (pSession != NULL && isInline(pSession)) {
                        // 时间戳相关，更新 lastReqTime
                        pSession->lastReqTime = getCurrentTime();
                        // 维护订阅等信息
                        for (int i = 0; i < unsubscribePayload.topicFilterNum; ++i) {
                            deleteSubscribeTreeNodeByTopicFilter(broker.pSubscribeTreeRoot,
                                                                 unsubscribePayload.pTopicFilterArray[i],
                                                                 unsubscribePayload.topicFilterLengthArray[i],
                                                                 pSession);
                        }
                        // 响应客户端
                        responseUnsubscribe(unsubscribePayload.clientId, commonVariableHeader.messageId);
                    } else {
                        printf("[Error]: Invalid clientID (or dead client) in process of processing UNSUBSCRIBE.\n");
                    }
                    // 善后处理
                    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
                    break;
                }
                case PINGREQ: {
                    printf("[INFO]: Get a PINGREQ.\n");
                    // 获取客户端标识符
                    int clientId = 0;
                    charArray2Int(pMessage + byteNum, &clientId);
                    // 获取该客户端对应的会话状态
                    Session *pSession = findSession(clientId, broker.pSessionHead);
                    if (pSession != NULL && isInline(pSession)) {
                        // 获取当前的时间戳，更新 lastReqTime
                        pSession->lastReqTime = getCurrentTime();
                        // 响应客户端
                        responsePingreq(clientId);
                    } else {
                        printf("[Error]: Invalid clientID (or dead client) in process of processing PINGREQ.\n");
                    }
                    // 善后处理
                    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
                    break;
                }
                case DISCONNECT: {
                    printf("[INFO]: Get a DISCONNECT.\n");
                    // 获取客户端标识符
                    int clientId = 0;
                    charArray2Int(pMessage + byteNum, &clientId);
                    // 获取该客户端对应的会话状态
                    Session *pSession = findSession(clientId, broker.pSessionHead);
                    if (pSession != NULL && isInline(pSession)) {
                        if (pSession->cleanSession) {
                            // 删除会话状态
                            deleteSession(broker.pSubscribeTreeRoot, &broker.pSessionHead, pSession);
                            printf("[INFO]: delete a related session.\n");
                        } else {
                            // 保留会话状态，但是要设为离线状态
                            pSession->isOnline = False;
                            // 时间戳相关
                            pSession->lastReqTime = getCurrentTime();
                            printf("[INFO]: keep a related session.\n");
                        }
                    } else {
                        printf("[Error]: Invalid clientID (or dead client) in process of processing DISCONNECT.\n");
                    }
                    // 善后处理
                    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
                    break;
                }
                default:
                    printf("[Error]: the type of Control Message!\n");
                    break;
            }
        }
    }
}

/**
 * 返回 Broker 的标识符
 */
int getBrokerId() {
    return (int) broker.msgId;
}

/**
* 停止 Broker
*/
void stopBroker() {
    printf("Broker stop success!\n");
    deleteAllSession(broker.pSessionHead);
    deleteSubscribeTree(broker.pSubscribeTreeRoot);
    msgQDelete(broker.msgId);
}

/******************************************************************
 *                       响应客户端的相关代码
 ******************************************************************/

/**
 * 响应 CONNECT
 * @param sp 只有服务端已经保存了会话状态，才置为 1
 * @param returnCode 保留（先假设，所有连接都合法，为以后扩展使用）
 */
void responseConnect(int clientId, UINT8 sp, UINT8 returnCode) {
    int packageLength = 4;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = CONNACK;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 2;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 封包：可变报头
    *(pMessage + byteNum++) = (char) sp;
    *(pMessage + byteNum++) = (char) returnCode;
    // 发送 CONNACK
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send CONNACK Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send CONNACK success!\n");
    }
    // 不管发送成功与否都要释放内存
    memory_management_free((void **) &pMessage, packageLength);
}

/**
 * 响应 subscribe
 * @param ClientId         客户端标识符
 * @param messageId        报文标识符
 * @param returnCodeArray 返回码数组
 * @param topicFilterNum  主题过滤器的数量
 */
void responseSubscribe(int clientId, unsigned int messageId, const char *returnCodeArray, int topicFilterNum) {
    int packageLength = 1 + getEncodeLength(2 + topicFilterNum) + 2 + topicFilterNum;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = SUBACK;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 2 + topicFilterNum;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 封包：可变报头
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);
    // 封包：有效载荷
    for (int i = 0; i < topicFilterNum; ++i) {
        *(pMessage + byteNum + i) = returnCodeArray[i];
    }
    byteNum += topicFilterNum;
    // 发送 SUBACK
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send SUBACK Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send SUBACK success!\n");
    }
    // 不管发送成功与否都要释放内存
    memory_management_free((void **) &pMessage, packageLength);
}

/**
 * 响应 unsubscribe
 * @param clientId  客户端标识符
 * @param messageId 报文标识符
 */
void responseUnsubscribe(int clientId, unsigned int messageId) {
    int packageLength = 4;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = UNSUBACK;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 2;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 封包：可变报头
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);
    // 发送 UNSUBACK
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send UNSUBACK Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send UNSUBACK success!\n");
    }
    // 不管发送成功与否都要释放内存
    memory_management_free((void **) &pMessage, packageLength);
}

/**
 * 响应 心跳
 * @param clientId 客户端标识符
 */
void responsePingreq(int clientId) {
    int packageLength = 2;
    FixedHeader fixedHeader;
    fixedHeader.messageType = PINGRESP;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 0;
    // 发送 UNSUBACK
    int errCode = msgQSend((msg_q *) clientId, (char *) &fixedHeader, packageLength, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send PINGRESP Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send PINGRESP success!\n");
    }
}

/**
 * 发送对 qos1 发布的确认
 * @param clientId
 * @param messageId
 */
void sendPuback(int clientId, int messageId) {
    int packageLength = 4;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBACK;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 2;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 封包：可变报头
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);
    // 发送 PUBACK
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send PUBACK Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send PUBACK success!\n");
    }
    // 不管发送成功与否都要释放内存
    memory_management_free((void **) &pMessage, packageLength);
}

/**
 * 发送 Pubrec
 * @param clientId
 * @param messageId
 */
void sendPubrec(int clientId, int messageId, int msgId) {
    int packageLength = 8;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBREC;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 6;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 封包：可变报头
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);
    // 额外：broker 标识符（新的消息队列）
    int2CharArray(msgId, pMessage + byteNum);
    byteNum += 4;
    // 发送 PUBREC
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send PUBREC Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send PUBREC success!\n");
    }
    // 不管发送成功与否都要释放内存
    memory_management_free((void **) &pMessage, packageLength);
}

/**
 * 发送对 qos2 发布的确认
 * @param clientId
 * @param messageId
 */
void sendPubrel(int clientId, int messageId) {
    int packageLength = 4;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBREL;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 2;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 封包：可变报头
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);
    // 发送 PUBREL
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send PUBREL Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send PUBREL success!\n");
    }
    // 不管发送成功与否都要释放内存
    memory_management_free((void **) &pMessage, packageLength);
}

/**
 * 发送 pubcomp
 */
void sendPubcomp(int clientId, int messageId) {
    int packageLength = 4;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBCOMP;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 2;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 封包：可变报头
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);
    // 发送 PUBCOMP
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send PUBCOMP Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send PUBCOMP success!\n");
    }
    // 不管发送成功与否都要释放内存
    memory_management_free((void **) &pMessage, packageLength);
}

/**
 * 发布 qos = 0
 * @param clientId
 * @param pTopic
 * @param topicLength
 * @param pContent
 * @param contentLength
 * @param key
 * @param pPublishMessage
 */
void sendPublishQos0(int clientId, char *pTopic, int topicLength, char *pContent, int contentLength, int key,
                     char *pPublishMessage) {
    int packageLength = 1 + getEncodeLength(contentLength + topicLength + 2) + contentLength + topicLength + 2;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBLISH;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = contentLength + topicLength + 2;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 封包：可变报头
    byteNum += packageCommonVariableHeader(pMessage + byteNum, topicLength);
    memcpy(pMessage + byteNum, pTopic, topicLength);
    byteNum += topicLength;
    // 封包：有效载荷
    memcpy(pMessage + byteNum, pContent, contentLength);
    byteNum += contentLength;
    // 发送 PUBLISH
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send PUBLISH qos0 fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send PUBLISH qos0 success!\n");
    }
    // 不管发送成功与否都要释放内存
    memory_management_free((void **) &pMessage, packageLength);
    // 视情况释放报文所占的内存
    // 加锁
    semTake(semCMHashTable, WAIT_FOREVER);
    int lenClientHashTable = hash_table_get(pCMHashTable, key);
    if (lenClientHashTable == 0) {
        memory_management_free((void **) &pPublishMessage, MAX_MESSAGE_LENGTH);
        hash_table_rm(pCMHashTable, key);
    } else {
        hash_table_put(pCMHashTable, key, lenClientHashTable - 1);
    }
    // 释放
    semGive(semCMHashTable);
}

/**
 * 发布 qos1
 * @param pSessionValue
 * @param pTopicValue
 * @param topicLength
 * @param pContentValue
 * @param contentLength
 * @param messageId
 */
void pubQos1TaskFunction(int pSessionValue, int pTopicValue, int topicLength, int pContentValue, int contentLength,
                         int messageId, int key, int pPublishMessageValue) {
    // 0）准备
    Session *pSession = (Session *) pSessionValue;
    char *pTopic = (char *) pTopicValue;
    char *pContent = (char *) pContentValue;
    char *pPublishMessage = (char *) pPublishMessageValue;
    MSG_Q_ID msgId = msgQCreate(PUB_MAX_MESSAGE_NUM, MAX_MESSAGE_LENGTH, MSG_Q_PRIORITY);
    char messages[MAX_MESSAGE_LENGTH];  /* 存放接受到的消息 */
    // 0.1）时间间隔
    UINT16 intervalTime = 0;
    if (pSession->pingTime == 0) {
        intervalTime = PING_TIME / 2;
    } else {
        intervalTime = pSession->pingTime / 2;
    }
    // 0.2）定时器相关
    int timerId;
    if (eGetTimer(&timerId, TIMER_PERIOD, taskIdSelf(), 1, 0, msgId, intervalTime)) {
        printf("[Error]: get timer fail in the process of processing qos1 publish message(%d)!\n", messageId);
        return;
    }
    // 1）逻辑
    int packageLength = 1 + getEncodeLength(contentLength + topicLength + 8) + contentLength + topicLength + 8;
    char *pMessage;
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBLISH;
    fixedHeader.flag = 0x02;
    fixedHeader.remainLength = contentLength + topicLength + 8;
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);   /* 封包：固定报头 */
    byteNum += packageCommonVariableHeader(pMessage + byteNum, topicLength);    /* 封包：可变报头 */
    memcpy(pMessage + byteNum, pTopic, topicLength);
    byteNum += topicLength;
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);
    int2CharArray((int) msgId, pMessage + byteNum); /* 封包：Broker标识符（新的消息队列ID） */
    byteNum += 4;
    memcpy(pMessage + byteNum, pContent, contentLength);    /* 封包：有效载荷 */
    // 1.1）发送 PUBLISH，并等待 PUBACK
    if (isInline(pSession)){
        int errCode = msgQSend((msg_q *)(pSession->clientId), pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);
        if (errCode) {
            printf("[Error]: send PUBLISH qos1 message(%d) fail! [ERR_CODE]: %d\n", messageId, errCode);
        } else {
            printf("[Success]: send PUBLISH qos1 message(%d) success!\n", messageId);
        }
        if (eStartTimer(timerId, 2019, intervalTime)) { /* 开启定时器 */
            printf("[Error]: start timer fail in the process of processing qos2 publish message(%d)!\n", messageId);
            return;
        }
    }
    boolean isACK = False;
    int tCount = 0;
    while (tCount < 3 && isInline(pSession)){
        int msgLen = msgQReceive(msgId, messages, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
        if (msgLen > 0) {
            if (msgLen == 20 && messages[0] == MT_TIMER) {  /* 此处有小概率出现判断错误，进入表明消息为“定时器”返回的 */
                int errCode = msgQSend((msg_q *)(pSession->clientId), pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);  /* 发 PUBLISH */
                if (errCode) {
                    printf("[Error]: send PUBLISH qos1 message(%d) fail! [ERR_CODE]: %d\n", messageId, errCode);
                } else {
                    printf("[Success]: send PUBLISH qos1 message(%d) success!\n", messageId);
                }
                ++tCount;
            } else{  /* broker 返回的消息 */
                byteNum = getFixedHeader(messages, &fixedHeader);
                if (fixedHeader.messageType == PUBACK) {
                    CommonVariableHeader commonVariableHeader;
                    getCommonVariableHeader(messages + byteNum, &commonVariableHeader);
                    if (messageId == commonVariableHeader.messageId) {
                        pSession->lastReqTime = getCurrentTime();   /* 更新上次通信时间 */
                        isACK = True;
                        eStopTimer(timerId);    /* 停止计时器 */
                        break;
                    }
                }
            }
        } else{
            printf("[Error]: message len in the process of publishing qos2 message(%d)!\n", messageId);
        }
    }
    // 1.2）输出结果
    if (isACK) {
        printf("[Success]: ack qos1 publish message(%d)!\n", messageId);
    } else {
        pSession->isOnline = False;
        printf("[Error]: unack %d qos1 publish message(%d)!\n", messageId);
    }
    // 2）善后
    eKillTimer(timerId);    /* 释放申请的定时器 */
    msgQDelete(msgId);  /* 释放两个消息队列 */
    memory_management_free((void **) &pMessage, packageLength); /* 不管发送成功与否都要释放内存 */
    // 加锁（视情况释放报文所占的内存）
    semTake(semCMHashTable, WAIT_FOREVER);
    int lenClientHashTable = hash_table_get(pCMHashTable, key);
    if (lenClientHashTable == 0) {
        memory_management_free((void **) &pPublishMessage, MAX_MESSAGE_LENGTH);
        hash_table_rm(pCMHashTable, key);
    } else {
        hash_table_put(pCMHashTable, key, lenClientHashTable - 1);
    }
    // 释放
    semGive(semCMHashTable);
}

/**
 * 发送 qos = 1 的 publish 报文
 * 注：需要开启新的 task ，使用新的 消息队列，在 PUBACK 的可变报头之后，有效载荷之前包含该消息队列的ID（可以认为这是 broker 的标识）
 * 注：这里所含 新的消息队列ID 与 报文标识符 的作用等价
 * @param pSession
 * @param pTopic
 * @param topicLength
 * @param pContent
 * @param contentLength
 * @param messageId
 */
void
sendPublishQos1(Session *pSession, char *pTopic, int topicLength, char *pContent, int contentLength, int messageId,
                int key, char *pPublishMessage) {
    taskSpawn("pubQos1Task", PUB_TASK_PRIO, 0, PUB_TASK_STACK,
              (FUNCPTR) pubQos1TaskFunction,
              (int) pSession, (int) pTopic, topicLength, (int) pContent, contentLength,
              messageId, key, (int) pPublishMessage,
              0, 0);
}

/**
 * 发布 qos2
 * @param pSessionValue
 * @param pTopicValue
 * @param topicLength
 * @param pContentValue
 * @param contentLength
 * @param messageId
 */
void pubQos2TaskFunction(int pSessionValue, int pTopicValue, int topicLength, int pContentValue, int contentLength,
                         int messageId, int key, int pPublishMessageValue) {
    // 0）准备
    Session *pSession = (Session *) pSessionValue;
    char *pTopic = (char *) pTopicValue;
    char *pContent = (char *) pContentValue;
    char *pPublishMessage = (char *) pPublishMessageValue;
    MSG_Q_ID msgId = msgQCreate(PUB_MAX_MESSAGE_NUM, MAX_MESSAGE_LENGTH, MSG_Q_PRIORITY);   /* 接受消息的队列 */
    char messages[MAX_MESSAGE_LENGTH];  /* 存放接受到的消息 */
    // 0.1）间隔时间
    UINT16 intervalTime = 0;
    if (pSession->pingTime == 0) {
        intervalTime = PING_TIME / 2;
    } else {
        intervalTime = pSession->pingTime / 2;
    }
    // 0.2）定时器相关
    int timerId;
    if (eGetTimer(&timerId, TIMER_PERIOD, taskIdSelf(), 1, 0, msgId, intervalTime)) {  /* 获取一个定时器 */
        printf("[Error]: get timer fail in the process of processing qos2 publish message(%d)!\n", messageId);
        return;
    }
    // 1）逻辑
    // 1.1）封包 PUBLISH
    int packageLength = 1 + getEncodeLength(contentLength + topicLength + 8) + contentLength + topicLength + 8;
    char *pMessage; /* PUBLISH消息头指针 */
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBLISH;
    fixedHeader.flag = 0x04;
    fixedHeader.remainLength = contentLength + topicLength + 8;
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);   /* 封包：固定报头 */
    byteNum += packageCommonVariableHeader(pMessage + byteNum, topicLength);    /* 封包：主题 */
    memcpy(pMessage + byteNum, pTopic, topicLength);
    byteNum += topicLength;
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);  /* 封包：消息标识符 */
    int2CharArray((int) msgId, pMessage + byteNum); /* 封包：Broker标识符（新的消息队列ID） */
    byteNum += 4;
    memcpy(pMessage + byteNum, pContent, contentLength);    /* 封包：有效载荷 */
    // 1.2）发送 PUBLISH
    if (isInline(pSession)){    /* 判断客户端是否在线 */
        int errCode = msgQSend((msg_q *)(pSession->clientId), pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);  /* 发 PUBLISH */
        if (errCode) {
            printf("[Error]: send PUBLISH qos2 message(%d) fail! [ERR_CODE]: %d\n", messageId, errCode);
        } else {
            printf("[Success]: send PUBLISH qos2 message(%d) success!\n", messageId);
        }
        if (eStartTimer(timerId, 2019, intervalTime)) { /* 开启定时器 */
            printf("[Error]: start timer fail in the process of processing qos2 publish message(%d)!\n", messageId);
            return;
        }
    }
    boolean isNeedSendPubrel = False;   /* 记录是否发送 PUBLISH */
    int tCount = 0;
    while (tCount < 3 && isInline(pSession)){
        int msgLen = msgQReceive(msgId, messages, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
        if (msgLen > 0) {
            if (msgLen == 20 && messages[0] == MT_TIMER) {  /* 此处有小概率出现判断错误，进入表明消息为“定时器”返回的 */
                int errCode = msgQSend((msg_q *)(pSession->clientId), pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);  /* 发 PUBLISH */
                if (errCode) {
                    printf("[Error]: send PUBLISH qos2 message(%d) fail! [ERR_CODE]: %d\n", messageId, errCode);
                } else {
                    printf("[Success]: send PUBLISH qos2 message(%d)  success!\n", messageId);
                }
                ++tCount;
            } else{  /* broker 返回的消息 */
                byteNum = getFixedHeader(messages, &fixedHeader);
                if (fixedHeader.messageType == PUBREC) {
                    CommonVariableHeader commonVariableHeader;
                    getCommonVariableHeader(messages + byteNum, &commonVariableHeader);
                    if (messageId == commonVariableHeader.messageId) {
                        pSession->lastReqTime = getCurrentTime();   /* 更新上次通信时间 */
                        isNeedSendPubrel = True;
                        eStopTimer(timerId);    /* 停止计时器 */
                        while (msgQNumMsgs(msgId) > 0){ /* 清空消息队列 */
                            msgQReceive(msgId, messages, 0, NO_WAIT);
                        }
                        break;
                    }
                }
            }
        } else{
            printf("[Error]: message len in the process of publishing qos2 message(%d)!\n", messageId);
        }
    }
    // 1.3）发送 PUBREL
    boolean isComp = False; /* 记录是否需要发送 PUBCOMP */
    tCount = 0;
    if (isNeedSendPubrel && isInline(pSession)) {
        sendPubrel(pSession->clientId, messageId);
        if (eStartTimer(timerId, 2019, intervalTime)) { /* 开启定时器 */
            printf("[Error]: start timer fail in the process of publishing qos2 message(%d)!\n", messageId);
            return;
        }
        while (tCount < 3 && isInline(pSession)) {
            int msgLen = msgQReceive(msgId, messages, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
            if (msgLen > 0) {
                if (msgLen == 20 && messages[0] == MT_TIMER) {  /* 此处有小概率出现判断错误，进入表明消息为“定时器”返回的 */
                    sendPubrel(pSession->clientId, messageId);
                    ++tCount;
                } else {  /* broker 返回的消息 */
                    byteNum = getFixedHeader(messages, &fixedHeader);
                    if (fixedHeader.messageType == PUBCOMP) {
                        CommonVariableHeader commonVariableHeader;
                        getCommonVariableHeader(messages + byteNum, &commonVariableHeader);
                        if (messageId == commonVariableHeader.messageId) {
                            pSession->lastReqTime = getCurrentTime();   /* 更新上次通信时间 */
                            isComp = True;
                            eStopTimer(timerId);    /* 停止计时器 */
                            break;
                        }
                    }
                }
            } else {
                printf("[Error]: invalid message len in the process of publishing qos2 message(%d)!\n", messageId);
            }
        }
    }
    // 1.4）输出结果
    if (isComp) {
        printf("[Success]: ack qos2 publish message(%d)!\n", messageId);
    } else {
        pSession->isOnline = False;
        printf("[Error]: unack qos2 publish message(%d)!\n", messageId);
    }
    // 2）善后
    eKillTimer(timerId);    /* 释放申请的定时器 */
    msgQDelete(msgId);  /* 释放消息队列 */
    memory_management_free((void **) &pMessage, packageLength);
    // 加锁（视情况释放报文所占的内存）
    semTake(semCMHashTable, WAIT_FOREVER);
    int lenClientHashTable = hash_table_get(pCMHashTable, key);
    if (lenClientHashTable == 0) {
        memory_management_free((void **) &pPublishMessage, MAX_MESSAGE_LENGTH);
        hash_table_rm(pCMHashTable, key);
    } else {
        hash_table_put(pCMHashTable, key, lenClientHashTable - 1);
    }
    // 释放
    semGive(semCMHashTable);
}

/**
 * 发送 qos = 2 的 publish 报文
 * @param pSession
 * @param pTopic
 * @param topicLength
 * @param pContent
 * @param contentLength
 * @param messageId
 */
void
sendPublishQos2(Session *pSession, char *pTopic, int topicLength, char *pContent, int contentLength, int messageId,
                int key, char *pPublishMessage) {
    taskSpawn("pubQos2Task", PUB_TASK_PRIO, 0, PUB_TASK_STACK,
              (FUNCPTR) pubQos2TaskFunction,
              (int) pSession, (int) pTopic, topicLength, (int) pContent, contentLength,
              messageId, key, (int) pPublishMessage,
              0, 0);
}

/**
 * 响应 qos2 publish 具体实现
 * @param pSessionValue
 * @param messageId
 */
void responsePublishQos2Function(int pSessionValue, int messageId) {
    // 0）准备
    Session *pSession = (Session *) pSessionValue;
    MSG_Q_ID msgId = msgQCreate(PUB_MAX_MESSAGE_NUM, MAX_MESSAGE_LENGTH, MSG_Q_PRIORITY);
    char messages[MAX_MESSAGE_LENGTH];
    // 0.1）时间间隔
    UINT16 intervalTime = 0;
    if (pSession->pingTime == 0) {
        intervalTime = PING_TIME / 2;
    } else {
        intervalTime = pSession->pingTime / 2;
    }
    // 0.2）定时器相关
    int timerId;
    if (eGetTimer(&timerId, TIMER_PERIOD, taskIdSelf(), 1, 0, msgId, intervalTime)) {   /* 获取一个定时器 */
        printf("[Error]: get timer fail in the process of processing response Qos2 publish message(%d)!\n", messageId);
        return;
    }
    // 1）响应逻辑
    boolean isNeedPubcomp = False;  /* 是否需要发送 PUBCOMP */
    int tCount = 0; /* 记录定时完成次数 */
    // 1.1）发送 PUBREC
    if (isInline(pSession)){    /* 判断客户端是否在线 */
        sendPubrec(pSession->clientId, messageId, (int) msgId);
        if (eStartTimer(timerId, 2019, intervalTime)) { /* 开启定时器 */
            printf("[Error]: start timer fail in the process of processing response Qos2 publish message(%d)!\n", messageId);
            return;
        }
    }
    while (tCount < 3 && isInline(pSession)) {
        int msgLen = msgQReceive(msgId, messages, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
        if (msgLen > 0) {
            if (msgLen == 20 && messages[0] == MT_TIMER) {  /* 此处有小概率出现判断错误，进入表明消息为“定时器”返回的 */
                sendPubrec(pSession->clientId, messageId, (int) msgId);
                ++tCount;
            } else{ /* broker 返回的消息 */
                FixedHeader fixedHeader;
                int byteNum = getFixedHeader(messages, &fixedHeader);
                if (fixedHeader.messageType == PUBREL) {
                    CommonVariableHeader commonVariableHeader;
                    getCommonVariableHeader(messages + byteNum, &commonVariableHeader);
                    if (messageId == commonVariableHeader.messageId) {
                        pSession->lastReqTime = getCurrentTime();   /* 更新上次通信时间 */
                        isNeedPubcomp = True;
                        eStopTimer(timerId);    /* 停止计时器 */
                        break;
                    }
                }
            }
        } else {
            printf("[Error]: message len in the process of processing qos2 message(%d) pubrel!\n", messageId);
        }
    }
    // 1.2）发送 PUBCOMP
    if (isNeedPubcomp) {
        sendPubcomp(pSession->clientId, messageId); /* 发送 pubcomp */
        // 加锁（用于从 Hashtable 删除）
        semTake(pSession->semM, WAIT_FOREVER);
        hash_table_rm(pSession->pMessageIdHashTable, messageId);
        // 释放
        semGive(pSession->semM);
    }
    // 2）善后处理
    eKillTimer(timerId);    /* 释放申请的定时器 */
    msgQDelete(msgId);      /* 释放消息队列 */
}

/**
 * 响应 qos2 publish
 * @param pSession
 * @param messageId
 */
void responsePublishQos2(Session *pSession, int messageId) {
    taskSpawn("responseQos2Task", PUB_TASK_PRIO, 0, PUB_TASK_STACK,
              (FUNCPTR) responsePublishQos2Function,
              (int) pSession, messageId,
              0, 0, 0, 0, 0, 0, 0, 0);
}
