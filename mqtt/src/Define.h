#ifndef MQTT_BROCKER_DEFINE_H
#define MQTT_BROCKER_DEFINE_H

#define boolean UINT8
#define True 1
#define False 0

#define TABLE_SIZE 512        			 // HashTable 的大小

#define MAX_MESSAGE_NUM 100          // broker 同时最大可接受的报文数
#define MAX_MESSAGE_LENGTH 512       // 最大报文长度
// TODO 修改1
#define MAX_SUB_OR_UNSUB 10          // 单条报文允许承载的最大订阅主题数（或取消订阅主题数）
#define PING_TIME 500

#define PUB_MAX_MESSAGE_NUM 5
#define PUB_TASK_PRIO 200
#define PUB_TASK_STACK 1024

#include "vxWorks.h"
#include "sysLib.h"
#include "semLib.h"
#include "msgQLib.h"
#include "taskLib.h"

#include "stdio.h"
#include "stdlib.h"
#include "time.h"

#include "HashTable.h"
#include "memory_management_api.h"
#include "TimerManagement.h"

#include "util.h"

#endif //MQTT_BROCKER_DEFINE_H
