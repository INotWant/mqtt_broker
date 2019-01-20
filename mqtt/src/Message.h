#ifndef MQTT_BROKER_MESSAGE_H
#define MQTT_BROKER_MESSAGE_H

#include "Define.h"

/**
 * �̶�ͷ��
 */
typedef struct _FixedHeader {
    UINT8 messageType;      // ���Ʊ�������
    char flag;             // ָ�����Ʊ������͵ı�־λ
    UINT16 reserved_;
    int remainLength;       // ʣ�೤��
} FixedHeader;

/**
 * CONNECT �ɱ�ͷ��
 */
typedef struct _ConnVariableHeader {
    char mqtt[4];                   // ��Ϊ ��MQTT��
    UINT16 protocolNameLength;      // ����� 4
    UINT8 level;                    // ����� 4
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
 * CONNECT ��Ч�غ�
 */
typedef struct _ConnPayload {
    int clientId;       // �ͻ��˱�ʶ�������Ｔ�ͻ��˶�Ӧ����Ϣ���е�ID��
    // �������� ���� �û��� ����� ���ﲻ��֧��
} ConnPayload;

/**
 * PUBLISH �ɱ䱨ͷ
 */
typedef struct _PublishVariableHeader {
    int topicLength;        // ����ĳ���
    char *pTopicHead;       // ����ͷָ��
    unsigned int messageId;// ���ı�ʶ��
} PublishVariableHeader;

/**
 * PUBLISH ��Ч�غ�
 */
typedef struct _PublishPayload {
    int clientId;            // �����ͻ��˵���Ϣ���е� ID�������� mqtt Э�鷶�룬���ڻָ�
    // ������Ч�غɵ���ǰ�ˣ��Ҳ�������ʣ�೤�ȣ�
    int contentLength;       // ���ݵĳ���
    char *pContent;           // Ӧ����Ϣ������
} PublishPayload;

/**
 * PUBACK or PUBREC or PUBREL or PUBCOMP or SUBSCRIBE or UNSUBSCRIBE �Ĺ����ɱ䱨ͷ
 */
typedef struct _CommonVariableHeader {
    unsigned int messageId;// ���ı�ʶ��
} CommonVariableHeader;

/**
 * Subscribe ����Ч�غ�
 */
typedef struct _SubscribePayload {
    int topicFilterLengthArray[MAX_SUB_OR_UNSUB];     // ���ڼ�¼ÿ��Ҫ��������������ĳ���
    UINT8 topicFilterQosArray[MAX_SUB_OR_UNSUB];      // ���ڼ�¼ÿ�������������Ӧ�ķ�������
    char *pTopicFilterArray[MAX_SUB_OR_UNSUB];        // ���ڼ�¼ÿ�������������Ӧ����ʼ��ַ
    int topicFilterNum;                                 // ���������������
    int clientId;                                       // �����ͻ��˵���Ϣ���е� ID�������� mqtt Э�鷶�룬���ڻָ�
    // ������Ч�غɵ���ǰ�ˣ��Ҳ�������ʣ�೤�ȣ�
} SubscribePayload;

/**
 * Unsubscribe ����Ч�غ�
 */
typedef struct _UnsubscribePayload {
    int topicFilterLengthArray[MAX_SUB_OR_UNSUB];     // ���ڼ�¼ÿ��Ҫ��������������ĳ���
    char *pTopicFilterArray[MAX_SUB_OR_UNSUB];        // ���ڼ�¼ÿ�������������Ӧ����ʼ��ַ
    int topicFilterNum;                                 // ���������������
    int clientId;                                       // �����ͻ��˵���Ϣ���е� ID�������� mqtt Э�鷶�룬���ڻָ�
    // ������Ч�غɵ���ǰ�ˣ��Ҳ�������ʣ�೤�ȣ�
} UnsubscribePayload;

/**
 * Ӧ����Ϣ �ṹ�壨��Ҫ���ڱ�����Ϣ��
 */
typedef struct _ApplicationMessage {
    int clientId;        // ��Ҫ���ڲ�ͬ �ͻ��� �ı�����Ϣ
    int qos;               // Ӧ����Ϣ�ķ�������
    char *topicName;       // Ӧ����Ϣ��������
    char *content;         // Ӧ����Ϣ������
} ApplicationMessage;

typedef struct _MessageListNode {
    ApplicationMessage *pApplicationMessage;
    /* ά��������Ϣ���� */
    struct _MessageListNode *pNext;
    struct _MessageListNode *pPre;
} MessageListNode;

/**
 * ���� ���Ϊ����Ϣ��
 */
typedef struct _Message {
    FixedHeader *pFixedHeader;  // �̶�ͷ��
    void *pVariableHeader;      // �ɱ�ͷ��
    void *pPayload;             // ��Ч�غ�
} Message;

/**
 * ���б��ģ���Ϣ����������
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
 *                       ����������
 ******************************************************************/

/**
 * ���ڶ�ȡ�����е���������
 */
unsigned int getUnsignedInt(char *pMessage);

/**
 * �������ȡ�̶���ͷ
 * ���أ��̶���ͷ��ռ���ֽ���
 */
int getFixedHeader(char *pMessage, FixedHeader *pFixedHeader);

/**
 * ��ȡ CONNECT �Ŀɱ䱨ͷ
 */
int getConnVariableHeader(char *pMessage, ConnVariableHeader *pConnVariableHeader);

/**
 * ��ȡ CONNECT ����Ч�غ�
 */
void getConnPayload(char *pMessage, ConnPayload *pConnPayload);

/**
 * ��ȡ publish �Ŀɱ�ͷ��
 * ���أ��ɱ䱨ͷ�ĳ���
 */
int getPublishVariableHeader(char *pMessage, PublishVariableHeader *pPublishVariableHeader, int qos);

/**
 * ��ȡ publish ����Ч�غ�
 */
void getPublishPayload(char *pMessage, PublishPayload *pPublishPayload, int remainLength, int variableHeaderLength);

/**
 * ��ȡ PUBACK or PUBREC or PUBREL or PUBCOMP �ɱ䱨ͷ
 * or SUBSCRIBE or UNSUBSCRIBE
 */
void getCommonVariableHeader(char *pMessage, CommonVariableHeader *pCommonVariableHeader);

/**
 * ��ȡ subscribe ����Ч�غ�
 */
void getSubscribePayload(char *pMessage, SubscribePayload *pSubscribePayload, int payloadLength);

/**
 * ��ȡ unsubscribe ����Ч�غ�
 */
void getUnsubscribePayload(char *pMessage, UnsubscribePayload *pUnsubscribePayload, int payloadLength);

/******************************************************************
 *                       ����������
 ******************************************************************/

/**
 * ��װ�̶�ͷ��
 * ���أ��̶���ͷ�ĳ���
 */
int packageFixedHeader(char *pMessage, FixedHeader *pFixedHeader);

/**
 * �����ͨ�õĿɱ䱨ͷ
 */
int packageCommonVariableHeader(char *pMessage, unsigned int messageId);

#endif //MQTT_BROKER_MESSAGE_H
