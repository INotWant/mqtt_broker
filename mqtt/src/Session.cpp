#include "Define.h"
#include "Session.h"
#include "SubscribeTree.h"

/******************************************************************
 *                       会话状态的相关操作实现
 ******************************************************************/

/**
 * 创建订阅链表结点
 * @param deep
 * @param pSubscribe
 * @return
 */
SubscribeListNode *createSubscribeListNode(UINT8 deep) {
    SubscribeListNode *pNewNode;
    memory_management_allocate(sizeof(SubscribeListNode), (void **) &pNewNode);
    pNewNode->deep = deep;
    // 安全初始化
    pNewNode->pTreeNode = NULL;
    pNewNode->pNext = NULL;
    pNewNode->pPre = NULL;
    return pNewNode;
}

/**
 * 插入到订阅链表
 * @param ppHead
 * @param pSubscribeListNode
 * @return
 */
void insertSubscribeListNode(SubscribeListNode **ppHead, SubscribeListNode *pSubscribeListNode) {
    if (*ppHead == NULL) {
        *ppHead = pSubscribeListNode;
    } else {
        SubscribeListNode *pNode = *ppHead;
        SubscribeListNode *pLastNode = NULL;
        while (pNode != NULL && pNode->deep >= pSubscribeListNode->deep) {
            pLastNode = pNode;
            pNode = pNode->pNext;
        }
        if (pNode == *ppHead) {
            pSubscribeListNode->pNext = pNode;
            pNode->pPre = pSubscribeListNode;
            *ppHead = pSubscribeListNode;
        } else if (pNode == NULL) {
            pLastNode->pNext = pSubscribeListNode;
            pSubscribeListNode->pPre = pLastNode;
        }
    }
}

/**
 * 删除订阅链表结点
 */
void deleteSubscribeListNode(SubscribeListNode *pSubscribeListNode, SubscribeListNode **ppSubscribeListHead) {
    if (*ppSubscribeListHead == pSubscribeListNode) {
        *ppSubscribeListHead = pSubscribeListNode->pNext;
    } else {
        pSubscribeListNode->pPre->pNext = pSubscribeListNode->pNext;
        if (pSubscribeListNode->pNext != NULL) {
            pSubscribeListNode->pNext->pPre = pSubscribeListNode->pPre;
        }
    }
    memory_management_free((void **) &pSubscribeListNode, sizeof(SubscribeListNode));
}

/**
 *  删除订阅链表
 */
void deleteSubscribeList(SubscribeListNode *pSubscribeListHead) {
    while (pSubscribeListHead != NULL) {
        SubscribeListNode *pNode = pSubscribeListHead;
        pSubscribeListHead = pSubscribeListHead->pNext;
        memory_management_free((void **) &pNode, sizeof(SubscribeListNode));
    }
}

/**
* 获取指定 session 的状态，返回 在线 或者 离线
* 注：比较（当前时间 - 上次请求时间）与 （pingTime * 1.5）
* 注：以 ms 为单位
* @return True 表明仍 在线，返回 False 表明已断开连接。
*/
boolean isInline(Session *pSession) {
    if (getCurrentTime() - pSession->lastReqTime > 15 * pSession->pingTime / 10) {
        pSession->isOnline = False;
    } else {
        pSession->isOnline = True;
    }
    return pSession->isOnline;
}

/**
* 当有新的客户端连接到来，必要时创建一个新的 会话状态
* 注：使用 mm 获取内存
*/
Session *createSession(int clientId, boolean cleanSession, UINT16 pingTime) {
    Session *pSession = NULL;
    memory_management_allocate(sizeof(Session), (void **) &pSession);
    pSession->clientId = clientId;
    pSession->cleanSession = cleanSession;
    pSession->isOnline = True;
    pSession->pingTime = pingTime;
    pSession->lastReqTime = getCurrentTime();
    // 安全清理
    pSession->pHead = NULL;
    // pSession->pOutlineMessageHead = NULL;
    pSession->pNext = NULL;
    pSession->pPre = NULL;
    // 保存 messageId
    pSession->pMessageIdHashTable = hash_table_new();
    pSession->semM = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
    return pSession;
}

/**
 * 将新 Session 插入到 会话状态链表中 (使用头插)
 */
void insertSession(Session *pSession, Session **ppSessionHead) {
    if (*ppSessionHead == NULL) {
        // 原来会话状态链表为空
        *ppSessionHead = pSession;
    } else {
        // 不为空
        (**ppSessionHead).pPre = pSession;
        pSession->pNext = *ppSessionHead;
        *ppSessionHead = pSession;
    }
}

/**
* 删除指定 session
* 注：内存的释放
*/
void deleteSession(SubscribeTreeNode *pSubscribeTreeRoot, Session **ppSessionHead, Session *pSession) {
    SubscribeListNode *pNode = pSession->pHead;
    while (pNode != NULL) {
        SubscribeListNode *pCurrNode = pNode;
        deleteSubscribeTreeNodeByAddress(pSubscribeTreeRoot, pNode->pTreeNode, pSession);
        pNode = pNode->pNext;
        memory_management_free((void **) &pCurrNode, sizeof(SubscribeListNode));
    }
    if (*ppSessionHead == pSession) {
        *ppSessionHead = pSession->pNext;
    } else {
        pSession->pPre->pNext = pSession->pNext;
        if (pSession->pNext != NULL) {
            pSession->pNext->pPre = pSession->pPre;
        }
    }
    memory_management_free((void **) &pSession, sizeof(Session));
}

/**
 * 删除所有 Session
 */
void deleteAllSession(Session *pSessionHead) {
    while (pSessionHead != NULL) {
        Session *pNode = pSessionHead;
        pSessionHead = pSessionHead->pNext;
        deleteSubscribeList(pNode->pHead);
        hash_table_delete(pNode->pMessageIdHashTable);
        semDelete(pNode->semM);
    }
}

/**
 * 修改 Session 的上次通信时间
 */
void modifySessionLastReqTime(long lastReqTime, Session *pSession) {
    pSession->lastReqTime = lastReqTime;
}

/**
 * 根据 客户端标识符 从原来的会话状态链表中查找
 */
Session *findSession(int clientId, Session *pSessionHead) {
    Session *pCurrSession = pSessionHead;
    while (pCurrSession != NULL) {
        if (pCurrSession->clientId == clientId) {
            return pCurrSession;
        }
        pCurrSession = pCurrSession->pNext;
    }
    return NULL;
}
