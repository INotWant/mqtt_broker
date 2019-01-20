#ifndef EXTENDTIMER_EXTENDTIMER_H
#define EXTENDTIMER_EXTENDTIMER_H

#include "vxWorks.h"
#include "sysLib.h"
#include "memLib.h"
#include "semLib.h"
#include "taskLib.h"
#include "msgQLib.h"
#include "usrConfig.h"

/* TIMERS_MANAGEMENT 相关 */
#define TIMERS_NUM 500
#define TIMERS_FREE 0
#define TIMERS_STANDBY 1
#define TIMERS_ACTIVE 2

/* TIMER 类型 */
#define TIMER_TIMEOUT 0
#define TIMER_PERIOD 1

/* TIMERS_MANAGEMENT_TASK 相关 */
#define TIMERS_MANAGEMENT_TASK_PRIO 10
#define TIMERS_MANAGEMENT_TASK_STACK 2048

/* 消息类型相关 */
#define MT_TIMER 1

/**
 * 描述定时器
 */
typedef struct _Timer {
    int preFlag;    // 此定时器前面的定时器的 timerId
    int nextFlag;   // 此定时器后面的定时器的 timerId
    int timerId;    // 标识此定时器
    UINT8 state;      // 此定时器所处的转态 free or standby or active
    UINT8 attr;       // 此定时器的类别 TIMER_TIMEOUT or TIMER_PERIOD
    UINT16 userTag;    // 详见文档
    int taskId;     // 此定时器所属的 task
    MSG_Q_ID messageId;  // 此定时器通知时所用的“消息队列”的 ID
    int interval;   // 此定时器的间隔，单位 ms
    int tickNum;    // 此定时器所剩心跳数
	UINT16 instId;     // 此定时器诶所属 task 的第 instId 实例
	UINT16 reserved;
} Timer;

/**
 * 封装所有的定时器
 */
typedef struct _TimerManagement {
    Timer timers[TIMERS_NUM];
    int freeFlag;       // 为所有 free 转态下的头定时器的 timerID
    int standbyFlag;    // 为所有 standby 转态下的头定时器的 timerID
    int activeFlag;     // 为所有 active 转态下的头定时器的 timerID
    SEM_ID semM;
} TimerManagement;

/**
 * 定时器通知 task 消息的数据结构
 */
typedef struct _TimerMessage {
    UINT8 mType;  			// 消息类型
    UINT8 mLen;   			// 消息长度
    UINT8 subTp, recv;  	// 消息子类型
    UINT16 userTag;
	UINT16 instId;
    int taskId;
    int timerId;
} TimerMessage;

/**
 * 错误码
 */
enum ErrorValue {
    ERR_SUCCESS = 0,    // 执行成功
    ERR_NOTIMER = 1,    // 无可用定时器
    ERR_IDINV = 2,      // 不合法的定时器ID
    ERR_STATE = 3,      // 不合法的定时器状态
    ERR_TIDINV = 4,     // 不合法的 taskID
	ERR_INTERVAL = 5,	// 不合法的 时间间隔
};


/**
 * 初始化 eTimer
 */
void eTimerManagementInit();

/**
 * 获取一个定时器
 * @param pTimerId (返回参数)
 * @param attr
 * @param taskId
 * @param instId
 * @param userTag
 * @param messageId
 * @param interval
 * @return 错误码
 */
int eGetTimer(int *pTimerId, UINT8 attr, int taskId, UINT16 instId, UINT16 userTag, MSG_Q_ID messageId, int interval);

/**
 * 开启指定定时器
 * @param timerId
 * @param userTag
 * @param interval
 * @return
 */
int eStartTimer(int timerId, UINT16 userTag, int interval);

/**
 * 停止指定定时器
 * @param timerId
 * @return
 */
int eStopTimer(int timerId);

/**
 * 归还指定定时器
 * @param timerId
 * @return
 */
int eKillTimer(int timerId);

/**
 * 停止指定 task 获取的所有定时器
 * @param taskId
 * @return
 */
int eStopTaskAllTimer(int taskId);

/**
 * 归还指定 task 获取的所有定时器
 * @param taskId
 * @return
 */
int eKillTaskAllTimer(int taskId);

/**
 * 定时器管理信息输出
 */
void ePrintTimersState();

#endif //EXTENDTIMER_EXTENDTIMER_H
