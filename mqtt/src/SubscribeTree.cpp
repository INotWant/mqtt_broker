#include "SubscribeTree.h"

/**
 * ����һ���µ��������� ���� �������ͻ��˵Ľ��
 */
ClientListNode *createClientListNode(Session *pSession, int qos, SubscribeListNode *pListNode) {
    ClientListNode *pClientListNode;
    memory_management_allocate(sizeof(ClientListNode), (void **) &pClientListNode);
    pClientListNode->pSession = pSession;
    pClientListNode->qos = qos;
    pClientListNode->pListNode = pListNode;
    // ��ȫ��ʼ��
    pClientListNode->pNext = NULL;
    pClientListNode->pPre = NULL;
    return pClientListNode;
}

/**
 * �������ͻ��������в����Ƿ��д˿ͻ���
 */
ClientListNode *findClientListNode(ClientListNode *pClientHead, Session *pSession) {
		if(pSession == NULL){
				return NULL;
		}
    while (pClientHead != NULL) {
        if (pClientHead->pSession == pSession) {
            return pClientHead;
        }
        pClientHead = pClientHead->pNext;
    }
    return NULL;
}


/**
 * �������Ľ����뵽������(ʹ��ͷ�巨)
 */
void insertClientListNode(ClientListNode **ppClientHead, ClientListNode *pCurrNode) {
    if (*ppClientHead == NULL) {
        *ppClientHead = pCurrNode;
    } else {
        (*ppClientHead)->pPre = pCurrNode;
        pCurrNode->pNext = *ppClientHead;
        *ppClientHead = pCurrNode;
    }
}

/**
 * �������ͻ���������ɾ��ĳ�ͻ���
 */
void deleteClientListNode(ClientListNode **ppClientHead, Session *pSession) {
    ClientListNode *pNode = findClientListNode(*ppClientHead, pSession);
    if (pNode != NULL){
   	    if (pNode == *ppClientHead) {
        		*ppClientHead = pNode->pNext;
		    } else {
		        pNode->pPre->pNext = pNode->pNext;
		        if (pNode->pNext != NULL) {
		            pNode->pNext->pPre = pNode->pPre;
		        }
		    }
		    deleteSubscribeListNode(pNode->pListNode, &pSession->pHead);
		    memory_management_free((void **) &pNode, sizeof(ClientListNode));
    }
}

/**
 * ����һ���µĶ��Ľ��
 * ע��ע��ʹ�� mm ��ȡ�ڴ�
 * @param pCurrentTopicName         ��ǰ�������Ƶ��׵�ַ
 * @param currentTopicNameLength    ��ǰ�������Ƶĳ���
 * @return
 */
SubscribeTreeNode *
createSubscribeTreeNode(char *pCurrentTopicName, int currentTopicNameLength) {
    SubscribeTreeNode *pSubscribeTreeNode;
    memory_management_allocate(sizeof(SubscribeTreeNode), (void **) &pSubscribeTreeNode);
    memory_management_allocate(currentTopicNameLength, (void **) &pSubscribeTreeNode->pCurrentTopicName);
    memcpy(pSubscribeTreeNode->pCurrentTopicName, pCurrentTopicName, (size_t) currentTopicNameLength);
    pSubscribeTreeNode->currentTopicNameLength = currentTopicNameLength;
    // ��ȫ��ʼ��
    pSubscribeTreeNode->clientNum = 0;
    pSubscribeTreeNode->pParent = NULL;
    pSubscribeTreeNode->pClientHead = NULL;
    pSubscribeTreeNode->pChild = NULL;
    pSubscribeTreeNode->pBrother = NULL;
    return pSubscribeTreeNode;
}

/**
 * ��ָ��������Ƿ�����ͬ��ǰ�������ƵĽ��
 * @param pCurrFloorRoot
 * @param pCurrTopicName             ��ǰ�������Ƶ��׵�ַ
 * @param currentTopicNameLength    ��ǰ�������Ƶĳ���
 * @param ppLastNode                 ���ڴ洢Ҫ���ҵ���һ��
 * @return ���û�з��� NULL
 */
