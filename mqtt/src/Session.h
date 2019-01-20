#ifndef MQTT_BROKER_SESSION_H
#define MQTT_BROKER_SESSION_H

/******************************************************************
 *                       会话状态的相关声明
 ******************************************************************/

typedef struct _SubscribeTreeNode SubscribeTreeNode;

/**
 * 订阅链表结点（用于会话状态中存储所有订阅）
 */
typedef struct _SubscribeListNode {
    SubscribeTreeNode *pTreeNode;      //  在订阅树中的位置（用于删除）
    struct _SubscribeListNode *pNext;
    struct _SubscribeListNode *pPre;
    UINT8 deep;                         // 主题过滤器的层数（插入时维护深度）
    UINT8 reserved_;
    UINT16 reserved__;
} SubscribeListNode;

/**
 * 创建订阅链表结点
 * 注：使用 mm 获取内存
 */
SubscribeListNode *createSubscribeListNode(UINT8 deep);

/**
 * 插入到订阅链表
 */
void insertSubscribeListNode(SubscribeListNode **ppHead, SubscribeListNode *pSubscribeListNode);

/**
 * 删除订阅链表结点
 */
void deleteSubscribeListNode(SubscribeListNode *pSubscribeListNode, SubscribeListNode **ppSubscribeListHead);

/**
 *  删除订阅链表
 */
void deleteSubscribeList(SubscribeListNode *pSubscribeListHead);

/**
 * 会话状态
 */
typedef struct _Session {
    int clientId;                   // 客户端标识符，等于客户端用于通信的消息队列 ID
    boolean cleanSession;           // 设置清理会话，False 表明要保存该会话状态
    boolean isOnline;                       // 用于判断是否在线
    UINT16 pingTime;                // 心跳时间
    long lastReqTime;                // 上次请求时间
    SubscribeListNode *pHead;       // 订阅链表头指针，方便最后的会话删除 (插入头插，但在删除会话时要排序)
    // TODO 暂不实现，若后续实现要考虑删除等操作，以防内存泄露
    // MessageListNode *pOutlineMessageHead;   // 保存 outline 后的 qos1/2 的消息
    /* 用于维护 会话状态链表 */
    HashTable *pMessageIdHashTable;         // 用于 qos2 messageID 保存
    SEM_ID semM;                            // 用于对 pMessageIdHashTable 的访问控制
    struct _Session *pNext;
    struct _Session *pPre;
} Session;

/**
 * 获取指定 session 的状态，返回 在线 或者 离线
 * 注：比较（当前时间 - 上次请求时间）与 （pingTime * 1.5）
 * @return True 表明仍 在线，返回 False 表明已断开连接。
 */
boolean isInline(Session *pSession);

/**
 * 当有新的客户端连接到来，必要时创建一个新的 会话状态
 * 注：使用 mm 获取内存
 */
Session *createSession(int clientId, boolean cleanSession, UINT16 pingTime);

/**
 * 将新 Session 插入到 会话状态链表中
 */
void insertSession(Session *pSession, Session **ppSessionHead);

/**
 * 删除指定 session
 * 注：内存的释放
 */
void deleteSession(SubscribeTreeNode *pSubscribeTreeRoot, Session **ppSessionHead, Session *pSession);

/**
 * 删除所有 Session
 */
void deleteAllSession(Session *pSessionHead);

/**
 * 修改 Session 的上次通信时间
 */
void modifySessionLastReqTime(long lastReqTime, Session *pSession);

/**
 * 根据 客户端标识符 从原来的会话状态链表中查找
 */
Session *findSession(int clientId, Session *pSessionHead);


#endif //MQTT_BROKER_SESSION_H
