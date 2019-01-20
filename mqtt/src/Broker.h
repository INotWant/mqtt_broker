#ifndef MQTT_BROKER_BROKER_H
#define MQTT_BROKER_BROKER_H

#include "Define.h"

#include "Message.h"
#include "SubscribeTree.h"
#include "RetainMessage.h"

/******************************************************************
 *                       Broker ���������
 ******************************************************************/

typedef struct _Broker {
    Session *pSessionHead;                  // �Ự״̬����ͷ
    RetainMessageNode *pRetainMessageHead;  // ������Ϣ����ͷ
    SubscribeTreeNode *pSubscribeTreeRoot;  // ��������
    MSG_Q_ID msgId;                         // broker ���ڽ��ܱ��ģ���Ϣ���Ķ���
} Broker;

/**
 * ������
 */
enum ErrorValueBroker {
    BROKER_ERR_SUCCESS = 0,    // ִ�гɹ�
};

/**
 * Broker ��ʼ��
 */
int initBroker();

/**
 * ���� Broker
 */
Broker *getBroker();

/**
 * ���� Broker
 */
void startBroker();

/**
 * ���� Broker �ı�ʶ��
 */
int getBrokerId();

/**
 * ֹͣ Broker
 */
void stopBroker();

#endif //MQTT_BROKER_BROKER_H
