#include "TimerManagement.h"

TimerManagement timerManagement;	// timerManagement
int timerManagementTid;				// timer_management_task 的 ID
SEM_ID tmTiskSemB = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
									// 全局信号量 timer management task

/**
 * 函数原型
 */
void insertToActive(int timerId);
void deleteFromList(int timerId);
void insertToFreeOrStandBy(int timerId, int state);
void countListTimerNum(int *pSum, int state);
void timerManagementTask();
void extendClock();

/******************************************************************
 *                       TimerManagement 相关代码
 ******************************************************************/

/**
 * 初始化
 */
void eTimerManagementInit() {
    for (int i = 0; i < TIMERS_NUM; ++i) {
        Timer *pTimer = &(timerManagement.timers[i]);
        // 初始化每个定时器
        pTimer->preFlag = i - 1;
        if (i == TIMERS_NUM - 1) {
            pTimer->nextFlag = -1;
        } else {
            pTimer->nextFlag = i + 1;
        }
        pTimer->timerId = i;
        pTimer->state = TIMERS_FREE;
    }
    timerManagement.freeFlag = 0;
    timerManagement.standbyFlag = -1;
    timerManagement.activeFlag = -1;
		timerManagement.semM = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
    // 启动管理 Task
		timerManagementTid = taskSpawn ("timer_management_task", TIMERS_MANAGEMENT_TASK_PRIO, 0, TIMERS_MANAGEMENT_TASK_STACK,
        (FUNCPTR)timerManagementTask,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		sysClkConnect((FUNCPTR)extendClock, 0);
		printf("[Info]: init timer management success!\n");
}

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
int eGetTimer(int *pTimerId, UINT8 attr, int taskId, UINT16 instId, UINT16 userTag, MSG_Q_ID messageId, int interval) {
		if (taskId < 0) {
	        return ERR_TIDINV;
	    }
		if (interval < 0) {
			return ERR_INTERVAL;
		}
  	// 加锁
		semTake(timerManagement.semM, WAIT_FOREVER);
    if (-1 == timerManagement.freeFlag) {
    		// 释放
				semGive(timerManagement.semM);
        return ERR_NOTIMER;
    }
    // 维护 free 与 standby 链表
    int timerId = timerManagement.freeFlag;
    // 从 free 中删除（头删）
    deleteFromList(timerId);
    // 插入到 standby（头插）
    insertToFreeOrStandBy(timerId, TIMERS_STANDBY);
    // 释放
		semGive(timerManagement.semM);
    *pTimerId = timerId;
    Timer *pTimer = &timerManagement.timers[timerId];
    pTimer->attr = attr;
    pTimer->taskId = taskId;
    pTimer->instId = instId;
    pTimer->userTag = userTag;
    pTimer->messageId = messageId;
    pTimer->interval = interval;
    pTimer->tickNum = interval * sysClkRateGet() / 1000;
    return ERR_SUCCESS;
}

/**
 * 开启指定定时器
 * @param timerId
 * @param userTag
 * @param interval
 * @return
 */
int eStartTimer(int timerId, UINT16 userTag, int interval) {
	if (interval < 0) {
		return ERR_INTERVAL;
	}
    if (timerId < 0 || timerId >= TIMERS_NUM) {
        return ERR_IDINV;
    }
    if (timerManagement.timers[timerId].state == TIMERS_FREE) {
        return ERR_STATE;
    }
    // 加锁
	semTake(timerManagement.semM, WAIT_FOREVER);
    Timer *pTimer = &timerManagement.timers[timerId];
    if (pTimer->state == TIMERS_STANDBY) {
        // 先从 standby 中删除
        deleteFromList(timerId);
    } else if (pTimer->state == TIMERS_ACTIVE) {
        // 先从 active 中删除
        deleteFromList(timerId);
    }
    pTimer->userTag = userTag;
		pTimer->interval = interval;
    pTimer->tickNum = interval * sysClkRateGet() / 1000;
    // 将此定时器插入到 active 中
    insertToActive(timerId);
    // 释放锁
	semGive(timerManagement.semM);
    return ERR_SUCCESS;
}

/**
 * 停止指定定时器，进入备用状态
 * @param timerId
 * @return
 */
int eStopTimer(int timerId) {
    if (timerId < 0 || timerId >= TIMERS_NUM) {
        return ERR_IDINV;
    }
    if (timerManagement.timers[timerId].state == TIMERS_FREE) {
        return ERR_STATE;
    }
    if (timerManagement.timers[timerId].state == TIMERS_STANDBY) {
        return ERR_SUCCESS;
    }
    // 加锁
	semTake(timerManagement.semM, WAIT_FOREVER);
    // 从 active 中删除
    deleteFromList(timerId);
    // 插入到 standby 中（头插）
    insertToFreeOrStandBy(timerId, TIMERS_STANDBY);
    // 释放
	semGive(timerManagement.semM);
    return ERR_SUCCESS;
}

/**
 * 归还指定定时器
 * @param timerId
 * @return
 */
int eKillTimer(int timerId) {
    if (timerId < 0 || timerId >= TIMERS_NUM) {
        return ERR_IDINV;
    }
    if (timerManagement.timers[timerId].state == TIMERS_FREE) {
        return ERR_SUCCESS;
    }
    // 加锁
	semTake(timerManagement.semM, WAIT_FOREVER);
    // 从 free or standby 链表中删除
    deleteFromList(timerId);
    // 插入到 free 链表中（头插）
    insertToFreeOrStandBy(timerId, TIMERS_FREE);
    // 释放
	semGive(timerManagement.semM);
    return ERR_SUCCESS;
}

/**
 * 停止指定 task 获取的所有定时器
 * @param taskId
 * @return
 */
int eStopTaskAllTimer(int taskId) {
    if (taskId < 0) {
        return ERR_TIDINV;
    }
    // 遍历 active 链表
    // 加锁
	semTake(timerManagement.semM, WAIT_FOREVER);
    int currFlag = timerManagement.activeFlag;
    while (-1 != currFlag) {
        int tempFlag = timerManagement.timers[currFlag].nextFlag;
        if (taskId == timerManagement.timers[currFlag].taskId) {
            // 先从 active 链表中删除
            deleteFromList(currFlag);
            // 添加到 standby 链表中
            insertToFreeOrStandBy(currFlag, TIMERS_STANDBY);
        }
        currFlag = tempFlag;
    }
    // 释放
	semGive(timerManagement.semM);
    return ERR_SUCCESS;
}

/**
 * 归还指定 task 获取的所有定时器
 * @param taskId
 * @return
 */
int eKillTaskAllTimer(int taskId) {
    if (taskId < 0) {
        return ERR_TIDINV;
    }
    // 加锁
    // 遍历 standby
	semTake(timerManagement.semM, WAIT_FOREVER);
    int currFlag = timerManagement.standbyFlag;
    while (-1 != currFlag) {
        int tempFlag = timerManagement.timers[currFlag].nextFlag;
        if (taskId == timerManagement.timers[currFlag].taskId) {
            // 先从 standby 链表中删除
            deleteFromList(currFlag);
            // 添加到 free 链表中
            insertToFreeOrStandBy(currFlag, TIMERS_FREE);
        }
        currFlag = tempFlag;
    }
    // 遍历 active
    currFlag = timerManagement.activeFlag;
    while (-1 != currFlag) {
        int tempFlag = timerManagement.timers[currFlag].nextFlag;
        if (taskId == timerManagement.timers[currFlag].taskId) {
            // 先从 active 链表中删除
            deleteFromList(currFlag);
            // 添加到 free 链表中
            insertToFreeOrStandBy(currFlag, TIMERS_FREE);
        }
        currFlag = tempFlag;
    }
    // 释放
	semGive(timerManagement.semM);
    return ERR_SUCCESS;
}

/**
 * 定时器管理信息输出
 */
void ePrintTimersState(){
	int freeSum = 0;
	int standbySum = 0;
	int activeSum = 0;
	int activeFlag = 0;
	// 加锁
	semTake(timerManagement.semM, WAIT_FOREVER);
	countListTimerNum(&freeSum, TIMERS_FREE);
	countListTimerNum(&standbySum, TIMERS_STANDBY);
	countListTimerNum(&activeSum, TIMERS_ACTIVE);
	activeFlag = timerManagement.activeFlag;
	// 释放
	semGive(timerManagement.semM);
	printf("=====================\n");
	printf("[FreeState]:\t%d\n", freeSum);
	printf("[StandbyState]:\t%d\n", standbySum);
	printf("[ActiveState]:\t%d\n", activeSum);
	printf("[Total Timers]:\t%d\n", (freeSum + standbySum + activeSum));
	printf("[activeFlag]:\t%d\n", activeFlag);
	printf("=====================\n");
}

/******************************************************************
 *                       扩展的定时器 ISR
 ******************************************************************/ 

 void extendClock(){
    tickAnnounce ();	/* announce system tick to kernel */
	// 释放信号量
	semGive(tmTiskSemB);
}

/******************************************************************
 *                       管理 Task 相关代码
 ******************************************************************/ 
 
void timerManagementTask() {
    while (1) {
		    // 等待信号量
				semTake(tmTiskSemB, WAIT_FOREVER);
		    // 刷新
		    // 加锁
				semTake(timerManagement.semM, WAIT_FOREVER);
		    int currFlag = timerManagement.activeFlag;
		    if (-1 != currFlag){
				    timerManagement.timers[currFlag].tickNum -= 1; 
				    while (0 == timerManagement.timers[currFlag].tickNum && -1 != currFlag) {
				        Timer *pTimer = &timerManagement.timers[currFlag];
				        int tempFlag = pTimer->nextFlag;
				        if (TIMER_TIMEOUT == pTimer->attr) {
				            // 超时处理
				            // 先从 active 删除
				            deleteFromList(currFlag);
				            // 再插入到 standby 中（头插）
				            insertToFreeOrStandBy(currFlag, TIMERS_STANDBY);
				        } else if (TIMER_PERIOD == pTimer->attr) {
				            // 再次定时
				            // 先从 active 删除
				            deleteFromList(currFlag);
				            // 重新制定 tickNum
				            pTimer->tickNum = pTimer->interval / (1000 / sysClkRateGet());
				            // 将此定时器插入到 active 中
				            insertToActive(currFlag);
				        }
				        // 使用消息队列发送通知
								TimerMessage tMessage;
								tMessage.mType = MT_TIMER;
								tMessage.mLen = sizeof(TimerMessage);
								tMessage.subTp = pTimer->attr;
								tMessage.userTag = pTimer->userTag;
								tMessage.taskId = pTimer->taskId;
								tMessage.instId = pTimer->instId;
								tMessage.timerId = pTimer->timerId;
								// NO_WAIT!!!
								int errCode = msgQSend(pTimer->messageId, (char*)&tMessage, sizeof(TimerMessage), NO_WAIT, MSG_PRI_NORMAL);
								if(errCode){
										printf("[Error]: Send Fail! [ERR_CODE]: %d\n", errCode);
								}
					      currFlag = tempFlag;
				    }
				}
		    // 释放
				semGive(timerManagement.semM);
    }
}

/******************************************************************
 *                       链表操作相关代码
 ******************************************************************/

/**
 * 将指定定时器插入到 active 中
 * Note: 使用前考虑是否已经获得相应的锁
 */
void insertToActive(int timerId) {
    int currFlag = timerManagement.activeFlag;
    int insertFlag = -1;
    int tickNum = timerManagement.timers[timerId].tickNum;
    if (-1 != currFlag) {
        while (tickNum > timerManagement.timers[currFlag].tickNum) {
            tickNum -= timerManagement.timers[currFlag].tickNum;
            insertFlag = currFlag;
            currFlag = timerManagement.timers[currFlag].nextFlag;
            // 到达最后
            if (-1 == currFlag) {
                break;
            }
        }
    }
    Timer *pTimer = &timerManagement.timers[timerId];
    pTimer->tickNum = tickNum;
    pTimer->state = TIMERS_ACTIVE;
    if (-1 == insertFlag) {
        pTimer->preFlag = -1;
        pTimer->nextFlag = timerManagement.activeFlag;
        timerManagement.activeFlag = timerId;
    } else {
        pTimer->preFlag = insertFlag;
        pTimer->nextFlag = timerManagement.timers[insertFlag].nextFlag;
        timerManagement.timers[insertFlag].nextFlag = timerId;
    }
    if (-1 != pTimer->nextFlag) {
        timerManagement.timers[pTimer->nextFlag].preFlag = timerId;
        timerManagement.timers[pTimer->nextFlag].tickNum -= pTimer->tickNum;
    }
}

/**
 * 将指定定时器从所在链表删除
 * Note: 使用前考虑是否已经获得相应的锁
 */
void deleteFromList(int timerId) {
    Timer *pTimer = &timerManagement.timers[timerId];
    if (-1 == pTimer->preFlag) {
        if (TIMERS_FREE == pTimer->state) {
            timerManagement.freeFlag = pTimer->nextFlag;
        } else if (TIMERS_STANDBY == pTimer->state) {
            timerManagement.standbyFlag = pTimer->nextFlag;
        } else if (TIMERS_ACTIVE == pTimer->state) {
            timerManagement.activeFlag = pTimer->nextFlag;
        }
    } else {
        timerManagement.timers[pTimer->preFlag].nextFlag = pTimer->nextFlag;
    }
    if (-1 != pTimer->nextFlag) {
        timerManagement.timers[pTimer->nextFlag].preFlag = pTimer->preFlag;
        if (TIMERS_ACTIVE == pTimer->state) {
            timerManagement.timers[pTimer->nextFlag].tickNum += pTimer->tickNum;
        }
    }
}

/**
 * 将指定定时器“头”插到 free or standby 链表
 * Note: 使用前考虑是否已经获得相应的锁
 */
void insertToFreeOrStandBy(int timerId, int state) {
    Timer *pTimer = &timerManagement.timers[timerId];
    pTimer->preFlag = -1;
    if (TIMERS_FREE == state) {
        pTimer->nextFlag = timerManagement.freeFlag;
        pTimer->state = TIMERS_FREE;
        if (-1 != timerManagement.freeFlag) {
            timerManagement.timers[timerManagement.freeFlag].preFlag = timerId;
        }
        timerManagement.freeFlag = timerId;
    } else if (TIMERS_STANDBY == state) {
        pTimer->nextFlag = timerManagement.standbyFlag;
        pTimer->state = TIMERS_STANDBY;
        if (-1 != timerManagement.standbyFlag) {
            timerManagement.timers[timerManagement.standbyFlag].preFlag = timerId;
        }
        timerManagement.standbyFlag = timerId;
    }
}

/**
 * 计算指定链表中元素的个数（即，处于指定状态的 定时器 的个数）
 */
void countListTimerNum(int *pSum, int state){
	int currFlag = -1;
	if (TIMERS_FREE == state) {
		currFlag = timerManagement.freeFlag;
	}else if (TIMERS_STANDBY == state){
		currFlag = timerManagement.standbyFlag;
	}else if (TIMERS_ACTIVE == state){
		currFlag = timerManagement.activeFlag;
	}
	int sum = 0;
	while(-1 != currFlag){
		++sum;
		currFlag = timerManagement.timers[currFlag].nextFlag;
	}
	*pSum = sum;
}

