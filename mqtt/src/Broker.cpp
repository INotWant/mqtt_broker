#include "Broker.h"

Broker broker;
HashTable *pClientHashTable;              // ���Ҫ�ַ��������пͻ���
HashTable *pCMHashTable;                                    // key: clientId �� messageId ��ƴ�ӣ�ȡclientId�ĵ�16λ���²����ĸ�16λ��messageId�ĵ�16λ���²����ĵ�16λ��
// value: δ��ɵķַ���
// ������ȫ���ַ���ɺ��ͷ� message ռ�õĶ�̬������ڴ�
SEM_ID semCMHashTable;                                        // ��	pCMHashTable �Ļ������

// TODO 1) ��������Ҫ�����еı��Ķ�Ҫ�ڿɱ䱨ͷ֮����Ч�غ�֮ǰ���� clientId ���� mqtt ��׼��
// TODO 2) ���ǵ�������ѯ Session ����O(n)�����Ƿ���Ҫ�������Ϊ Hash ��
// TODO 3) ���� HashTable �����Ƿ���԰�����������дһ��ͨ�õ�����
// TODO 4) Ϊ�ƶ�ͳһ��Ϣ������

/******************************************************************
 *                       һЩ�ڲ���������
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
 * Broker ��ʼ��
 */
int initBroker() {
    // ��ʼ���ڴ����
    int fragments_num = 3;
    int fragments_size[] = {16, 128, 512};
    int fragments_count[] = {32, 16, 8};
    int errorValue = memory_management_init(fragments_num, fragments_size, fragments_count);
    if (errorValue) {
        return errorValue;
    }
    // ��ʼ����ʱ��
    eTimerManagementInit();
    // ��ʼ�� Broker �ڲ�
    broker.msgId = msgQCreate(MAX_MESSAGE_NUM, MAX_MESSAGE_LENGTH, MSG_Q_PRIORITY);
    // ��ȫ��ʼ��
    broker.pSessionHead = NULL;
    broker.pRetainMessageHead = NULL;
    // Ϊ���������� # �� / ���
    char currentTopicName = '#';
    broker.pSubscribeTreeRoot = createSubscribeTreeNode(&currentTopicName, 1);
    currentTopicName = '+';
    broker.pSubscribeTreeRoot->pBrother = createSubscribeTreeNode(&currentTopicName, 1);
    // ��ʼ�� ��ϣ��
    pClientHashTable = hash_table_new();
    pCMHashTable = hash_table_new();
    semCMHashTable = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
    printf("[INFO]:Broker init success!\n");
    return BROKER_ERR_SUCCESS;
}

/**
 * ���� Broker
 */
Broker *getBroker() {
    return &broker;
}

/**
 * ���� Broker
 */
