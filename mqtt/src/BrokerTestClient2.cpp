#include "Broker.h"
#include "Define.h"

MSG_Q_ID msgIdClient2 = msgQCreate(MAX_MESSAGE_NUM, MAX_MESSAGE_LENGTH, MSG_Q_PRIORITY);

/**
 * 模拟客户端发送 CONNECT 报文
 */
void testConnectClient2() {
    int packageLength = 16;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    MSG_Q_ID brokerMsgId = (MSG_Q_ID) getBrokerId();
    FixedHeader fixedHeader;
    fixedHeader.messageType = CONNECT;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 14;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 封包：可变报头
    *(pMessage + byteNum + 0) = 0x00;
    *(pMessage + byteNum + 1) = 0x04;
    *(pMessage + byteNum + 2) = 0x4D;
    *(pMessage + byteNum + 3) = 0x51;
    *(pMessage + byteNum + 4) = 0x54;
    *(pMessage + byteNum + 5) = 0x54;
    *(pMessage + byteNum + 6) = 0x04;
    *(pMessage + byteNum + 7) = 0xCE;
    *(pMessage + byteNum + 8) = 0x03;
    *(pMessage + byteNum + 9) = 0xe8;
    // 封包：客户端标识符
    int2CharArray((int) msgIdClient2, pMessage + byteNum + 10);
    // 发送 CONNECT
    int errCode = msgQSend(brokerMsgId, pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: Send CONNECT Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: Send CONNECT success!\n");
    }
    // 释放内存
    memory_management_free((void **) (&pMessage), sizeof(packageLength));
    // 申请内存
    memory_management_allocate(MAX_MESSAGE_LENGTH, (void **) &pMessage);
    int mLen = msgQReceive(msgIdClient2, pMessage, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
    if (mLen > 0) {
        printf("[INFO]: Get CONNACK!\n");
    }
    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
}

/**
 * 模拟客户端发送 PUBLISH qos0
 */
void testPublishQos0() {
		testConnectClient2();
    int packageLength = 16;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    MSG_Q_ID brokerMsgId = (MSG_Q_ID) getBrokerId();
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBLISH;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 14;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 可变报头
    *(pMessage + byteNum + 0) = 0x00;
    *(pMessage + byteNum + 1) = 0x03;
    *(pMessage + byteNum + 2) = 0x61;
    *(pMessage + byteNum + 3) = 0x2F;
    *(pMessage + byteNum + 4) = 0x62;
    // 客户端标识
    int2CharArray((int) msgIdClient2, pMessage + byteNum + 5);
    // 有效载荷
    // 0x480x650x6c0x6c0x6f
    *(pMessage + byteNum + 9) = 0x48;
    *(pMessage + byteNum + 10) = 0x65;
    *(pMessage + byteNum + 11) = 0x6c;
    *(pMessage + byteNum + 12) = 0x6c;
    *(pMessage + byteNum + 13) = 0x6f;
    // 发送 SUBSCRIBE
    int errCode = msgQSend(brokerMsgId, pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: Send PUBLISH qos0 Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: Send PUBLISH qos0 success!\n");
    }
    // 释放内存
    memory_management_free((void **) (&pMessage), sizeof(packageLength));
}

/**
 * 模拟客户端发送 PUBLISH qos1
 */
void testPublishQos1() {
		testConnectClient2();
    int packageLength = 18;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    MSG_Q_ID brokerMsgId = (MSG_Q_ID) getBrokerId();
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBLISH;
    fixedHeader.flag = 0x02;
    fixedHeader.remainLength = 16;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 可变报头：主题
    *(pMessage + byteNum + 0) = 0x00;
    *(pMessage + byteNum + 1) = 0x03;
    *(pMessage + byteNum + 2) = 0x61;
    *(pMessage + byteNum + 3) = 0x2F;
    *(pMessage + byteNum + 4) = 0x62;
    // 可变报头：保温标识符
    *(pMessage + byteNum + 5) = 0x01;
    *(pMessage + byteNum + 6) = 0x01;
    // 客户端标识
    int2CharArray((int) msgIdClient2, pMessage + byteNum + 7);
    // 有效载荷
    // 0x480x650x6c0x6c0x6f
    *(pMessage + byteNum + 11) = 0x48;
    *(pMessage + byteNum + 12) = 0x65;
    *(pMessage + byteNum + 13) = 0x6c;
    *(pMessage + byteNum + 14) = 0x6c;
    *(pMessage + byteNum + 15) = 0x6f;
    // 发送 SUBSCRIBE
    int errCode = msgQSend(brokerMsgId, pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: Send PUBLISH qos1 Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: Send PUBLISH qos1 success!\n");
    }
    // 释放内存
    memory_management_free((void **) (&pMessage), sizeof(packageLength));
    // 申请内存
    memory_management_allocate(MAX_MESSAGE_LENGTH, (void **) &pMessage);
		// 等待 Broker 的响应
    int mLen = msgQReceive(msgIdClient2, pMessage, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
    if (mLen > 0) {
        byteNum = getFixedHeader(pMessage, &fixedHeader);
        if (fixedHeader.messageType == PUBACK) {
            printf("[INFO]: Get PUBACK!\n");
            CommonVariableHeader commonVariableHeader;
            getCommonVariableHeader(pMessage + byteNum, &commonVariableHeader);
            if (commonVariableHeader.messageId == 0x0101) {
                printf("[Success]: publish success!\n");
            }
        }
    }
    // 释放内存
    memory_management_free((void **) (&pMessage), sizeof(packageLength));
}

/**
 * 模拟客户端发送 PUBLISH qos2
 */
void testPublishQos2() {
		testConnectClient2();
    int packageLength = 18;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    MSG_Q_ID brokerMsgId = (MSG_Q_ID) getBrokerId();
    FixedHeader fixedHeader;
    fixedHeader.messageType = PUBLISH;
    fixedHeader.flag = 0x04;
    fixedHeader.remainLength = 16;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 可变报头：主题
    *(pMessage + byteNum + 0) = 0x00;
    *(pMessage + byteNum + 1) = 0x03;
    *(pMessage + byteNum + 2) = 0x61;
    *(pMessage + byteNum + 3) = 0x2F;
    *(pMessage + byteNum + 4) = 0x62;
    // 可变报头：保温标识符
    *(pMessage + byteNum + 5) = 0x01;
    *(pMessage + byteNum + 6) = 0x01;
    // 客户端标识
    int2CharArray((int) msgIdClient2, pMessage + byteNum + 7);
    // 有效载荷
    // 0x480x650x6c0x6c0x6f
    *(pMessage + byteNum + 11) = 0x48;
    *(pMessage + byteNum + 12) = 0x65;
    *(pMessage + byteNum + 13) = 0x6c;
    *(pMessage + byteNum + 14) = 0x6c;
    *(pMessage + byteNum + 15) = 0x6f;
    // 发送 SUBSCRIBE
    int errCode = msgQSend(brokerMsgId, pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: Send PUBLISH qos2 Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: Send PUBLISH qos2 success!\n");
    }
    // 释放内存
    memory_management_free((void **) (&pMessage), sizeof(packageLength));
    // 申请内存
    memory_management_allocate(MAX_MESSAGE_LENGTH, (void **) &pMessage);
		// 等待 Broker 的响应
    int mLen = msgQReceive(msgIdClient2, pMessage, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
    if (mLen > 0) {
        byteNum = getFixedHeader(pMessage, &fixedHeader);
        if (fixedHeader.messageType == PUBREC) {
            printf("[INFO]: Get PUBREC!\n");
            CommonVariableHeader commonVariableHeader;
            getCommonVariableHeader(pMessage + byteNum, &commonVariableHeader);
            if (commonVariableHeader.messageId == 0x0101) {
                // 发送 PUBREL
        				char *pSendMessage;
						    // 申请内存
						    memory_management_allocate(4, (void **) &pSendMessage);
						    fixedHeader.messageType = PUBREL;
						    fixedHeader.flag = 0x00;
						    fixedHeader.remainLength = 2;
						    // 封包：固定报头
						    byteNum = packageFixedHeader(pSendMessage, &fixedHeader);
						    // 可变报头
						    byteNum += packageCommonVariableHeader(pSendMessage + byteNum, commonVariableHeader.messageId);
						    // 开始发送
						    int id = 0;
						    charArray2Int(pMessage + 4, &id);
						    int errCode = msgQSend((msg_q *)id, pSendMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
						    if (errCode) {
						        printf("[Error]: Send PUBREL Fail! [ERR_CODE]: %d\n", errCode);
						    } else {
						        printf("[Success]: Send PUBREL success!\n");
						    }
						    // 等待 PUBCOMP
						    while (true) {
							    	mLen = msgQReceive(msgIdClient2, pSendMessage, 4, WAIT_FOREVER);
								    if (mLen > 0) {
								        byteNum = getFixedHeader(pSendMessage, &fixedHeader);
								        if (fixedHeader.messageType == PUBCOMP) {
								            printf("[INFO]: Get PUBCOMP!\n");
								            CommonVariableHeader commonVariableHeader;
								            getCommonVariableHeader(pSendMessage + byteNum, &commonVariableHeader);
								            if (commonVariableHeader.messageId == 0x0101){
								            		printf("[Success]: ack %d qos2 publish!\n", commonVariableHeader.messageId);
								            		break;
								            }else{
								            		printf("[Error]: unack %d qos2 publish!\n", commonVariableHeader.messageId);
								            }
								        }
								    }
						  	}
						    // 释放内存
						    memory_management_free((void **) &pSendMessage, MAX_MESSAGE_LENGTH);
            }
        }
    }
    // 释放内存
    memory_management_free((void **) (&pMessage), sizeof(packageLength));
}

void testDisconnClient2(){
		int packageLength = 6;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    MSG_Q_ID brokerMsgId = (MSG_Q_ID) getBrokerId();
    FixedHeader fixedHeader;
    fixedHeader.messageType = DISCONNECT;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 4;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 封包：客户端标识符
    int2CharArray((int) msgIdClient2, pMessage + byteNum);
		// 发送 DISCONNECT
    int errCode = msgQSend(brokerMsgId, pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: Send DISCONNECT Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: Send DISCONNECT success!\n");
    }
    // 释放内存
    memory_management_free((void **) (&pMessage), sizeof(packageLength));
}