SubscribeTreeNode *
findSubscribeTreeNodeInCurrFloor(SubscribeTreeNode *pCurrFloorRoot, char *pCurrTopicName, int currentTopicNameLength,
                                 SubscribeTreeNode **ppLastNode) {
    SubscribeTreeNode *pLastNode = NULL;
    while (pCurrFloorRoot != NULL) {
        if (pCurrFloorRoot->currentTopicNameLength == currentTopicNameLength) {
            if (strCompare(pCurrFloorRoot->pCurrentTopicName, pCurrTopicName, currentTopicNameLength)) {
                if (ppLastNode != NULL) {
                    *ppLastNode = pLastNode;
                }
                return pCurrFloorRoot;
            }
        }
        pLastNode = pCurrFloorRoot;
        pCurrFloorRoot = pCurrFloorRoot->pBrother;
    }
    return NULL;
}

/**
 * ���½����Ϊ �ֵ� ���뵽��Ӧ�Ĳ㼶��
 * @param pCurrFloorRoot
 * @param pNewNode
 */
void insertSubscribeTreeNodeAsBrother(SubscribeTreeNode *pCurrFloorRoot, SubscribeTreeNode *pNewNode) {
    while (pCurrFloorRoot->pBrother != NULL) {
        pCurrFloorRoot = pCurrFloorRoot->pBrother;
    }
    pCurrFloorRoot->pBrother = pNewNode;
}


/**
 * ���������в���һ�� ����
 * �����費����� a/// a// ���Ƶ������
 * @param pSubscribeTreeRoot ��������
 * @param pTopic             ���ĵ�ͷ��ַ
 * @param topicLength        ���ĵĳ���
 * @param pSession           �Ựָ��
 * @param qos                ��������
 * @param pListNode         �ڶ��������е�λ�ã��û����ĵ�ȡ���Ȳ�����Ҫ��
 * @param pReturnCode         ������
 * @return �����ڶ������ж�Ӧ�Ľ��
 */
SubscribeTreeNode *insertSubscribeTreeNode(SubscribeTreeNode *pSubscribeTreeRoot, char *pTopic, int topicLength,
                                           Session *pSession, int qos, SubscribeListNode *pListNode,
                                           char *pReturnCode) {
    int lastPos = -1;   // ��һ�� / ��λ��
    SubscribeTreeNode *pCurrFloorRoot = pSubscribeTreeRoot;
    SubscribeTreeNode *pLastFloorRoot = NULL;
    for (int i = 0; i < topicLength; i++) {
        if (pTopic[i] == '/' || i == topicLength - 1) {
            if (i - lastPos > 1 || i == topicLength - 1) {
                // ��ǰ��������ͷ��ַ
                char *pCurrTopicName = pTopic + lastPos + 1;
                int currentTopicNameLength = i - lastPos - 1;
                if (i == topicLength - 1 && pTopic[i] != '/') {
                    currentTopicNameLength = i - lastPos;
                }
                // ���ҵ�ǰ���Ƿ��е�ǰ�������ƶ�Ӧ�Ľ��
                SubscribeTreeNode *pLastNode;
                SubscribeTreeNode *pFindResult = findSubscribeTreeNodeInCurrFloor(pCurrFloorRoot, pCurrTopicName,
                                                                                  currentTopicNameLength, &pLastNode);
                if (pFindResult == NULL) {
                    // �����µĽ��
                    SubscribeTreeNode *pNewNode = createSubscribeTreeNode(pCurrTopicName, currentTopicNameLength);
                    if (pCurrFloorRoot == NULL) {
                        pLastFloorRoot->pChild = pNewNode;
                        pNewNode->pParent = pLastFloorRoot;
                    } else {
                        // ���µĽ����뵽��ǰ����Ҷ�
                        insertSubscribeTreeNodeAsBrother(pCurrFloorRoot, pNewNode);
                        pLastNode->pBrother = pNewNode;
                        pNewNode->pParent = pLastNode;
                    }
                    pFindResult = pNewNode;
                }
                // ��������һ������Ҫά�� pClientHead
                if (i == topicLength - 1) {
                    // ������ԭ�������в����Ƿ������ͬ�Ķ���
                    ClientListNode *pClientNode = findClientListNode(pFindResult->pClientHead, pSession);
                    if (pClientNode != NULL) {
                        // ����Ϊ�µ� ���������
                        pClientNode->qos = qos;
                        pClientNode->pListNode = pListNode;
                    } else {
                        // ������������ ���� �������ͻ��˵Ľ�㣬Ȼ����뵽ָ������
                        pClientNode = createClientListNode(pSession, qos, pListNode);
                        insertClientListNode(&pFindResult->pClientHead, pClientNode);
                        // �������������Ӧ�Ŀͻ��� +1
                        pFindResult->clientNum += 1;
                    }
                    // ������
                    if (pClientNode->qos == 0) {
                        *pReturnCode = 0x00;
                    } else if (pClientNode->qos == 1) {
                        *pReturnCode = 0x01;
                    } else if (pClientNode->qos == 2) {
                        *pReturnCode = 0x02;
                    } else {
                        *pReturnCode = (char) 0x80;
                    }
                    return pFindResult;
                }
                // ������һ��
                pLastFloorRoot = pFindResult;
                pCurrFloorRoot = pFindResult->pChild;
            }
            lastPos = i;
        }
    }
}

