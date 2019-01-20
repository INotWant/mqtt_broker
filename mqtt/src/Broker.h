#ifndef MQTT_BROKER_BROKER_H
#define MQTT_BROKER_BROKER_H

#include "Define.h"

#include "Message.h"
#include "SubscribeTree.h"
#include "RetainMessage.h"

/******************************************************************
 *                       Broker 的相关声明
 ******************************************************************/

typedef struct _Broker {
    Session *pSessionHead;                  // 会话状态链表头
    RetainMessageNode *pRetainMessageHead;  // 保留消息链表头
    SubscribeTreeNode *pSubscribeTreeRoot;  // 订阅树根
    MSG_Q_ID msgId;                         // broker 用于接受报文（消息）的队列
} Broker;

/**
 * 错误码
 */
enum ErrorValueBroker {
    BROKER_ERR_SUCCESS = 0,    // 执行成功
};

/**
 * Broker 初始化
 */
int initBroker();

/**
 * 返回 Broker
 */
Broker *getBroker();

/**
 * 启动 Broker
 */
void startBroker();

/**
 * 返回 Broker 的标识符
 */
int getBrokerId();

/**
 * 停止 Broker
 */
void stopBroker();

#endif //MQTT_BROKER_BROKER_H
