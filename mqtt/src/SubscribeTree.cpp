#include "SubscribeTree.h"

/**
 * 创建一个新的用于描述 订阅 的所属客户端的结点
 */
ClientListNode *createClientListNode(Session *pSession, int qos, SubscribeListNode *pListNode) {
    ClientListNode *pClientListNode;
    memory_management_allocate(sizeof(ClientListNode), (void **) &pClientListNode);
    pClientListNode->pSession = pSession;
    pClientListNode->qos = qos;
    pClientListNode->pListNode = pListNode;
    // 安全初始化
    pClientListNode->pNext = NULL;
    pClientListNode->pPre = NULL;
    return pClientListNode;
}

/**
 * 从所属客户端链表中查找是否有此客户端
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
 * 将上述的结点插入到链表中(使用头插法)
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
 * 从所属客户端链表中删除某客户端
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
 * 创建一个新的订阅结点
 * 注：注：使用 mm 获取内存
 * @param pCurrentTopicName         当前主题名称的首地址
 * @param currentTopicNameLength    当前主题名称的长度
 * @return
 */
SubscribeTreeNode *
createSubscribeTreeNode(char *pCurrentTopicName, int currentTopicNameLength) {
    SubscribeTreeNode *pSubscribeTreeNode;
    memory_management_allocate(sizeof(SubscribeTreeNode), (void **) &pSubscribeTreeNode);
    memory_management_allocate(currentTopicNameLength, (void **) &pSubscribeTreeNode->pCurrentTopicName);
    memcpy(pSubscribeTreeNode->pCurrentTopicName, pCurrentTopicName, (size_t) currentTopicNameLength);
    pSubscribeTreeNode->currentTopicNameLength = currentTopicNameLength;
    // 安全初始化
    pSubscribeTreeNode->clientNum = 0;
    pSubscribeTreeNode->pParent = NULL;
    pSubscribeTreeNode->pClientHead = NULL;
    pSubscribeTreeNode->pChild = NULL;
    pSubscribeTreeNode->pBrother = NULL;
    return pSubscribeTreeNode;
}

/**
 * 从指定层查找是否有相同当前主题名称的结点
 * @param pCurrFloorRoot
 * @param pCurrTopicName             当前主题名称的首地址
 * @param currentTopicNameLength    当前主题名称的长度
 * @param ppLastNode                 用于存储要查找的上一个
 * @return 如果没有返回 NULL
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
 * 将新结点作为 兄弟 插入到对应的层级中
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
 * 往订阅树中插入一个 订阅
 * （假设不会出现 a/// a// 类似的情况）
 * @param pSubscribeTreeRoot 订阅树根
 * @param pTopic             订阅的头地址
 * @param topicLength        订阅的长度
 * @param pSession           会话指针
 * @param qos                订阅质量
 * @param pListNode         在订阅链表中的位置（用户订阅的取消等操作需要）
 * @param pReturnCode         返回码
 * @return 返回在订阅树中对应的结点
 */