void startBroker() {
    printf("[INFO]:Broker start!\n");
    while (True) {
        // ʹ�ö�̬�ڴ汣�� ���ģ��ں����������һ��Ҫ�ڴ�����ñ���ʱ�Ѹ��ڴ��ͷ�
        char *pMessage;
        memory_management_allocate(MAX_MESSAGE_LENGTH, (void **) &pMessage);
        int mLen = msgQReceive(broker.msgId, (char *) pMessage, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
        if (mLen <= 0) {
            printf("[Error]: message len!\n");
            memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
            continue;
        } else {
            // ���
            FixedHeader fixedHeader;
            // 1����ȡ�̶���ͷ
            int byteNum = getFixedHeader(pMessage, &fixedHeader);
            // 2�����ݿ��Ʊ�����������Ӧ����
            switch (fixedHeader.messageType) {
                case CONNECT: {
                    // ��ȡ CONNECT �Ŀɱ䱨ͷ
                    ConnVariableHeader connVariableHeader;
                    byteNum += getConnVariableHeader(pMessage + byteNum, &connVariableHeader);
                    // ��ȡ CONNECT ����Ч�غ�
                    ConnPayload connPayload;
                    getConnPayload(pMessage + byteNum, &connPayload);
                    // ���ҸûỰ֮ǰ�Ƿ����
                    printf("[INFO]: Get a connect, clientId is %d.\n", connPayload.clientId);
                    Session *pSession = findSession(connPayload.clientId, broker.pSessionHead);
                    boolean spFlag = pSession == NULL ? False : True;
                    if (connVariableHeader.cleanSession) {
                        // Ҫ����Ự״̬�����֮ǰ�ж�Ӧ�ĻỰ״̬������Ҫɾ����
                        if (pSession != NULL) {
                            deleteSession(broker.pSubscribeTreeRoot, &broker.pSessionHead, pSession);
                        }
                        printf("[INFO]keepAlive:\t%d\n", connVariableHeader.keepAlive);
                        // �����µĻỰ�������´����Ĳ��뵽�Ự״̬������
                        // �����µĻỰ���Ѿ��� lastReqTime �������ã���������������
                        pSession = createSession(connPayload.clientId, connVariableHeader.cleanSession,
                                                 connVariableHeader.keepAlive);
                        insertSession(pSession, &broker.pSessionHead);
                        printf("[INFO]: Create a session for clientID(%d).\n", connPayload.clientId);
                    } else {
                        // �����ûỰ״̬
                        if (pSession != NULL) {
                            // ��ȡ��ǰʱ������޸��ϴ�����ʱ�䣬ʹ�� �ͻ��˱��
                            modifySessionLastReqTime(getCurrentTime(), pSession);
                            printf("[INFO]: Use a old session for clientID(%d).\n", connPayload.clientId);
                        } else {
                            // �����µĻỰ�������´����Ĳ��뵽�Ự״̬������
                            pSession = createSession(connPayload.clientId, connVariableHeader.cleanSession,
                                                     connVariableHeader.keepAlive);
                            insertSession(pSession, &broker.pSessionHead);
                            printf("[INFO]: Create a session for clientID(%d).\n", connPayload.clientId);
                        }
                    }
                    // ��Ӧ�ͻ���
                    responseConnect(connPayload.clientId, spFlag, 0x00);
                    // �ƺ���
                    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
                    break;
                }
                case PUBLISH: {
                    // ��ȡ publish �й̶���ͷ�е�һЩ��־
                    boolean dupFlag = getCharSpecialBit(&fixedHeader.flag, 3);
                    UINT8 qos = getCharSpecialBit(&fixedHeader.flag, 2) * 2;
                    qos += getCharSpecialBit(&fixedHeader.flag, 1);
                    boolean retainFlag = getCharSpecialBit(&fixedHeader.flag, 0);
                    // ��ȡ publish �еĿɱ䱨ͷ
                    PublishVariableHeader publishVariableHeader;
                    int variableHeaderLength = getPublishVariableHeader(pMessage + byteNum, &publishVariableHeader,
                                                                        qos);
                    byteNum += variableHeaderLength;
                    // ��ȡ publish �е���Ч�غ�
                    PublishPayload publishPayload;
                    getPublishPayload(pMessage + byteNum, &publishPayload, fixedHeader.remainLength,
                                      variableHeaderLength);
                    // ��ȡ�ͻ��˶�Ӧ�ĻỰ״̬�������»Ự״̬�е��ϴ�ͨ��ʱ��
                    Session *pSession = findSession(publishPayload.clientId, broker.pSessionHead);
                    if (pSession != NULL && isInline(pSession)) {
                        // ��ȡʱ��������� lastReqTime
                        pSession->lastReqTime = getCurrentTime();
                        // 0����ʾ���յ��� publish
                        printf("[INFO]: receive a public, topic is ");
                        for (int i = 0; i < publishVariableHeader.topicLength; ++i) {
                            printf("%c", publishVariableHeader.pTopicHead[i]);
                        }
                        printf(", qos is %d\n", qos);
                        // 1����Ӧ�߼�
                        if (qos == 1) {
                            sendPuback(publishPayload.clientId, publishVariableHeader.messageId);
                        } else if (qos == 2) {
                            // ����
                            semTake(pSession->semM, WAIT_FOREVER);
                            if (hash_table_get(pSession->pMessageIdHashTable, publishVariableHeader.messageId) !=
                                NULL) {
                                break;
                            }
                            hash_table_put(pSession->pMessageIdHashTable, publishVariableHeader.messageId, 1);
                            // �ͷ�
                            semGive(pSession->semM);
                            // ʹ���µ� task ȥ��Ӧ
                            responsePublishQos2(pSession, publishVariableHeader.messageId);
                        }
                        // 2���ַ��߼�
                        // ��ȡ���еȴ��ַ��Ŀͻ���
                        findAllClientInSubscribeTree(broker.pSubscribeTreeRoot, publishVariableHeader.pTopicHead,
                                                     publishVariableHeader.topicLength, pClientHashTable);
                        // ���� pClientHashTable ���зַ���ע��Ӧ�ѱ���д�� hashTable.h �У����� ���� ����
                        // ���ű������������ pClientHashTable
                        int lenClientHashTable = hash_table_len(pClientHashTable);
                        int key = (publishPayload.clientId << 16) + (publishVariableHeader.messageId & 0xFFFF);
                        // ����
                        semTake(semCMHashTable, WAIT_FOREVER);
                        hash_table_put(pCMHashTable, key, lenClientHashTable);
                        // �ͷ�
                        semGive(semCMHashTable);
                        for (int i = 0; i < TABLE_SIZE; ++i) {
                            struct kv *p = pClientHashTable->table[i];
                            struct kv *q = NULL;
                            while (p) {
                                q = p->next;
                                // ��ȡ�ַ��� qos
                                qos = p->value <= qos ? p->value : qos;
                                Session *pASession = (Session *) p->key;
                                if (isInline(pASession)) {
                                    if (qos == 0) {
                                        // ֱ�ӷ���
                                        sendPublishQos0(pASession->clientId, publishVariableHeader.pTopicHead,
                                                        publishVariableHeader.topicLength, publishPayload.pContent,
                                                        publishPayload.contentLength, key, pMessage);
                                    } else if (qos == 1) {
                                        // ʹ���µ� task ȥ����
                                        sendPublishQos1(pASession, publishVariableHeader.pTopicHead,
                                                        publishVariableHeader.topicLength, publishPayload.pContent,
                                                        publishPayload.contentLength, publishVariableHeader.messageId,
                                                        key, pMessage);
                                    } else if (qos == 2) {
                                        // ʹ���µ� task ȥ����
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
                    // ���ƹ̶���ͷ�ı���λ����Ϊ 0x02
                    if (fixedHeader.flag != 0x02) {
                        printf("[Error]: Illegal flag in process of processing SUBSCRIBE.\n");
                        break;
                    }
                    printf("[INFO]: get a SUBSCRIBE!\n");
                    // ��ȡ SUBSCRIBE �Ŀɱ䱨ͷ
                    CommonVariableHeader commonVariableHeader;
                    getCommonVariableHeader(pMessage + byteNum, &commonVariableHeader);
                    byteNum += 2;
                    // ��ȡ SUBSCRIBE ����Ч�غ�
                    SubscribePayload subscribePayload;
                    getSubscribePayload(pMessage + byteNum, &subscribePayload, fixedHeader.remainLength - 2);
                    // ��������� & QOS ���
                    for (int j = 0; j < subscribePayload.topicFilterNum; ++j) {
                        printf("[INFO]: topicFilter is ");
                        for (int i = 0; i < subscribePayload.topicFilterLengthArray[j]; ++i) {
                            printf("%c", subscribePayload.pTopicFilterArray[j][i]);
                        }
                        printf(" ,qos is %d about clientId(%d) in process of processing SUBSCRIBE.\n",
                               subscribePayload.topicFilterQosArray[j], subscribePayload.clientId);
                    }
                    // ��ȡ�ͻ��˶�Ӧ�ĻỰ״̬�������»Ự״̬�е��ϴ�ͨ��ʱ��
                    Session *pSession = findSession(subscribePayload.clientId, broker.pSessionHead);
                    if (pSession != NULL && isInline(pSession)) {
                        // ��ȡʱ��������� lastReqTime
                        pSession->lastReqTime = getCurrentTime();
                        // ά�����ĵ���Ϣ
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
                        // ��Ӧ�ͻ���
                        responseSubscribe(subscribePayload.clientId, commonVariableHeader.messageId, returnCodeArray,
                                          subscribePayload.topicFilterNum);
                    } else {
                        printf("[Error]: Invalid clientID (or dead client) in process of processing SUBSCRIBE.\n");
                    }
                    // �ƺ���
                    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
                    break;
                }
                case UNSUBSCRIBE: {
                    // ���ƹ̶���ͷ�ı���λ����Ϊ 0x02
                    if (fixedHeader.flag != 0x02) {
                        printf("[Error]: Illegal flag in process of processing UNSUBSCRIBE.\n");
                        break;
                    }
                    printf("[INFO]: get a UNSUBSCRIBE!\n");
                    // ��ȡ UNSUBSCRIBE �Ŀɱ䱨ͷ
                    CommonVariableHeader commonVariableHeader;
                    getCommonVariableHeader(pMessage + byteNum, &commonVariableHeader);
                    byteNum += 2;
                    // ��ȡ UNSUBSCRIBE ����Ч�غ�
                    UnsubscribePayload unsubscribePayload;
                    getUnsubscribePayload(pMessage + byteNum, &unsubscribePayload, fixedHeader.remainLength - 2);
                    // ������������
                    for (int j = 0; j < unsubscribePayload.topicFilterNum; ++j) {
                        printf("[INFO]: topicFilter is ");
                        for (int i = 0; i < unsubscribePayload.topicFilterLengthArray[j]; ++i) {
                            printf("%c", unsubscribePayload.pTopicFilterArray[j][i]);
                        }
                        printf(" , about clientID(%d) in process of processing UNSUBSCRIBE.\n",
                               unsubscribePayload.clientId);
                    }
                    // ��ȡ�ͻ��˶�Ӧ�ĻỰ״̬�������»Ự״̬�е��ϴ�ͨ��ʱ��
                    Session *pSession = findSession(unsubscribePayload.clientId, broker.pSessionHead);
                    if (pSession != NULL && isInline(pSession)) {
                        // ʱ�����أ����� lastReqTime
                        pSession->lastReqTime = getCurrentTime();
                        // ά�����ĵ���Ϣ
                        for (int i = 0; i < unsubscribePayload.topicFilterNum; ++i) {
                            deleteSubscribeTreeNodeByTopicFilter(broker.pSubscribeTreeRoot,
                                                                 unsubscribePayload.pTopicFilterArray[i],
                                                                 unsubscribePayload.topicFilterLengthArray[i],
                                                                 pSession);
                        }
                        // ��Ӧ�ͻ���
                        responseUnsubscribe(unsubscribePayload.clientId, commonVariableHeader.messageId);
                    } else {
                        printf("[Error]: Invalid clientID (or dead client) in process of processing UNSUBSCRIBE.\n");
                    }
                    // �ƺ���
                    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
                    break;
                }
                case PINGREQ: {
                    printf("[INFO]: Get a PINGREQ.\n");
                    // ��ȡ�ͻ��˱�ʶ��
                    int clientId = 0;
                    charArray2Int(pMessage + byteNum, &clientId);
                    // ��ȡ�ÿͻ��˶�Ӧ�ĻỰ״̬
                    Session *pSession = findSession(clientId, broker.pSessionHead);
                    if (pSession != NULL && isInline(pSession)) {
                        // ��ȡ��ǰ��ʱ��������� lastReqTime
                        pSession->lastReqTime = getCurrentTime();
                        // ��Ӧ�ͻ���
                        responsePingreq(clientId);
                    } else {
                        printf("[Error]: Invalid clientID (or dead client) in process of processing PINGREQ.\n");
                    }
                    // �ƺ���
                    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
                    break;
                }
                case DISCONNECT: {
                    printf("[INFO]: Get a DISCONNECT.\n");
                    // ��ȡ�ͻ��˱�ʶ��
                    int clientId = 0;
                    charArray2Int(pMessage + byteNum, &clientId);
                    // ��ȡ�ÿͻ��˶�Ӧ�ĻỰ״̬
                    Session *pSession = findSession(clientId, broker.pSessionHead);
                    if (pSession != NULL && isInline(pSession)) {
                        if (pSession->cleanSession) {
                            // ɾ���Ự״̬
                            deleteSession(broker.pSubscribeTreeRoot, &broker.pSessionHead, pSession);
                            printf("[INFO]: delete a related session.\n");
                        } else {
                            // �����Ự״̬������Ҫ��Ϊ����״̬
                            pSession->isOnline = False;
                            // ʱ������
                            pSession->lastReqTime = getCurrentTime();
                            printf("[INFO]: keep a related session.\n");
                        }
                    } else {
                        printf("[Error]: Invalid clientID (or dead client) in process of processing DISCONNECT.\n");
                    }
                    // �ƺ���
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
 * ���� Broker �ı�ʶ��
 */
int getBrokerId() {
    return (int) broker.msgId;
}

/**
* ֹͣ Broker
*/
void stopBroker() {
    printf("Broker stop success!\n");
    deleteAllSession(broker.pSessionHead);
    deleteSubscribeTree(broker.pSubscribeTreeRoot);
    msgQDelete(broker.msgId);
}

/******************************************************************
 *                       ��Ӧ�ͻ��˵���ش���
 ******************************************************************/

/**
 * ��Ӧ CONNECT
 * @param sp ֻ�з�����Ѿ������˻Ự״̬������Ϊ 1
 * @param returnCode �������ȼ��裬�������Ӷ��Ϸ���Ϊ�Ժ���չʹ�ã�
 */
void responseConnect(int clientId, UINT8 sp, UINT8 returnCode) {
    int packageLength = 4;
    char *pMessage;
    // �����ڴ�
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = CONNACK;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 2;
    // ������̶���ͷ
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // ������ɱ䱨ͷ
    *(pMessage + byteNum++) = (char) sp;
    *(pMessage + byteNum++) = (char) returnCode;
    // ���� CONNACK
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send CONNACK Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send CONNACK success!\n");
    }
    // ���ܷ��ͳɹ����Ҫ�ͷ��ڴ�
    memory_management_free((void **) &pMessage, packageLength);
}

/**
 * ��Ӧ subscribe
 * @param ClientId         �ͻ��˱�ʶ��
 * @param messageId        ���ı�ʶ��
 * @param returnCodeArray ����������
 * @param topicFilterNum  ���������������
 */
void responseSubscribe(int clientId, unsigned int messageId, const char *returnCodeArray, int topicFilterNum) {
    int packageLength = 1 + getEncodeLength(2 + topicFilterNum) + 2 + topicFilterNum;
    char *pMessage;
    // �����ڴ�
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = SUBACK;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 2 + topicFilterNum;
    // ������̶���ͷ
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // ������ɱ䱨ͷ
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);
    // �������Ч�غ�
    for (int i = 0; i < topicFilterNum; ++i) {
        *(pMessage + byteNum + i) = returnCodeArray[i];
    }
    byteNum += topicFilterNum;
    // ���� SUBACK
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send SUBACK Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send SUBACK success!\n");
    }
    // ���ܷ��ͳɹ����Ҫ�ͷ��ڴ�
    memory_management_free((void **) &pMessage, packageLength);
}

/**
 * ��Ӧ unsubscribe
 * @param clientId  �ͻ��˱�ʶ��
 * @param messageId ���ı�ʶ��
 */
void responseUnsubscribe(int clientId, unsigned int messageId) {
    int packageLength = 4;
    char *pMessage;
    // �����ڴ�
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = UNSUBACK;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 2;
    // ������̶���ͷ
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // ������ɱ䱨ͷ
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);
    // ���� UNSUBACK
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send UNSUBACK Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send UNSUBACK success!\n");
    }
    // ���ܷ��ͳɹ����Ҫ�ͷ��ڴ�
    memory_management_free((void **) &pMessage, packageLength);
}

/**
 * ��Ӧ ����
 * @param clientId �ͻ��˱�ʶ��
 */
void responsePingreq(int clientId) {
    int packageLength = 2;
    FixedHeader fixedHeader;
    fixedHeader.messageType = PINGRESP;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 0;
    // ���� UNSUBACK
    int errCode = msgQSend((msg_q *) clientId, (char *) &fixedHeader, packageLength, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send PINGRESP Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send PINGRESP success!\n");
    }
}

/**
 * ���Ͷ� qos1 ������ȷ��
 * @param clientId
 * @param messageId
 */
void sendPuback(int clientId, int messageId) {
    int packageLength = 4;
    char *pMessage;
    // �����ڴ�
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBACK;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 2;
    // ������̶���ͷ
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // ������ɱ䱨ͷ
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);
    // ���� PUBACK
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send PUBACK Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send PUBACK success!\n");
    }
    // ���ܷ��ͳɹ����Ҫ�ͷ��ڴ�
    memory_management_free((void **) &pMessage, packageLength);
}

/**
 * ���� Pubrec
 * @param clientId
 * @param messageId
 */
void sendPubrec(int clientId, int messageId, int msgId) {
    int packageLength = 8;
    char *pMessage;
    // �����ڴ�
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBREC;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 6;
    // ������̶���ͷ
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // ������ɱ䱨ͷ
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);
    // ���⣺broker ��ʶ�����µ���Ϣ���У�
    int2CharArray(msgId, pMessage + byteNum);
    byteNum += 4;
    // ���� PUBREC
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send PUBREC Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send PUBREC success!\n");
    }
    // ���ܷ��ͳɹ����Ҫ�ͷ��ڴ�
    memory_management_free((void **) &pMessage, packageLength);
}

/**
 * ���Ͷ� qos2 ������ȷ��
 * @param clientId
 * @param messageId
 */
void sendPubrel(int clientId, int messageId) {
    int packageLength = 4;
    char *pMessage;
    // �����ڴ�
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBREL;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 2;
    // ������̶���ͷ
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // ������ɱ䱨ͷ
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);
    // ���� PUBREL
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send PUBREL Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send PUBREL success!\n");
    }
    // ���ܷ��ͳɹ����Ҫ�ͷ��ڴ�
    memory_management_free((void **) &pMessage, packageLength);
}

