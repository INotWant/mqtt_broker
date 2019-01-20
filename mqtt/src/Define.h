#ifndef MQTT_BROCKER_DEFINE_H
#define MQTT_BROCKER_DEFINE_H

#define boolean UINT8
#define True 1
#define False 0

#define TABLE_SIZE 512        			 // HashTable �Ĵ�С

#define MAX_MESSAGE_NUM 100          // broker ͬʱ���ɽ��ܵı�����
#define MAX_MESSAGE_LENGTH 512       // ����ĳ���
// TODO �޸�1
#define MAX_SUB_OR_UNSUB 10          // ��������������ص����������������ȡ��������������
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
