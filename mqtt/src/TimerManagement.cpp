#include "TimerManagement.h"

TimerManagement timerManagement;	// timerManagement
int timerManagementTid;				// timer_management_task �� ID
SEM_ID tmTiskSemB = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
									// ȫ���ź��� timer management task

/**
 * ����ԭ��
 */
void insertToActive(int timerId);
void deleteFromList(int timerId);
void insertToFreeOrStandBy(int timerId, int state);
void countListTimerNum(int *pSum, int state);
void timerManagementTask();
void extendClock();

/******************************************************************
 *                       TimerManagement ��ش���
 ******************************************************************/

/**
 * ��ʼ��
 */
void eTimerManagementInit() {
    for (int i = 0; i < TIMERS_NUM; ++i) {
        Timer *pTimer = &(timerManagement.timers[i]);
        // ��ʼ��ÿ����ʱ��
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
    // �������� Task
		timerManagementTid = taskSpawn ("timer_management_task", TIMERS_MANAGEMENT_TASK_PRIO, 0, TIMERS_MANAGEMENT_TASK_STACK,
        (FUNCPTR)timerManagementTask,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		sysClkConnect((FUNCPTR)extendClock, 0);
		printf("[Info]: init timer management success!\n");
}

/**
 * ��ȡһ����ʱ��
 * @param pTimerId (���ز���)
 * @param attr
 * @param taskId
 * @param instId
 * @param userTag
 * @param messageId
 * @param interval
 * @return ������
 */
int eGetTimer(int *pTimerId, UINT8 attr, int taskId, UINT16 instId, UINT16 userTag, MSG_Q_ID messageId, int interval) {
		if (taskId < 0) {
	        return ERR_TIDINV;
	    }
		if (interval < 0) {
			return ERR_INTERVAL;
		}
  	// ����
		semTake(timerManagement.semM, WAIT_FOREVER);
    if (-1 == timerManagement.freeFlag) {
    		// �ͷ�
				semGive(timerManagement.semM);
        return ERR_NOTIMER;
    }
    // ά�� free �� standby ����
    int timerId = timerManagement.freeFlag;
    // �� free ��ɾ����ͷɾ��
    deleteFromList(timerId);
    // ���뵽 standby��ͷ�壩
    insertToFreeOrStandBy(timerId, TIMERS_STANDBY);
    // �ͷ�
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
 * ����ָ����ʱ��
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
    // ����
	semTake(timerManagement.semM, WAIT_FOREVER);
    Timer *pTimer = &timerManagement.timers[timerId];
    if (pTimer->state == TIMERS_STANDBY) {
        // �ȴ� standby ��ɾ��
        deleteFromList(timerId);
    } else if (pTimer->state == TIMERS_ACTIVE) {
        // �ȴ� active ��ɾ��
        deleteFromList(timerId);
    }
    pTimer->userTag = userTag;
		pTimer->interval = interval;
    pTimer->tickNum = interval * sysClkRateGet() / 1000;
    // ���˶�ʱ�����뵽 active ��
    insertToActive(timerId);
    // �ͷ���
	semGive(timerManagement.semM);
    return ERR_SUCCESS;
}

/**
 * ָֹͣ����ʱ�������뱸��״̬
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
    // ����
	semTake(timerManagement.semM, WAIT_FOREVER);
    // �� active ��ɾ��
    deleteFromList(timerId);
    // ���뵽 standby �У�ͷ�壩
    insertToFreeOrStandBy(timerId, TIMERS_STANDBY);
    // �ͷ�
	semGive(timerManagement.semM);
    return ERR_SUCCESS;
}

/**
 * �黹ָ����ʱ��
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
    // ����
	semTake(timerManagement.semM, WAIT_FOREVER);
    // �� free or standby ������ɾ��
    deleteFromList(timerId);
    // ���뵽 free �����У�ͷ�壩
    insertToFreeOrStandBy(timerId, TIMERS_FREE);
    // �ͷ�
	semGive(timerManagement.semM);
    return ERR_SUCCESS;
}

/**
 * ָֹͣ�� task ��ȡ�����ж�ʱ��
 * @param taskId
 * @return
 */
int eStopTaskAllTimer(int taskId) {
    if (taskId < 0) {
        return ERR_TIDINV;
    }
    // ���� active ����
    // ����
	semTake(timerManagement.semM, WAIT_FOREVER);
    int currFlag = timerManagement.activeFlag;
    while (-1 != currFlag) {
        int tempFlag = timerManagement.timers[currFlag].nextFlag;
        if (taskId == timerManagement.timers[currFlag].taskId) {
            // �ȴ� active ������ɾ��
            deleteFromList(currFlag);
            // ��ӵ� standby ������
            insertToFreeOrStandBy(currFlag, TIMERS_STANDBY);
        }
        currFlag = tempFlag;
    }
    // �ͷ�
	semGive(timerManagement.semM);
    return ERR_SUCCESS;
}

/**
 * �黹ָ�� task ��ȡ�����ж�ʱ��
 * @param taskId
 * @return
 */
int eKillTaskAllTimer(int taskId) {
    if (taskId < 0) {
        return ERR_TIDINV;
    }
    // ����
    // ���� standby
	semTake(timerManagement.semM, WAIT_FOREVER);
    int currFlag = timerManagement.standbyFlag;
    while (-1 != currFlag) {
        int tempFlag = timerManagement.timers[currFlag].nextFlag;
        if (taskId == timerManagement.timers[currFlag].taskId) {
            // �ȴ� standby ������ɾ��
            deleteFromList(currFlag);
            // ��ӵ� free ������
            insertToFreeOrStandBy(currFlag, TIMERS_FREE);
        }
        currFlag = tempFlag;
    }
    // ���� active
    currFlag = timerManagement.activeFlag;
    while (-1 != currFlag) {
        int tempFlag = timerManagement.timers[currFlag].nextFlag;
        if (taskId == timerManagement.timers[currFlag].taskId) {
            // �ȴ� active ������ɾ��
            deleteFromList(currFlag);
            // ��ӵ� free ������
            insertToFreeOrStandBy(currFlag, TIMERS_FREE);
        }
        currFlag = tempFlag;
    }
    // �ͷ�
	semGive(timerManagement.semM);
    return ERR_SUCCESS;
}

/**
 * ��ʱ��������Ϣ���
 */
void ePrintTimersState(){
	int freeSum = 0;
	int standbySum = 0;
	int activeSum = 0;
	int activeFlag = 0;
	// ����
	semTake(timerManagement.semM, WAIT_FOREVER);
	countListTimerNum(&freeSum, TIMERS_FREE);
	countListTimerNum(&standbySum, TIMERS_STANDBY);
	countListTimerNum(&activeSum, TIMERS_ACTIVE);
	activeFlag = timerManagement.activeFlag;
	// �ͷ�
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
 *                       ��չ�Ķ�ʱ�� ISR
 ******************************************************************/ 

 void extendClock(){
    tickAnnounce ();	/* announce system tick to kernel */
	// �ͷ��ź���
	semGive(tmTiskSemB);
}

/******************************************************************
 *                       ���� Task ��ش���
 ******************************************************************/ 
 
void timerManagementTask() {
    while (1) {
		    // �ȴ��ź���
				semTake(tmTiskSemB, WAIT_FOREVER);
		    // ˢ��
		    // ����
				semTake(timerManagement.semM, WAIT_FOREVER);
		    int currFlag = timerManagement.activeFlag;
		    if (-1 != currFlag){
				    timerManagement.timers[currFlag].tickNum -= 1; 
				    while (0 == timerManagement.timers[currFlag].tickNum && -1 != currFlag) {
				        Timer *pTimer = &timerManagement.timers[currFlag];
				        int tempFlag = pTimer->nextFlag;
				        if (TIMER_TIMEOUT == pTimer->attr) {
				            // ��ʱ����
				            // �ȴ� active ɾ��
				            deleteFromList(currFlag);
				            // �ٲ��뵽 standby �У�ͷ�壩
				            insertToFreeOrStandBy(currFlag, TIMERS_STANDBY);
				        } else if (TIMER_PERIOD == pTimer->attr) {
				            // �ٴζ�ʱ
				            // �ȴ� active ɾ��
				            deleteFromList(currFlag);
				            // �����ƶ� tickNum
				            pTimer->tickNum = pTimer->interval / (1000 / sysClkRateGet());
				            // ���˶�ʱ�����뵽 active ��
				            insertToActive(currFlag);
				        }
				        // ʹ����Ϣ���з���֪ͨ
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
		    // �ͷ�
				semGive(timerManagement.semM);
    }
}

/******************************************************************
 *                       ���������ش���
 ******************************************************************/

/**
 * ��ָ����ʱ�����뵽 active ��
 * Note: ʹ��ǰ�����Ƿ��Ѿ������Ӧ����
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
            // �������
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
 * ��ָ����ʱ������������ɾ��
 * Note: ʹ��ǰ�����Ƿ��Ѿ������Ӧ����
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
 * ��ָ����ʱ����ͷ���嵽 free or standby ����
 * Note: ʹ��ǰ�����Ƿ��Ѿ������Ӧ����
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
 * ����ָ��������Ԫ�صĸ�������������ָ��״̬�� ��ʱ�� �ĸ�����
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