/**
 * ���� pubcomp
 */
void sendPubcomp(int clientId, int messageId) {
    int packageLength = 4;
    char *pMessage;
    // �����ڴ�
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBCOMP;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 2;
    // ������̶���ͷ
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // ������ɱ䱨ͷ
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);
    // ���� PUBCOMP
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send PUBCOMP Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send PUBCOMP success!\n");
    }
    // ���ܷ��ͳɹ����Ҫ�ͷ��ڴ�
    memory_management_free((void **) &pMessage, packageLength);
}

/**
 * ���� qos = 0
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
    // �����ڴ�
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBLISH;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = contentLength + topicLength + 2;
    // ������̶���ͷ
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // ������ɱ䱨ͷ
    byteNum += packageCommonVariableHeader(pMessage + byteNum, topicLength);
    memcpy(pMessage + byteNum, pTopic, topicLength);
    byteNum += topicLength;
    // �������Ч�غ�
    memcpy(pMessage + byteNum, pContent, contentLength);
    byteNum += contentLength;
    // ���� PUBLISH
    int errCode = msgQSend((msg_q *) clientId, pMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: send PUBLISH qos0 fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: send PUBLISH qos0 success!\n");
    }
    // ���ܷ��ͳɹ����Ҫ�ͷ��ڴ�
    memory_management_free((void **) &pMessage, packageLength);
    // ������ͷű�����ռ���ڴ�
    // ����
    semTake(semCMHashTable, WAIT_FOREVER);
    int lenClientHashTable = hash_table_get(pCMHashTable, key);
    if (lenClientHashTable == 0) {
        memory_management_free((void **) &pPublishMessage, MAX_MESSAGE_LENGTH);
        hash_table_rm(pCMHashTable, key);
    } else {
        hash_table_put(pCMHashTable, key, lenClientHashTable - 1);
    }
    // �ͷ�
    semGive(semCMHashTable);
}

/**
 * ���� qos1
 * @param pSessionValue
 * @param pTopicValue
 * @param topicLength
 * @param pContentValue
 * @param contentLength
 * @param messageId
 */
