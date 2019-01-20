#include "vxWorks.h"
#include "stdio.h"
#include "stdlib.h"
#include "semLib.h"
#include "taskLib.h"
#include "sysLib.h"

/**
 * [WARNING]
 * ִ��ǰ��ȷ���Ѿ�ִ���˳�ʼ������������ eTimerManagementInit()
 */

#include "TimerManagement.h"
/* �궨�� */
#define STACK_SIZE  2000 /* �����ջ��СΪ2000bytes */

/* ȫ�ֱ��� */
SEM_ID dataSemId;        /* ͬ���ź��� */
int    tidSend;          /* Send Task����id */
int    tidReceive;       /* Receive Task����id */
unsigned long cnt;

/* �������� */
void   progStart(void);
void   taskSend(void);
void   taskReceive(void);
void   SendInit(void);
void   ReceiveInit(void);
void   progStop(void);

/******************************************************************
* progStart - start����
* ���𴴽�Send Task��Receive Task������ʼ����
* RETURNS: N/A
*/
void progStart(void){
	cnt = 0;

	/* �����ź��� */
	dataSemId  = semBCreate(SEM_Q_FIFO, SEM_EMPTY);

	/* �������� */
	tidSend    = taskSpawn ("tSend", 200, 0, STACK_SIZE,
								  (FUNCPTR)taskSend,
							0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	tidReceive = taskSpawn ("tReceive", 220, 0, STACK_SIZE,
								  (FUNCPTR)taskReceive, 
								  0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	return;
}

/******************************************************************
* taskSend - ������������
* ÿ�������ͷ�һ���ź���dataSemId������ģ���յ�����
*/
void taskSend(void){
	/* Receive Task�ĳ�ʼ�� */
	ReceiveInit();
	/* ������Ϣ���У����ڽ��ܶ�ʱ����֪ͨ */
	MSG_Q_ID msgId = msgQCreate(5, 20, MSG_Q_PRIORITY);
	/* ���ڴ洢��ʱ����ID */
	int timerId;
	/* ���ڴ洢TIMEOUT��Ϣ */
	TimerMessage tMessage;
	/* get ��ʱ�� */
	if(eGetTimer(&timerId, TIMER_PERIOD, taskIdSelf(), 1, 0, msgId, 2000)){
		printf("[Error]: get timer!\n");
		return;
	}
	/* start ָ����ʱ�� */
	if(eStartTimer(timerId, 2018, 2000)){
		printf("[Error]: start timer!\n");
		return;
	}
	/* ��ѭ�� */
	while(1){
		int mLen = msgQReceive(msgId, (char *)&tMessage, 20, WAIT_FOREVER);
		if (mLen < 0){
			printf("[Error]: message len!\n");
			return;
		}else{
			if(tMessage.mType != MT_TIMER){
				printf("[Error]: message type!\n");
				return;
			}else{
				/* �ͷ��ź���������ʾ�յ����� */
				semGive(dataSemId);
			}
		}
	}
	return;
}

/******************************************************************
* taskReceive - ������������
* �ȴ��ź���dataSemId�������ж��Ƿ��յ�����
*/
void taskReceive(void){
	/* Receive Task�ĳ�ʼ�� */
	ReceiveInit();

	/* ��ѭ�� */
	while(1){
		/* �ȴ��ź����������ݣ� */
		semTake(dataSemId, WAIT_FOREVER);
		printf("\nReceive a data #%d", cnt++);
	}
	return;
}


/******************************************************************
* SendInit - ��ʼ��Send Task
* ��ʼ��Send Task��
*/
void SendInit(void)	{
	/* ��ʼ������ */
	printf("\nInitial Send Task");
	return;
}

/******************************************************************
* ReceiveInit - ��ʼ��Receive Task
* ��ʼ��Receive Task��
*/
void ReceiveInit(void){
	/* ��ʼ������ */
	printf("\nInitial Receive Task");
	return;
}

/******************************************************************
* progStop - ��ֹ��������
* ɾ���ź�����ɾ������������ֹ�������С�
*/
void progStop(void){
	/* ɾ���ź��� */
	semDelete(dataSemId);

	/* �黹��ʱ�� */
	eKillTaskAllTimer(tidSend);
	
	/* ɾ�������ź��� */
	taskDelete(tidSend);
	taskDelete(tidReceive);

	printf("The End\n");

	return;
}
