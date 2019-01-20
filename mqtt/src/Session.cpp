#include "Define.h"
#include "Session.h"
#include "SubscribeTree.h"

/******************************************************************
 *                       �Ự״̬����ز���ʵ��
 ******************************************************************/

/**
 * ��������������
 * @param deep
 * @param pSubscribe
 * @return
 */
SubscribeListNode *createSubscribeListNode(UINT8 deep) {
    SubscribeListNode *pNewNode;
    memory_management_allocate(sizeof(SubscribeListNode), (void **) &pNewNode);
    pNewNode->deep = deep;
    // ��ȫ��ʼ��
    pNewNode->pTreeNode = NULL;
    pNewNode->pNext = NULL;
    pNewNode->pPre = NULL;
    return pNewNode;
}

/**
 * ���뵽��������
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
 * ɾ������������
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
 *  ɾ����������
 */
void deleteSubscribeList(SubscribeListNode *pSubscribeListHead) {
    while (pSubscribeListHead != NULL) {
        SubscribeListNode *pNode = pSubscribeListHead;
        pSubscribeListHead = pSubscribeListHead->pNext;
        memory_management_free((void **) &pNode, sizeof(SubscribeListNode));
    }
}

/**
* ��ȡָ�� session ��״̬������ ���� ���� ����
* ע���Ƚϣ���ǰʱ�� - �ϴ�����ʱ�䣩�� ��pingTime * 1.5��
* ע���� ms Ϊ��λ
* @return True ������ ���ߣ����� False �����ѶϿ����ӡ�
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
* �����µĿͻ������ӵ�������Ҫʱ����һ���µ� �Ự״̬
* ע��ʹ�� mm ��ȡ�ڴ�
*/
Session *createSession(int clientId, boolean cleanSession, UINT16 pingTime) {
    Session *pSession = NULL;
    memory_management_allocate(sizeof(Session), (void **) &pSession);
    pSession->clientId = clientId;
    pSession->cleanSession = cleanSession;
    pSession->isOnline = True;
    pSession->pingTime = pingTime;
    pSession->lastReqTime = getCurrentTime();
    // ��ȫ����
    pSession->pHead = NULL;
    // pSession->pOutlineMessageHead = NULL;
    pSession->pNext = NULL;
    pSession->pPre = NULL;
    // ���� messageId
    pSession->pMessageIdHashTable = hash_table_new();
    pSession->semM = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
    return pSession;
}

/**
 * ���� Session ���뵽 �Ự״̬������ (ʹ��ͷ��)
 */
void insertSession(Session *pSession, Session **ppSessionHead) {
    if (*ppSessionHead == NULL) {
        // ԭ���Ự״̬����Ϊ��
        *ppSessionHead = pSession;
    } else {
        // ��Ϊ��
        (**ppSessionHead).pPre = pSession;
        pSession->pNext = *ppSessionHead;
        *ppSessionHead = pSession;
    }
}

/**
* ɾ��ָ�� session
* ע���ڴ���ͷ�
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
 * ɾ������ Session
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
 * �޸� Session ���ϴ�ͨ��ʱ��
 */
void modifySessionLastReqTime(long lastReqTime, Session *pSession) {
    pSession->lastReqTime = lastReqTime;
}

/**
 * ���� �ͻ��˱�ʶ�� ��ԭ���ĻỰ״̬�����в���
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