/**
 * �Ӷ�������ɾ��ָ�� ���������
 * (���費����� a/// ���Ƶ������������������ǹ淶����)
 * @param pSubscribeTreeRoot
 * @param pTopic
 * @param topicLength
 * @param pSession
 */
void deleteSubscribeTreeNodeByTopicFilter(SubscribeTreeNode *pSubscribeTreeRoot, char *pTopic, int topicLength,
                                          Session *pSession) {
    // ��һ�� "/" ��λ��
    int lastPos = -1;
    SubscribeTreeNode *pCurrFloorRoot = pSubscribeTreeRoot;
    for (int i = 0; i < topicLength; i++) {
        if (pTopic[i] == '/' || i == topicLength - 1) {
            if (i - lastPos > 1 || i == topicLength - 1) {
                // ��ǰ��������ͷ��ַ
                char *pCurrTopicName = pTopic + lastPos + 1;
                int currentTopicNameLength = i - lastPos - 1;
                if (i == topicLength - 1 && pTopic[i] != '/') {
                    currentTopicNameLength = i - lastPos;
                }
                // ���ҵ�ǰ���Ƿ��е�ǰ�������ƶ�Ӧ�Ľ��
                SubscribeTreeNode *pNode = findSubscribeTreeNodeInCurrFloor(pCurrFloorRoot, pCurrTopicName,
                                                                            currentTopicNameLength, NULL);
                if (pNode == NULL) {
                    printf("[Info]: Invalid topic filter, in process of processing UNSUBSCRIBE !");
                    return;
                } else {
                    // �ж��Ƿ�Ϊ���һ��
                    if (i == topicLength - 1) {
                        // ����ɾ��
                        boolean flag = True;       // Ϊ False ʱ��һ���޿�ɾ���
                        boolean isDeleted = False;  // ������һ�����Ƿ���ɾ��
                        SubscribeTreeNode *pEnd = pNode;
                        while (flag && pNode->pChild == NULL && pNode->clientNum == 1) {
                            SubscribeTreeNode *pDeleteNode = pNode;
                            // ��ʼ������� `+` `#` �������������
                            if (pDeleteNode != pSubscribeTreeRoot && pDeleteNode != pSubscribeTreeRoot->pBrother) {
                                isDeleted = True;
                                // ɾǰ׼��
                                if (pNode->pParent->pChild == pNode) {
                                    // ���֧
                                    pNode->pParent->pChild = pNode->pBrother;
                                    if (pNode->pBrother != NULL) {
                                        pNode->pBrother->pParent = pNode->pParent;
                                    }
                                    pNode = pNode->pParent;
                                } else {
                                    // �����ҷ�֧�������һ���޿�ɾ��
                                    pNode->pParent->pBrother = pNode->pBrother;
                                    if (pNode->pBrother != NULL) {
                                        pNode->pBrother->pParent = pNode->pParent;
                                    }
                                    flag = False;
                                }
                                // ��ɾ���ý���� Session �ж�Ӧ�� �������� �еĽ��
                                deleteSubscribeListNode(pDeleteNode->pClientHead->pListNode, &pSession->pHead);
                                // ��ɾ������
                                memory_management_free((void **) &pDeleteNode->pClientHead, sizeof(ClientListNode));
                                // ��ɾ����
                                memory_management_free((void **) &pDeleteNode->pCurrentTopicName,
                                                       pDeleteNode->currentTopicNameLength);
                                // ��ɾ�����
                                memory_management_free((void **) &pDeleteNode, sizeof(SubscribeTreeNode));
                            }
                        }
                        // �����һ����û��ɾ��ʱ������Ҫ�Ӹý���ϵ������ͻ�������ɾ����Ӧ�Ľ��
                        if (!isDeleted) {
                            deleteClientListNode(&pEnd->pClientHead, pSession);
                            pEnd->clientNum -= 1;
                        }
                        // ���� for ѭ��
                        break;
                    }
                }
                // ������һ��
                pCurrFloorRoot = pNode->pChild;
            }
            lastPos = i;
        }
    }
}

