#ifndef MQTT_BROKER_MESSAGE_H
#define MQTT_BROKER_MESSAGE_H

#include "Define.h"

/**
 * 固定头部
 */
typedef struct _FixedHeader {
    UINT8 messageType;      // 控制报文类型
    char flag;             // 指定控制报文类型的标志位
    UINT16 reserved_;
    int remainLength;       // 剩余长度
} FixedHeader;

/**
 * CONNECT 可变头部
 */
typedef struct _ConnVariableHeader {
    char mqtt[4];                   // 恒为 ‘MQTT’
    UINT16 protocolNameLength;      // 恒等于 4
    UINT8 level;                    // 恒等于 4
    boolean userNameFlag;
    boolean passwordFlag;
    boolean willRetain;
    boolean willFlag;
    UINT8 willQos;
    boolean cleanSession;
    boolean reserved_;
    UINT16 keepAlive;
} ConnVariableHeader;

/**
 * CONNECT 有效载荷
 */
typedef struct _ConnPayload {
    int clientId;       // 客户端标识符（这里即客户端对应的消息队列的ID）
    // 其他的如 遗嘱 用户名 密码等 这里不予支持
} ConnPayload;

/**
 * PUBLISH 可变报头
 */
typedef struct _PublishVariableHeader {
    int topicLength;        // 主题的长度
    char *pTopicHead;       // 主题头指针
    unsigned int messageId;// 报文标识符
} PublishVariableHeader;

/**
 * PUBLISH 有效载荷
 */
typedef struct _PublishPayload {
    int clientId;            // （即客户端的消息队列的 ID）不属于 mqtt 协议范畴，用于恢复
    // 置于有效载荷的最前端（且不计算入剩余长度）
    int contentLength;       // 内容的长度
    char *pContent;           // 应用消息的内容
} PublishPayload;

/**
 * PUBACK or PUBREC or PUBREL or PUBCOMP or SUBSCRIBE or UNSUBSCRIBE 的公共可变报头
 */
typedef struct _CommonVariableHeader {
    unsigned int messageId;// 报文标识符
} CommonVariableHeader;

/**
 * Subscribe 的有效载荷
 */
typedef struct _SubscribePayload {
    int topicFilterLengthArray[MAX_SUB_OR_UNSUB];     // 用于记录每个要订阅主题过滤器的长度
    UINT8 topicFilterQosArray[MAX_SUB_OR_UNSUB];      // 用于记录每个主题过滤器对应的服务质量
    char *pTopicFilterArray[MAX_SUB_OR_UNSUB];        // 用于记录每个主题过滤器对应的起始地址
    int topicFilterNum;                                 // 主题过滤器的总量
    int clientId;                                       // （即客户端的消息队列的 ID）不属于 mqtt 协议范畴，用于恢复
    // 置于有效载荷的最前端（且不计算入剩余长度）
} SubscribePayload;

/**
 * Unsubscribe 的有效载荷
 */
typedef struct _UnsubscribePayload {
    int topicFilterLengthArray[MAX_SUB_OR_UNSUB];     // 用于记录每个要订阅主题过滤器的长度
    char *pTopicFilterArray[MAX_SUB_OR_UNSUB];        // 用于记录每个主题过滤器对应的起始地址
    int topicFilterNum;                                 // 主题过滤器的总量
    int clientId;                                       // （即客户端的消息队列的 ID）不属于 mqtt 协议范畴，用于恢复
    // 置于有效载荷的最前端（且不计算入剩余长度）
} UnsubscribePayload;

/**
 * 应用消息 结构体（主要用于保留消息）
 */
typedef struct _ApplicationMessage {
    int clientId;        // 主要用于不同 客户端 的保留消息
    int qos;               // 应用消息的服务质量
    char *topicName;       // 应用消息的主题名
    char *content;         // 应用消息的内容
} ApplicationMessage;

typedef struct _MessageListNode {
    ApplicationMessage *pApplicationMessage;
    /* 维护保留消息链表 */
    struct _MessageListNode *pNext;
    struct _MessageListNode *pPre;
} MessageListNode;

/**
 * 报文 或称为“消息”
 */
typedef struct _Message {
    FixedHeader *pFixedHeader;  // 固定头部
    void *pVariableHeader;      // 可变头部
    void *pPayload;             // 有效载荷
} Message;

/**
 * 所有报文（消息）控制类型
 */
enum MessageType {
    RESERVED_ = 0,
    CONNECT = 1,
    CONNACK = 2,
    PUBLISH = 3,
    PUBACK = 4,
    PUBREC = 5,
    PUBREL = 6,
    PUBCOMP = 7,
    SUBSCRIBE = 8,
    SUBACK = 9,
    UNSUBSCRIBE = 10,
    UNSUBACK = 11,
    PINGREQ = 12,
    PINGRESP = 13,
    DISCONNECT = 14,
    RESERVED__ = 15,
};


/******************************************************************
 *                       拆包相关声明
 ******************************************************************/

/**
 * 用于读取报文中的所有整型
 */
unsigned int getUnsignedInt(char *pMessage);

/**
 * 拆包：获取固定报头
 * 返回：固定报头所占的字节数
 */
int getFixedHeader(char *pMessage, FixedHeader *pFixedHeader);

/**
 * 获取 CONNECT 的可变报头
 */
int getConnVariableHeader(char *pMessage, ConnVariableHeader *pConnVariableHeader);

/**
 * 获取 CONNECT 的有效载荷
 */
void getConnPayload(char *pMessage, ConnPayload *pConnPayload);

/**
 * 获取 publish 的可变头部
 * 返回：可变报头的长度
 */
int getPublishVariableHeader(char *pMessage, PublishVariableHeader *pPublishVariableHeader, int qos);

/**
 * 获取 publish 的有效载荷
 */
void getPublishPayload(char *pMessage, PublishPayload *pPublishPayload, int remainLength, int variableHeaderLength);

/**
 * 获取 PUBACK or PUBREC or PUBREL or PUBCOMP 可变报头
 * or SUBSCRIBE or UNSUBSCRIBE
 */
void getCommonVariableHeader(char *pMessage, CommonVariableHeader *pCommonVariableHeader);

/**
 * 获取 subscribe 的有效载荷
 */
void getSubscribePayload(char *pMessage, SubscribePayload *pSubscribePayload, int payloadLength);

/**
 * 获取 unsubscribe 的有效载荷
 */
void getUnsubscribePayload(char *pMessage, UnsubscribePayload *pUnsubscribePayload, int payloadLength);

/******************************************************************
 *                       封包相关声明
 ******************************************************************/

/**
 * 封装固定头部
 * 返回：固定报头的长度
 */
int packageFixedHeader(char *pMessage, FixedHeader *pFixedHeader);

/**
 * 封包：通用的可变报头
 */
int packageCommonVariableHeader(char *pMessage, unsigned int messageId);

#endif //MQTT_BROKER_MESSAGE_H
