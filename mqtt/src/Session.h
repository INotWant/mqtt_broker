#ifndef MQTT_BROKER_SESSION_H
#define MQTT_BROKER_SESSION_H

/******************************************************************
 *                       �Ự״̬���������
 ******************************************************************/

typedef struct _SubscribeTreeNode SubscribeTreeNode;

/**
 * ���������㣨���ڻỰ״̬�д洢���ж��ģ�
 */
typedef struct _SubscribeListNode {
    SubscribeTreeNode *pTreeNode;      //  �ڶ������е�λ�ã�����ɾ����
    struct _SubscribeListNode *pNext;
    struct _SubscribeListNode *pPre;
    UINT8 deep;                         // ����������Ĳ���������ʱά����ȣ�
    UINT8 reserved_;
    UINT16 reserved__;
} SubscribeListNode;

/**
 * ��������������
 * ע��ʹ�� mm ��ȡ�ڴ�
 */
SubscribeListNode *createSubscribeListNode(UINT8 deep);

/**
 * ���뵽��������
 */
void insertSubscribeListNode(SubscribeListNode **ppHead, SubscribeListNode *pSubscribeListNode);

/**
 * ɾ������������
 */
void deleteSubscribeListNode(SubscribeListNode *pSubscribeListNode, SubscribeListNode **ppSubscribeListHead);

/**
 *  ɾ����������
 */
void deleteSubscribeList(SubscribeListNode *pSubscribeListHead);

/**
 * �Ự״̬
 */
typedef struct _Session {
    int clientId;                   // �ͻ��˱�ʶ�������ڿͻ�������ͨ�ŵ���Ϣ���� ID
    boolean cleanSession;           // ��������Ự��False ����Ҫ����ûỰ״̬
    boolean isOnline;                       // �����ж��Ƿ�����
    UINT16 pingTime;                // ����ʱ��
    long lastReqTime;                // �ϴ�����ʱ��
    SubscribeListNode *pHead;       // ��������ͷָ�룬�������ĻỰɾ�� (����ͷ�壬����ɾ���ỰʱҪ����)
    // TODO �ݲ�ʵ�֣�������ʵ��Ҫ����ɾ���Ȳ������Է��ڴ�й¶
    // MessageListNode *pOutlineMessageHead;   // ���� outline ��� qos1/2 ����Ϣ
    /* ����ά�� �Ự״̬���� */
    HashTable *pMessageIdHashTable;         // ���� qos2 messageID ����
    SEM_ID semM;                            // ���ڶ� pMessageIdHashTable �ķ��ʿ���
    struct _Session *pNext;
    struct _Session *pPre;
} Session;

/**
 * ��ȡָ�� session ��״̬������ ���� ���� ����
 * ע���Ƚϣ���ǰʱ�� - �ϴ�����ʱ�䣩�� ��pingTime * 1.5��
 * @return True ������ ���ߣ����� False �����ѶϿ����ӡ�
 */
boolean isInline(Session *pSession);

/**
 * �����µĿͻ������ӵ�������Ҫʱ����һ���µ� �Ự״̬
 * ע��ʹ�� mm ��ȡ�ڴ�
 */
Session *createSession(int clientId, boolean cleanSession, UINT16 pingTime);

/**
 * ���� Session ���뵽 �Ự״̬������
 */
void insertSession(Session *pSession, Session **ppSessionHead);

/**
 * ɾ��ָ�� session
 * ע���ڴ���ͷ�
 */
void deleteSession(SubscribeTreeNode *pSubscribeTreeRoot, Session **ppSessionHead, Session *pSession);

/**
 * ɾ������ Session
 */
void deleteAllSession(Session *pSessionHead);

/**
 * �޸� Session ���ϴ�ͨ��ʱ��
 */
void modifySessionLastReqTime(long lastReqTime, Session *pSession);

/**
 * ���� �ͻ��˱�ʶ�� ��ԭ���ĻỰ״̬�����в���
 */
Session *findSession(int clientId, Session *pSessionHead);


#endif //MQTT_BROKER_SESSION_H