/**
 * ɾ�� ��������������Ŀͻ��� ����
 */
void deleteClientList(ClientListNode *pClientHead) {
    while (pClientHead != NULL) {
        ClientListNode *pNode = pClientHead;
        pClientHead = pClientHead->pNext;
        memory_management_free((void **) &pNode, sizeof(ClientListNode));
    }
}

/**
 * ɾ������������
 * ע����Ҫ����ɾ������ Session �����ж���
 * @param pSubscribeTreeRoot
 */
void deleteSubscribeTree(SubscribeTreeNode *pSubscribeTreeRoot) {
    if (pSubscribeTreeRoot != NULL) {
        if (pSubscribeTreeRoot->pChild != NULL) {
            deleteSubscribeTree(pSubscribeTreeRoot->pChild);
        }
        if (pSubscribeTreeRoot->pBrother != NULL) {
            deleteSubscribeTree(pSubscribeTreeRoot->pBrother);
        }
        // ɾ����ǰ���
        // ��ɾ�� ��������������Ŀͻ��� ����
        deleteClientList(pSubscribeTreeRoot->pClientHead);
        // ��ɾ����
        memory_management_free((void **) &pSubscribeTreeRoot->pCurrentTopicName,
                               pSubscribeTreeRoot->currentTopicNameLength);
        // ��ɾ�����
        memory_management_free((void **) &pSubscribeTreeRoot, sizeof(SubscribeTreeNode));
    }
}

/**
 * �Ӷ�������ɾ��ָ�����
 * ע����Ҫ����ɾ���ý���� Session �ж�Ӧ�� �������� �еĽ��
 * @param pSubscribeTreeRoot
 * @param pSubscribeListNode
 * @param pSession
 */
void
deleteSubscribeTreeNodeByAddress(SubscribeTreeNode *pSubscribeTreeRoot, SubscribeTreeNode *pNode, Session *pSession) {
    SubscribeTreeNode *pEnd = pNode;
    boolean flag = True;       // Ϊ False ʱ��һ���޿�ɾ���
    boolean isDeleted = False;  // ������һ�����Ƿ���ɾ��
    while (flag && pNode->pChild == NULL && pNode->clientNum == 1) {
        SubscribeTreeNode *pDeleteNode = pNode;
        // ��ʼ������� `+` `#` �������������
        if (pDeleteNode != pSubscribeTreeRoot && pDeleteNode != pSubscribeTreeRoot->pBrother) {
            isDeleted = True;
            // ɾǰ׼��
            if (pNode->pParent->pChild == pNode) {
                // ���֧
                pNode->pParent->pChild = pNode->pBrother;
                if (pNode->pBrother != NULL) {
                    pNode->pBrother->pParent = pNode->pParent;
                }
                pNode = pNode->pParent;
            } else {
                // �����ҷ�֧�������һ���޿�ɾ��
                pNode->pParent->pBrother = pNode->pBrother;
                if (pNode->pBrother != NULL) {
                    pNode->pBrother->pParent = pNode->pParent;
                }
                flag = False;
            }
            // ��ɾ���ý���� Session �ж�Ӧ�� �������� �еĽ��
//            deleteSubscribeListNode(pDeleteNode->pClientHead->pListNode, &pSession->pHead);
            // ��ɾ������
            memory_management_free((void **) &pDeleteNode->pClientHead, sizeof(ClientListNode));
            // ��ɾ����
            memory_management_free((void **) &pDeleteNode->pCurrentTopicName,
                                   pDeleteNode->currentTopicNameLength);
            // ��ɾ�����
            memory_management_free((void **) &pDeleteNode, sizeof(SubscribeTreeNode));
        }
    }
    // �����һ����û��ɾ��ʱ������Ҫ�Ӹý���ϵ������ͻ�������ɾ����Ӧ�Ľ��
    if (!isDeleted) {
        deleteClientListNode(&pEnd->pClientHead, pSession);
        pEnd->clientNum -= 1;
    }
}