void pubQos1TaskFunction(int pSessionValue, int pTopicValue, int topicLength, int pContentValue, int contentLength,
                         int messageId, int key, int pPublishMessageValue) {
    // 0��׼��
    Session *pSession = (Session *) pSessionValue;
    char *pTopic = (char *) pTopicValue;
    char *pContent = (char *) pContentValue;
    char *pPublishMessage = (char *) pPublishMessageValue;
    MSG_Q_ID msgId = msgQCreate(PUB_MAX_MESSAGE_NUM, MAX_MESSAGE_LENGTH, MSG_Q_PRIORITY);
    char messages[MAX_MESSAGE_LENGTH];  /* ��Ž��ܵ�����Ϣ */
    // 0.1��ʱ����
    UINT16 intervalTime = 0;
    if (pSession->pingTime == 0) {
        intervalTime = PING_TIME / 2;
    } else {
        intervalTime = pSession->pingTime / 2;
    }
    // 0.2����ʱ�����
    int timerId;
    if (eGetTimer(&timerId, TIMER_PERIOD, taskIdSelf(), 1, 0, msgId, intervalTime)) {
        printf("[Error]: get timer fail in the process of processing qos1 publish message(%d)!\n", messageId);
        return;
    }
    // 1���߼�
    int packageLength = 1 + getEncodeLength(contentLength + topicLength + 8) + contentLength + topicLength + 8;
    char *pMessage;
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBLISH;
    fixedHeader.flag = 0x02;
    fixedHeader.remainLength = contentLength + topicLength + 8;
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);   /* ������̶���ͷ */
    byteNum += packageCommonVariableHeader(pMessage + byteNum, topicLength);    /* ������ɱ䱨ͷ */
    memcpy(pMessage + byteNum, pTopic, topicLength);
    byteNum += topicLength;
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);
    int2CharArray((int) msgId, pMessage + byteNum); /* �����Broker��ʶ�����µ���Ϣ����ID�� */
    byteNum += 4;
    memcpy(pMessage + byteNum, pContent, contentLength);    /* �������Ч�غ� */
    // 1.1������ PUBLISH�����ȴ� PUBACK
    if (isInline(pSession)){
        int errCode = msgQSend((msg_q *)(pSession->clientId), pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);
        if (errCode) {
            printf("[Error]: send PUBLISH qos1 message(%d) fail! [ERR_CODE]: %d\n", messageId, errCode);
        } else {
            printf("[Success]: send PUBLISH qos1 message(%d) success!\n", messageId);
        }
        if (eStartTimer(timerId, 2019, intervalTime)) { /* ������ʱ�� */
            printf("[Error]: start timer fail in the process of processing qos2 publish message(%d)!\n", messageId);
            return;
        }
    }
    boolean isACK = False;
    int tCount = 0;
    while (tCount < 3 && isInline(pSession)){
        int msgLen = msgQReceive(msgId, messages, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
        if (msgLen > 0) {
            if (msgLen == 20 && messages[0] == MT_TIMER) {  /* �˴���С���ʳ����жϴ��󣬽��������ϢΪ����ʱ�������ص� */
                int errCode = msgQSend((msg_q *)(pSession->clientId), pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);  /* �� PUBLISH */
                if (errCode) {
                    printf("[Error]: send PUBLISH qos1 message(%d) fail! [ERR_CODE]: %d\n", messageId, errCode);
                } else {
                    printf("[Success]: send PUBLISH qos1 message(%d) success!\n", messageId);
                }
                ++tCount;
            } else{  /* broker ���ص���Ϣ */
                byteNum = getFixedHeader(messages, &fixedHeader);
                if (fixedHeader.messageType == PUBACK) {
                    CommonVariableHeader commonVariableHeader;
                    getCommonVariableHeader(messages + byteNum, &commonVariableHeader);
                    if (messageId == commonVariableHeader.messageId) {
                        pSession->lastReqTime = getCurrentTime();   /* �����ϴ�ͨ��ʱ�� */
                        isACK = True;
                        eStopTimer(timerId);    /* ֹͣ��ʱ�� */
                        break;
                    }
                }
            }
        } else{
            printf("[Error]: message len in the process of publishing qos2 message(%d)!\n", messageId);
        }
    }
    // 1.2��������
    if (isACK) {
        printf("[Success]: ack qos1 publish message(%d)!\n", messageId);
    } else {
        pSession->isOnline = False;
        printf("[Error]: unack %d qos1 publish message(%d)!\n", messageId);
    }
    // 2���ƺ�
    eKillTimer(timerId);    /* �ͷ�����Ķ�ʱ�� */
    msgQDelete(msgId);  /* �ͷ�������Ϣ���� */
    memory_management_free((void **) &pMessage, packageLength); /* ���ܷ��ͳɹ����Ҫ�ͷ��ڴ� */
    // ������������ͷű�����ռ���ڴ棩
    semTake(semCMHashTable, WAIT_FOREVER);
    int lenClientHashTable = hash_table_get(pCMHashTable, key);
    if (lenClientHashTable == 0) {
        memory_management_free((void **) &pPublishMessage, MAX_MESSAGE_LENGTH);
        hash_table_rm(pCMHashTable, key);
    } else {
        hash_table_put(pCMHashTable, key, lenClientHashTable - 1);
    }
    // �ͷ�
    semGive(semCMHashTable);
}

