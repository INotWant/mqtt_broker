#include "vxWorks.h"
#include "stdio.h"
#include "stdlib.h"
#include "semLib.h"
#include "taskLib.h"
#include "sysLib.h"

/**
 * [WARNING]
 * 执行前请确保已经执行了初始化，即调用了 eTimerManagementInit()
 */

#include "TimerManagement.h"
/* 宏定义 */
#define STACK_SIZE  2000 /* 任务堆栈大小为2000bytes */

/* 全局变量 */
SEM_ID dataSemId;        /* 同步信号量 */
int    tidSend;          /* Send Task任务id */
int    tidReceive;       /* Receive Task任务id */
unsigned long cnt;

/* 函数声明 */
void   progStart(void);
void   taskSend(void);
void   taskReceive(void);
void   SendInit(void);
void   ReceiveInit(void);
void   progStop(void);

/******************************************************************
* progStart - start程序
* 负责创建Send Task和Receive Task，并开始运行
* RETURNS: N/A
*/
void progStart(void){
	cnt = 0;

	/* 创建信号量 */
	dataSemId  = semBCreate(SEM_Q_FIFO, SEM_EMPTY);

	/* 创建任务 */
	tidSend    = taskSpawn ("tSend", 200, 0, STACK_SIZE,
								  (FUNCPTR)taskSend,
							0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	tidReceive = taskSpawn ("tReceive", 220, 0, STACK_SIZE,
								  (FUNCPTR)taskReceive, 
								  0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	return;
}

/******************************************************************
* taskSend - 发送数据任务
* 每两秒钟释放一次信号量dataSemId，用来模拟收到数据
*/
void taskSend(void){
	/* Receive Task的初始化 */
	ReceiveInit();
	/* 创建消息队列，用于接受定时器的通知 */
	MSG_Q_ID msgId = msgQCreate(5, 20, MSG_Q_PRIORITY);
	/* 用于存储定时器的ID */
	int timerId;
	/* 用于存储TIMEOUT消息 */
	TimerMessage tMessage;
	/* get 定时器 */
	if(eGetTimer(&timerId, TIMER_PERIOD, taskIdSelf(), 1, 0, msgId, 2000)){
		printf("[Error]: get timer!\n");
		return;
	}
	/* start 指定定时器 */
	if(eStartTimer(timerId, 2018, 2000)){
		printf("[Error]: start timer!\n");
		return;
	}
	/* 主循环 */
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
				/* 释放信号量用来表示收到数据 */
				semGive(dataSemId);
			}
		}
	}
	return;
}

/******************************************************************
* taskReceive - 接收数据任务
* 等待信号量dataSemId，用来判断是否收到数据
*/
void taskReceive(void){
	/* Receive Task的初始化 */
	ReceiveInit();

	/* 主循环 */
	while(1){
		/* 等待信号量（收数据） */
		semTake(dataSemId, WAIT_FOREVER);
		printf("\nReceive a data #%d", cnt++);
	}
	return;
}


/******************************************************************
* SendInit - 初始化Send Task
* 初始化Send Task。
*/
void SendInit(void)	{
	/* 初始化代码 */
	printf("\nInitial Send Task");
	return;
}

/******************************************************************
* ReceiveInit - 初始化Receive Task
* 初始化Receive Task。
*/
void ReceiveInit(void){
	/* 初始化代码 */
	printf("\nInitial Receive Task");
	return;
}

/******************************************************************
* progStop - 终止程序运行
* 删除信号量，删除所以任务，终止程序运行。
*/
void progStop(void){
	/* 删除信号量 */
	semDelete(dataSemId);

	/* 归还定时器 */
	eKillTaskAllTimer(tidSend);
	
	/* 删除所有信号量 */
	taskDelete(tidSend);
	taskDelete(tidReceive);

	printf("The End\n");

	return;
}