/**
 * �Ӷ������м��������ж���ָ������Ŀͻ���
 * (���費����� a// a/// ���Ƶ������������������ǹ淶����)
 * @param pSubscribeTreeRoot
 * @param pTopic
 * @param topicLength
 * @param pHashTable
 */
void findAllClientInSubscribeTree(SubscribeTreeNode *pSubscribeTreeRoot, char *pTopic, int topicLength,
                                  HashTable *pHashTable) {
    // ��һ�� "/" ��λ��
    int lastPos = -1;
    SubscribeTreeNode *pCurrFloorRoot = pSubscribeTreeRoot;
    for (int i = 0; i < topicLength; i++) {
        if (pTopic[i] == '/' || i == topicLength - 1) {
            if (i - lastPos > 1 || i == topicLength - 1) {
                // ��ǰ��������ͷ��ַ
                char *pCurrTopicName = pTopic + lastPos + 1;
                int currentTopicNameLength = i - lastPos - 1;
                if (i == topicLength - 1 && pTopic[i] != '/') {
                    currentTopicNameLength = i - lastPos;
                }
                SubscribeTreeNode *pNode = pCurrFloorRoot;
                while (pNode != NULL) {
                    if (pNode->pCurrentTopicName[0] == '#') {
                        // ͨ��� # ����
                        ClientListNode *pClientNode = pNode->pClientHead;
                        while (pClientNode != NULL) {
                            // �鿴�ûỰ�Ƿ�֮ǰ����֮ƥ�䣬���У����� qos �����Ƿ����
                            int qos = hash_table_get(pHashTable, (int) pClientNode->pSession);
                            if (qos == NULL || qos < pClientNode->qos) {
                                hash_table_put(pHashTable, (int) pClientNode->pSession, pClientNode->qos);
                            }
                            pClientNode = pClientNode->pNext;
                        }
                    } else if (pNode->pCurrentTopicName[0] == '+' ||
                               (pNode->currentTopicNameLength == currentTopicNameLength &&
                                strCompare(pNode->pCurrentTopicName, pCurrTopicName, currentTopicNameLength))) {
                        // ͨ��� + ���� || (��ǰ����������ȫƥ������)
                        if (i == topicLength - 1) {
                            // �����һ��
                            ClientListNode *pClientNode = pNode->pClientHead;
                            while (pClientNode != NULL) {
                                // �鿴�ûỰ�Ƿ�֮ǰ����֮ƥ�䣬���У����� qos �����Ƿ����
                                int qos = hash_table_get(pHashTable, (int) pClientNode->pSession);
                                if (qos == NULL || qos < pClientNode->qos) {
                                    hash_table_put(pHashTable, (int)pClientNode->pSession, pClientNode->qos);
                                }
                                pClientNode = pClientNode->pNext;
                            }
                        } else {
                            // �������һ�㣬���еݹ�
                            findAllClientInSubscribeTree(pNode->pChild, pCurrTopicName + currentTopicNameLength + 1,
                                                         topicLength - i - 1,
                                                         pHashTable);
                        }
                    }
                    pNode = pNode->pBrother;
                }
            }
            lastPos = i;
        }
    }
}