SubscribeTreeNode *insertSubscribeTreeNode(SubscribeTreeNode *pSubscribeTreeRoot, char *pTopic, int topicLength,
                                           Session *pSession, int qos, SubscribeListNode *pListNode,
                                           char *pReturnCode) {
    int lastPos = -1;   // 上一个 / 的位置
    SubscribeTreeNode *pCurrFloorRoot = pSubscribeTreeRoot;
    SubscribeTreeNode *pLastFloorRoot = NULL;
    for (int i = 0; i < topicLength; i++) {
        if (pTopic[i] == '/' || i == topicLength - 1) {
            if (i - lastPos > 1 || i == topicLength - 1) {
                // 当前主题名称头地址
                char *pCurrTopicName = pTopic + lastPos + 1;
                int currentTopicNameLength = i - lastPos - 1;
                if (i == topicLength - 1 && pTopic[i] != '/') {
                    currentTopicNameLength = i - lastPos;
                }
                // 查找当前层是否有当前主题名称对应的结点
                SubscribeTreeNode *pLastNode;
                SubscribeTreeNode *pFindResult = findSubscribeTreeNodeInCurrFloor(pCurrFloorRoot, pCurrTopicName,
                                                                                  currentTopicNameLength, &pLastNode);
                if (pFindResult == NULL) {
                    // 创建新的结点
                    SubscribeTreeNode *pNewNode = createSubscribeTreeNode(pCurrTopicName, currentTopicNameLength);
                    if (pCurrFloorRoot == NULL) {
                        pLastFloorRoot->pChild = pNewNode;
                        pNewNode->pParent = pLastFloorRoot;
                    } else {
                        // 将新的结点插入到当前层的右端
                        insertSubscribeTreeNodeAsBrother(pCurrFloorRoot, pNewNode);
                        pLastNode->pBrother = pNewNode;
                        pNewNode->pParent = pLastNode;
                    }
                    pFindResult = pNewNode;
                }
                // 如果是最后一层则需要维护 pClientHead
                if (i == topicLength - 1) {
                    // 首先在原来链表中查找是否存在相同的订阅
                    ClientListNode *pClientNode = findClientListNode(pFindResult->pClientHead, pSession);
                    if (pClientNode != NULL) {
                        // 更新为新的 主题过滤器
                        pClientNode->qos = qos;
                        pClientNode->pListNode = pListNode;
                    } else {
                        // 创建用于描述 订阅 的所属客户端的结点，然后插入到指定链表
                        pClientNode = createClientListNode(pSession, qos, pListNode);
                        insertClientListNode(&pFindResult->pClientHead, pClientNode);
                        // 该主题过滤器对应的客户端 +1
                        pFindResult->clientNum += 1;
                    }
                    // 返回码
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
                // 进入下一层
                pLastFloorRoot = pFindResult;
                pCurrFloorRoot = pFindResult->pChild;
            }
            lastPos = i;
        }
    }
}

/**
 * 从订阅树中删除指定 主题过滤器
 * (假设不会出现 a/// 类似的情况，即主题过滤器是规范化的)
 * @param pSubscribeTreeRoot
 * @param pTopic
 * @param topicLength
 * @param pSession
 */
void deleteSubscribeTreeNodeByTopicFilter(SubscribeTreeNode *pSubscribeTreeRoot, char *pTopic, int topicLength,
                                          Session *pSession) {
    // 上一个 "/" 的位置
    int lastPos = -1;
    SubscribeTreeNode *pCurrFloorRoot = pSubscribeTreeRoot;
    for (int i = 0; i < topicLength; i++) {
        if (pTopic[i] == '/' || i == topicLength - 1) {
            if (i - lastPos > 1 || i == topicLength - 1) {
                // 当前主题名称头地址
                char *pCurrTopicName = pTopic + lastPos + 1;
                int currentTopicNameLength = i - lastPos - 1;
                if (i == topicLength - 1 && pTopic[i] != '/') {
                    currentTopicNameLength = i - lastPos;
                }
                // 查找当前层是否有当前主题名称对应的结点
                SubscribeTreeNode *pNode = findSubscribeTreeNodeInCurrFloor(pCurrFloorRoot, pCurrTopicName,
                                                                            currentTopicNameLength, NULL);
                if (pNode == NULL) {
                    printf("[Info]: Invalid topic filter, in process of processing UNSUBSCRIBE !");
                    return;
                } else {
                    // 判断是否为最后一层
                    if (i == topicLength - 1) {
                        // 倒着删除
                        boolean flag = True;       // 为 False 时，一定无可删结点
                        boolean isDeleted = False;  // 标记最后一个层是否已删除
                        SubscribeTreeNode *pEnd = pNode;
                        while (flag && pNode->pChild == NULL && pNode->clientNum == 1) {
                            SubscribeTreeNode *pDeleteNode = pNode;
                            // 初始化构造的 `+` `#` 结点有免死金牌
                            if (pDeleteNode != pSubscribeTreeRoot && pDeleteNode != pSubscribeTreeRoot->pBrother) {
                                isDeleted = True;
                                // 删前准备
                                if (pNode->pParent->pChild == pNode) {
                                    // 左分支
                                    pNode->pParent->pChild = pNode->pBrother;
                                    if (pNode->pBrother != NULL) {
                                        pNode->pBrother->pParent = pNode->pParent;
                                    }
                                    pNode = pNode->pParent;
                                } else {
                                    // 进入右分支，父结点一定无可删的
                                    pNode->pParent->pBrother = pNode->pBrother;
                                    if (pNode->pBrother != NULL) {
                                        pNode->pBrother->pParent = pNode->pParent;
                                    }
                                    flag = False;
                                }
                                // 先删除该结点在 Session 中对应在 订阅链表 中的结点
                                deleteSubscribeListNode(pDeleteNode->pClientHead->pListNode, &pSession->pHead);
                                // 再删除链表
                                memory_management_free((void **) &pDeleteNode->pClientHead, sizeof(ClientListNode));
                                // 再删主题
                                memory_management_free((void **) &pDeleteNode->pCurrentTopicName,
                                                       pDeleteNode->currentTopicNameLength);
                                // 再删除结点
                                memory_management_free((void **) &pDeleteNode, sizeof(SubscribeTreeNode));
                            }
                        }
                        // 当最后一个层没有删除时，则需要从该结点上的所属客户端链表删除对应的结点
                        if (!isDeleted) {
                            deleteClientListNode(&pEnd->pClientHead, pSession);
                            pEnd->clientNum -= 1;
                        }
                        // 跳出 for 循环
                        break;
                    }
                }
                // 进入下一层
                pCurrFloorRoot = pNode->pChild;
            }
            lastPos = i;
        }
    }
}

/**
 * 删除 主题过滤器所属的客户端 链表
 */
void deleteClientList(ClientListNode *pClientHead) {
    while (pClientHead != NULL) {
        ClientListNode *pNode = pClientHead;
        pClientHead = pClientHead->pNext;
        memory_management_free((void **) &pNode, sizeof(ClientListNode));
    }
}

/**
 * 删除整个订阅树
 * 注：需要自行删除所有 Session 中所有订阅
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
        // 删除当前结点
        // 先删除 主题过滤器所属的客户端 链表
        deleteClientList(pSubscribeTreeRoot->pClientHead);
        // 再删主题
        memory_management_free((void **) &pSubscribeTreeRoot->pCurrentTopicName,
                               pSubscribeTreeRoot->currentTopicNameLength);
        // 再删除结点
        memory_management_free((void **) &pSubscribeTreeRoot, sizeof(SubscribeTreeNode));
    }
}

/**
 * 从订阅树中删除指定结点
 * 注：需要自行删除该结点在 Session 中对应的 订阅链表 中的结点
 * @param pSubscribeTreeRoot
 * @param pSubscribeListNode
 * @param pSession
 */
void
deleteSubscribeTreeNodeByAddress(SubscribeTreeNode *pSubscribeTreeRoot, SubscribeTreeNode *pNode, Session *pSession) {
    SubscribeTreeNode *pEnd = pNode;
    boolean flag = True;       // 为 False 时，一定无可删结点
    boolean isDeleted = False;  // 标记最后一个层是否已删除
    while (flag && pNode->pChild == NULL && pNode->clientNum == 1) {
        SubscribeTreeNode *pDeleteNode = pNode;
        // 初始化构造的 `+` `#` 结点有免死金牌
        if (pDeleteNode != pSubscribeTreeRoot && pDeleteNode != pSubscribeTreeRoot->pBrother) {
            isDeleted = True;
            // 删前准备
            if (pNode->pParent->pChild == pNode) {
                // 左分支
                pNode->pParent->pChild = pNode->pBrother;
                if (pNode->pBrother != NULL) {
                    pNode->pBrother->pParent = pNode->pParent;
                }
                pNode = pNode->pParent;
            } else {
                // 进入右分支，父结点一定无可删的
                pNode->pParent->pBrother = pNode->pBrother;
                if (pNode->pBrother != NULL) {
                    pNode->pBrother->pParent = pNode->pParent;
                }
                flag = False;
            }
            // 先删除该结点在 Session 中对应在 订阅链表 中的结点
//            deleteSubscribeListNode(pDeleteNode->pClientHead->pListNode, &pSession->pHead);
            // 再删除链表
            memory_management_free((void **) &pDeleteNode->pClientHead, sizeof(ClientListNode));
            // 再删主题
            memory_management_free((void **) &pDeleteNode->pCurrentTopicName,
                                   pDeleteNode->currentTopicNameLength);
            // 再删除结点
            memory_management_free((void **) &pDeleteNode, sizeof(SubscribeTreeNode));
        }
    }
    // 当最后一个层没有删除时，则需要从该结点上的所属客户端链表删除对应的结点
    if (!isDeleted) {
        deleteClientListNode(&pEnd->pClientHead, pSession);
        pEnd->clientNum -= 1;
    }
}

/**
 * 从订阅树中检索出所有订阅指定主题的客户端
 * (假设不会出现 a// a/// 类似的情况，即主题过滤器是规范化的)
 * @param pSubscribeTreeRoot
 * @param pTopic
 * @param topicLength
 * @param pHashTable
 */
void findAllClientInSubscribeTree(SubscribeTreeNode *pSubscribeTreeRoot, char *pTopic, int topicLength,
                                  HashTable *pHashTable) {
    // 上一个 "/" 的位置
    int lastPos = -1;
    SubscribeTreeNode *pCurrFloorRoot = pSubscribeTreeRoot;
    for (int i = 0; i < topicLength; i++) {
        if (pTopic[i] == '/' || i == topicLength - 1) {
            if (i - lastPos > 1 || i == topicLength - 1) {
                // 当前主题名称头地址
                char *pCurrTopicName = pTopic + lastPos + 1;
                int currentTopicNameLength = i - lastPos - 1;
                if (i == topicLength - 1 && pTopic[i] != '/') {
                    currentTopicNameLength = i - lastPos;
                }
                SubscribeTreeNode *pNode = pCurrFloorRoot;
                while (pNode != NULL) {
                    if (pNode->pCurrentTopicName[0] == '#') {
                        // 通配符 # 处理
                        ClientListNode *pClientNode = pNode->pClientHead;
                        while (pClientNode != NULL) {
                            // 查看该会话是否之前已与之匹配，若有，则由 qos 决定是否更新
                            int qos = hash_table_get(pHashTable, (int) pClientNode->pSession);
                            if (qos == NULL || qos < pClientNode->qos) {
                                hash_table_put(pHashTable, (int) pClientNode->pSession, pClientNode->qos);
                            }
                            pClientNode = pClientNode->pNext;
                        }
                    } else if (pNode->pCurrentTopicName[0] == '+' ||
                               (pNode->currentTopicNameLength == currentTopicNameLength &&
                                strCompare(pNode->pCurrentTopicName, pCurrTopicName, currentTopicNameLength))) {
                        // 通配符 + 处理 || (当前主题名称完全匹配的情况)
                        if (i == topicLength - 1) {
                            // 是最后一层
                            ClientListNode *pClientNode = pNode->pClientHead;
                            while (pClientNode != NULL) {
                                // 查看该会话是否之前已与之匹配，若有，则由 qos 决定是否更新
                                int qos = hash_table_get(pHashTable, (int) pClientNode->pSession);
                                if (qos == NULL || qos < pClientNode->qos) {
                                    hash_table_put(pHashTable, (int)pClientNode->pSession, pClientNode->qos);
                                }
                                pClientNode = pClientNode->pNext;
                            }
                        } else {
                            // 不是最后一层，进行递归
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