/**
 * ���� qos = 1 �� publish ����
 * ע����Ҫ�����µ� task ��ʹ���µ� ��Ϣ���У��� PUBACK �Ŀɱ䱨ͷ֮����Ч�غ�֮ǰ��������Ϣ���е�ID��������Ϊ���� broker �ı�ʶ��
 * ע���������� �µ���Ϣ����ID �� ���ı�ʶ�� �����õȼ�
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
 * ���� qos2
 * @param pSessionValue
 * @param pTopicValue
 * @param topicLength
 * @param pContentValue
 * @param contentLength
 * @param messageId
 */
void pubQos2TaskFunction(int pSessionValue, int pTopicValue, int topicLength, int pContentValue, int contentLength,
                         int messageId, int key, int pPublishMessageValue) {
    // 0��׼��
    Session *pSession = (Session *) pSessionValue;
    char *pTopic = (char *) pTopicValue;
    char *pContent = (char *) pContentValue;
    char *pPublishMessage = (char *) pPublishMessageValue;
    MSG_Q_ID msgId = msgQCreate(PUB_MAX_MESSAGE_NUM, MAX_MESSAGE_LENGTH, MSG_Q_PRIORITY);   /* ������Ϣ�Ķ��� */
    char messages[MAX_MESSAGE_LENGTH];  /* ��Ž��ܵ�����Ϣ */
    // 0.1�����ʱ��
    UINT16 intervalTime = 0;
    if (pSession->pingTime == 0) {
        intervalTime = PING_TIME / 2;
    } else {
        intervalTime = pSession->pingTime / 2;
    }
    // 0.2����ʱ�����
    int timerId;
    if (eGetTimer(&timerId, TIMER_PERIOD, taskIdSelf(), 1, 0, msgId, intervalTime)) {  /* ��ȡһ����ʱ�� */
        printf("[Error]: get timer fail in the process of processing qos2 publish message(%d)!\n", messageId);
        return;
    }
    // 1���߼�
    // 1.1����� PUBLISH
    int packageLength = 1 + getEncodeLength(contentLength + topicLength + 8) + contentLength + topicLength + 8;
    char *pMessage; /* PUBLISH��Ϣͷָ�� */
    memory_management_allocate(packageLength, (void **) &pMessage);
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBLISH;
    fixedHeader.flag = 0x04;
    fixedHeader.remainLength = contentLength + topicLength + 8;
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);   /* ������̶���ͷ */
    byteNum += packageCommonVariableHeader(pMessage + byteNum, topicLength);    /* ��������� */
    memcpy(pMessage + byteNum, pTopic, topicLength);
    byteNum += topicLength;
    byteNum += packageCommonVariableHeader(pMessage + byteNum, messageId);  /* �������Ϣ��ʶ�� */
    int2CharArray((int) msgId, pMessage + byteNum); /* �����Broker��ʶ�����µ���Ϣ����ID�� */
    byteNum += 4;
    memcpy(pMessage + byteNum, pContent, contentLength);    /* �������Ч�غ� */
    // 1.2������ PUBLISH
    if (isInline(pSession)){    /* �жϿͻ����Ƿ����� */
        int errCode = msgQSend((msg_q *)(pSession->clientId), pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);  /* �� PUBLISH */
        if (errCode) {
            printf("[Error]: send PUBLISH qos2 message(%d) fail! [ERR_CODE]: %d\n", messageId, errCode);
        } else {
            printf("[Success]: send PUBLISH qos2 message(%d) success!\n", messageId);
        }
        if (eStartTimer(timerId, 2019, intervalTime)) { /* ������ʱ�� */
            printf("[Error]: start timer fail in the process of processing qos2 publish message(%d)!\n", messageId);
            return;
        }
    }
    boolean isNeedSendPubrel = False;   /* ��¼�Ƿ��� PUBLISH */
    int tCount = 0;
    while (tCount < 3 && isInline(pSession)){
        int msgLen = msgQReceive(msgId, messages, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
        if (msgLen > 0) {
            if (msgLen == 20 && messages[0] == MT_TIMER) {  /* �˴���С���ʳ����жϴ��󣬽��������ϢΪ����ʱ�������ص� */
                int errCode = msgQSend((msg_q *)(pSession->clientId), pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);  /* �� PUBLISH */
                if (errCode) {
                    printf("[Error]: send PUBLISH qos2 message(%d) fail! [ERR_CODE]: %d\n", messageId, errCode);
                } else {
                    printf("[Success]: send PUBLISH qos2 message(%d)  success!\n", messageId);
                }
                ++tCount;
            } else{  /* broker ���ص���Ϣ */
                byteNum = getFixedHeader(messages, &fixedHeader);
                if (fixedHeader.messageType == PUBREC) {
                    CommonVariableHeader commonVariableHeader;
                    getCommonVariableHeader(messages + byteNum, &commonVariableHeader);
                    if (messageId == commonVariableHeader.messageId) {
                        pSession->lastReqTime = getCurrentTime();   /* �����ϴ�ͨ��ʱ�� */
                        isNeedSendPubrel = True;
                        eStopTimer(timerId);    /* ֹͣ��ʱ�� */
                        while (msgQNumMsgs(msgId) > 0){ /* �����Ϣ���� */
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
    // 1.3������ PUBREL
    boolean isComp = False; /* ��¼�Ƿ���Ҫ���� PUBCOMP */
    tCount = 0;
    if (isNeedSendPubrel && isInline(pSession)) {
        sendPubrel(pSession->clientId, messageId);
        if (eStartTimer(timerId, 2019, intervalTime)) { /* ������ʱ�� */
            printf("[Error]: start timer fail in the process of publishing qos2 message(%d)!\n", messageId);
            return;
        }
        while (tCount < 3 && isInline(pSession)) {
            int msgLen = msgQReceive(msgId, messages, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
            if (msgLen > 0) {
                if (msgLen == 20 && messages[0] == MT_TIMER) {  /* �˴���С���ʳ����жϴ��󣬽��������ϢΪ����ʱ�������ص� */
                    sendPubrel(pSession->clientId, messageId);
                    ++tCount;
                } else {  /* broker ���ص���Ϣ */
                    byteNum = getFixedHeader(messages, &fixedHeader);
                    if (fixedHeader.messageType == PUBCOMP) {
                        CommonVariableHeader commonVariableHeader;
                        getCommonVariableHeader(messages + byteNum, &commonVariableHeader);
                        if (messageId == commonVariableHeader.messageId) {
                            pSession->lastReqTime = getCurrentTime();   /* �����ϴ�ͨ��ʱ�� */
                            isComp = True;
                            eStopTimer(timerId);    /* ֹͣ��ʱ�� */
                            break;
                        }
                    }
                }
            } else {
                printf("[Error]: invalid message len in the process of publishing qos2 message(%d)!\n", messageId);
            }
        }
    }
    // 1.4��������
    if (isComp) {
        printf("[Success]: ack qos2 publish message(%d)!\n", messageId);
    } else {
        pSession->isOnline = False;
        printf("[Error]: unack qos2 publish message(%d)!\n", messageId);
    }
    // 2���ƺ�
    eKillTimer(timerId);    /* �ͷ�����Ķ�ʱ�� */
    msgQDelete(msgId);  /* �ͷ���Ϣ���� */
    memory_management_free((void **) &pMessage, packageLength);
    // ������������ͷű�����ռ���ڴ棩
    semTake(semCMHashTable, WAIT_FOREVER);
    int lenClientHashTable = hash_table_get(pCMHashTable, key);
    if (lenClientHashTable == 0) {
        memory_management_free((void **) &pPublishMessage, MAX_MESSAGE_LENGTH);
        hash_table_rm(pCMHashTable, key);
    } else {
        hash_table_put(pCMHashTable, key, lenClientHashTable - 1);
    }
    // �ͷ�
    semGive(semCMHashTable);
}

/**
 * ���� qos = 2 �� publish ����
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
 * ��Ӧ qos2 publish ����ʵ��
 * @param pSessionValue
 * @param messageId
 */
void responsePublishQos2Function(int pSessionValue, int messageId) {
    // 0��׼��
    Session *pSession = (Session *) pSessionValue;
    MSG_Q_ID msgId = msgQCreate(PUB_MAX_MESSAGE_NUM, MAX_MESSAGE_LENGTH, MSG_Q_PRIORITY);
    char messages[MAX_MESSAGE_LENGTH];
    // 0.1��ʱ����
    UINT16 intervalTime = 0;
    if (pSession->pingTime == 0) {
        intervalTime = PING_TIME / 2;
    } else {
        intervalTime = pSession->pingTime / 2;
    }
    // 0.2����ʱ�����
    int timerId;
    if (eGetTimer(&timerId, TIMER_PERIOD, taskIdSelf(), 1, 0, msgId, intervalTime)) {   /* ��ȡһ����ʱ�� */
        printf("[Error]: get timer fail in the process of processing response Qos2 publish message(%d)!\n", messageId);
        return;
    }
    // 1����Ӧ�߼�
    boolean isNeedPubcomp = False;  /* �Ƿ���Ҫ���� PUBCOMP */
    int tCount = 0; /* ��¼��ʱ��ɴ��� */
    // 1.1������ PUBREC
    if (isInline(pSession)){    /* �жϿͻ����Ƿ����� */
        sendPubrec(pSession->clientId, messageId, (int) msgId);
        if (eStartTimer(timerId, 2019, intervalTime)) { /* ������ʱ�� */
            printf("[Error]: start timer fail in the process of processing response Qos2 publish message(%d)!\n", messageId);
            return;
        }
    }
    while (tCount < 3 && isInline(pSession)) {
        int msgLen = msgQReceive(msgId, messages, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
        if (msgLen > 0) {
            if (msgLen == 20 && messages[0] == MT_TIMER) {  /* �˴���С���ʳ����жϴ��󣬽��������ϢΪ����ʱ�������ص� */
                sendPubrec(pSession->clientId, messageId, (int) msgId);
                ++tCount;
            } else{ /* broker ���ص���Ϣ */
                FixedHeader fixedHeader;
                int byteNum = getFixedHeader(messages, &fixedHeader);
                if (fixedHeader.messageType == PUBREL) {
                    CommonVariableHeader commonVariableHeader;
                    getCommonVariableHeader(messages + byteNum, &commonVariableHeader);
                    if (messageId == commonVariableHeader.messageId) {
                        pSession->lastReqTime = getCurrentTime();   /* �����ϴ�ͨ��ʱ�� */
                        isNeedPubcomp = True;
                        eStopTimer(timerId);    /* ֹͣ��ʱ�� */
                        break;
                    }
                }
            }
        } else {
            printf("[Error]: message len in the process of processing qos2 message(%d) pubrel!\n", messageId);
        }
    }
    // 1.2������ PUBCOMP
    if (isNeedPubcomp) {
        sendPubcomp(pSession->clientId, messageId); /* ���� pubcomp */
        // ���������ڴ� Hashtable ɾ����
        semTake(pSession->semM, WAIT_FOREVER);
        hash_table_rm(pSession->pMessageIdHashTable, messageId);
        // �ͷ�
        semGive(pSession->semM);
    }
    // 2���ƺ���
    eKillTimer(timerId);    /* �ͷ�����Ķ�ʱ�� */
    msgQDelete(msgId);      /* �ͷ���Ϣ���� */
}

/**
 * ��Ӧ qos2 publish
 * @param pSession
 * @param messageId
 */
void responsePublishQos2(Session *pSession, int messageId) {
    taskSpawn("responseQos2Task", PUB_TASK_PRIO, 0, PUB_TASK_STACK,
              (FUNCPTR) responsePublishQos2Function,
              (int) pSession, messageId,
              0, 0, 0, 0, 0, 0, 0, 0);
}
