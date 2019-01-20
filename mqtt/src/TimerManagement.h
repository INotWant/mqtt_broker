#ifndef EXTENDTIMER_EXTENDTIMER_H
#define EXTENDTIMER_EXTENDTIMER_H

#include "vxWorks.h"
#include "sysLib.h"
#include "memLib.h"
#include "semLib.h"
#include "taskLib.h"
#include "msgQLib.h"
#include "usrConfig.h"

/* TIMERS_MANAGEMENT ��� */
#define TIMERS_NUM 500
#define TIMERS_FREE 0
#define TIMERS_STANDBY 1
#define TIMERS_ACTIVE 2

/* TIMER ���� */
#define TIMER_TIMEOUT 0
#define TIMER_PERIOD 1

/* TIMERS_MANAGEMENT_TASK ��� */
#define TIMERS_MANAGEMENT_TASK_PRIO 10
#define TIMERS_MANAGEMENT_TASK_STACK 2048

/* ��Ϣ������� */
#define MT_TIMER 1

/**
 * ������ʱ��
 */
typedef struct _Timer {
    int preFlag;    // �˶�ʱ��ǰ��Ķ�ʱ���� timerId
    int nextFlag;   // �˶�ʱ������Ķ�ʱ���� timerId
    int timerId;    // ��ʶ�˶�ʱ��
    UINT8 state;      // �˶�ʱ��������ת̬ free or standby or active
    UINT8 attr;       // �˶�ʱ������� TIMER_TIMEOUT or TIMER_PERIOD
    UINT16 userTag;    // ����ĵ�
    int taskId;     // �˶�ʱ�������� task
    MSG_Q_ID messageId;  // �˶�ʱ��֪ͨʱ���õġ���Ϣ���С��� ID
    int interval;   // �˶�ʱ���ļ������λ ms
    int tickNum;    // �˶�ʱ����ʣ������
	UINT16 instId;     // �˶�ʱ�������� task �ĵ� instId ʵ��
	UINT16 reserved;
} Timer;

/**
 * ��װ���еĶ�ʱ��
 */
typedef struct _TimerManagement {
    Timer timers[TIMERS_NUM];
    int freeFlag;       // Ϊ���� free ת̬�µ�ͷ��ʱ���� timerID
    int standbyFlag;    // Ϊ���� standby ת̬�µ�ͷ��ʱ���� timerID
    int activeFlag;     // Ϊ���� active ת̬�µ�ͷ��ʱ���� timerID
    SEM_ID semM;
} TimerManagement;

/**
 * ��ʱ��֪ͨ task ��Ϣ�����ݽṹ
 */
typedef struct _TimerMessage {
    UINT8 mType;  			// ��Ϣ����
    UINT8 mLen;   			// ��Ϣ����
    UINT8 subTp, recv;  	// ��Ϣ������
    UINT16 userTag;
	UINT16 instId;
    int taskId;
    int timerId;
} TimerMessage;

/**
 * ������
 */
enum ErrorValue {
    ERR_SUCCESS = 0,    // ִ�гɹ�
    ERR_NOTIMER = 1,    // �޿��ö�ʱ��
    ERR_IDINV = 2,      // ���Ϸ��Ķ�ʱ��ID
    ERR_STATE = 3,      // ���Ϸ��Ķ�ʱ��״̬
    ERR_TIDINV = 4,     // ���Ϸ��� taskID
	ERR_INTERVAL = 5,	// ���Ϸ��� ʱ����
};


/**
 * ��ʼ�� eTimer
 */
void eTimerManagementInit();

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
int eGetTimer(int *pTimerId, UINT8 attr, int taskId, UINT16 instId, UINT16 userTag, MSG_Q_ID messageId, int interval);

/**
 * ����ָ����ʱ��
 * @param timerId
 * @param userTag
 * @param interval
 * @return
 */
int eStartTimer(int timerId, UINT16 userTag, int interval);

/**
 * ָֹͣ����ʱ��
 * @param timerId
 * @return
 */
int eStopTimer(int timerId);

/**
 * �黹ָ����ʱ��
 * @param timerId
 * @return
 */
int eKillTimer(int timerId);

/**
 * ָֹͣ�� task ��ȡ�����ж�ʱ��
 * @param taskId
 * @return
 */
int eStopTaskAllTimer(int taskId);

/**
 * �黹ָ�� task ��ȡ�����ж�ʱ��
 * @param taskId
 * @return
 */
int eKillTaskAllTimer(int taskId);

/**
 * ��ʱ��������Ϣ���
 */
void ePrintTimersState();

#endif //EXTENDTIMER_EXTENDTIMER_H
