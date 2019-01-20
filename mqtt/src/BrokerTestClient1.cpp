#include "Broker.h"
#include "Define.h"

MSG_Q_ID msgIdClient1 = msgQCreate(MAX_MESSAGE_NUM, MAX_MESSAGE_LENGTH, MSG_Q_PRIORITY);

/**
 * 测试初始化
 */
void testInit(){
		initBroker();
    startBroker();
}

/**
 * 模拟客户端发送 CONNECT 报文
 */
void testConnectClient1() {
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
    int2CharArray((int) msgIdClient1, pMessage + byteNum + 10);
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
    int mLen = msgQReceive(msgIdClient1, pMessage, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
    if (mLen > 0) {
        printf("[INFO]: Get CONNACK!\n");
    }
    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
}

/**
 * 模拟客户端发布订阅
 */
void testSubscribe() {
		testConnectClient1();
    int packageLength = 14;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    MSG_Q_ID brokerMsgId = (MSG_Q_ID) getBrokerId();
    FixedHeader fixedHeader;
    fixedHeader.messageType = SUBSCRIBE;
    fixedHeader.flag = 0x02;
    fixedHeader.remainLength = 12;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 可变报头
    *(pMessage + byteNum + 0) = 0x00;
    *(pMessage + byteNum + 1) = 0x01;
    // 客户端标识
    int2CharArray((int) msgIdClient1, pMessage + byteNum + 2);
    // 有效载荷
    *(pMessage + byteNum + 6) = 0x00;
    *(pMessage + byteNum + 7) = 0x03;
    *(pMessage + byteNum + 8) = 0x61;
    *(pMessage + byteNum + 9) = 0x2F;
    *(pMessage + byteNum + 10) = 0x62;
    *(pMessage + byteNum + 11) = 0x02;
    // 发送 SUBSCRIBE
    int errCode = msgQSend(brokerMsgId, pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: Send SUBSCRIBE Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: Send SUBSCRIBE success!\n");
    }
    // 释放内存
    memory_management_free((void **) (&pMessage), sizeof(packageLength));
    // 申请内存
    memory_management_allocate(MAX_MESSAGE_LENGTH, (void **) &pMessage);
    // 等待响应
    int mLen = msgQReceive(msgIdClient1, pMessage, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
    if (mLen > 0) {
        byteNum = getFixedHeader(pMessage, &fixedHeader);
        if (fixedHeader.messageType == SUBACK) {
            printf("[INFO]: Get SUBACK!\n");
            CommonVariableHeader commonVariableHeader;
            getCommonVariableHeader(pMessage + byteNum, &commonVariableHeader);
            byteNum += 2;
            if (commonVariableHeader.messageId == 0x01) {
                printf("[INFO]: success messageId of SUBACK!\n");
            }
            if (*(pMessage + byteNum) == 0x02) {
                printf("[INFO]: success subscribe!\n");
            }
        }
    } else {
    		printf("[ERROR]: receive SUBACK failed!\n");
    }
    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
    // 等待接受订阅的内容
		// 申请内存
    memory_management_allocate(MAX_MESSAGE_LENGTH, (void **) &pMessage);
    mLen = msgQReceive(msgIdClient1, pMessage, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
    if (mLen > 0) {
        byteNum = getFixedHeader(pMessage, &fixedHeader);
        if (fixedHeader.messageType == PUBLISH) {
            int qos = getCharSpecialBit(&fixedHeader.flag, 2) * 2;
            qos += getCharSpecialBit(&fixedHeader.flag, 1);
            printf("[INFO]: Get a subscribed content!\n");
            PublishVariableHeader publishVariableHeader;
            int variableHeaderLength = getPublishVariableHeader(pMessage + byteNum, &publishVariableHeader, qos);
            byteNum += variableHeaderLength;
            printf("[INFO]: qos is %d,topic is ", qos);
            for (int i = 0; i < publishVariableHeader.topicLength; ++i) {
                printf("%c", publishVariableHeader.pTopicHead[i]);
            }
            printf(".\n");
            PublishPayload publishPayload;
            if (qos == 0){
								publishPayload.contentLength = fixedHeader.remainLength - variableHeaderLength;
								publishPayload.pContent = (pMessage + byteNum);
		            printf("[INFO]: content is ");
		            for (int i = 0; i < publishPayload.contentLength; ++i) {
		                printf("%c", publishPayload.pContent[i]);
		            }
		            printf(".\n");
            }else {
            		getPublishPayload(pMessage + byteNum, &publishPayload, fixedHeader.remainLength, variableHeaderLength);
            		printf("[INFO]: content is ");
		            for (int i = 0; i < publishPayload.contentLength; ++i) {
		                printf("%c", publishPayload.pContent[i]);
		            }
		            printf(".\n");
            		if (qos == 1){
            				// 发送 PUBACK
            				char *pSendMessage;
								    // 申请内存
								    memory_management_allocate(4, (void **) &pSendMessage);
								    fixedHeader.messageType = PUBACK;
								    fixedHeader.flag = 0x00;
								    fixedHeader.remainLength = 2;
								    // 封包：固定报头
								    byteNum = packageFixedHeader(pSendMessage, &fixedHeader);
								    // 可变报头
								    byteNum += packageCommonVariableHeader(pSendMessage + byteNum, publishVariableHeader.messageId);
								    // 开始发送
								    int errCode = msgQSend((msg_q * ) (publishPayload.clientId), pSendMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
								    if (errCode) {
								        printf("[Error]: Send PUBACK Fail! [ERR_CODE]: %d\n", errCode);
								    } else {
								        printf("[Success]: Send PUBACK success!\n");
								    }
								    // 释放内存
								    memory_management_free((void **) &pSendMessage, MAX_MESSAGE_LENGTH);
            		}else {
            				// qos == 2 的情况，发送 PUBREC & PUBCOMP
            				// 发送 PUBACK
            				char *pSendMessage;
								    // 申请内存
								    memory_management_allocate(4, (void **) &pSendMessage);
								    fixedHeader.messageType = PUBREC;
								    fixedHeader.flag = 0x00;
								    fixedHeader.remainLength = 2;
								    // 封包：固定报头
								    byteNum = packageFixedHeader(pSendMessage, &fixedHeader);
								    // 可变报头
								    byteNum += packageCommonVariableHeader(pSendMessage + byteNum, publishVariableHeader.messageId);
								    // 开始发送
								    int errCode = msgQSend((msg_q *) (publishPayload.clientId), pSendMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
								    if (errCode) {
								        printf("[Error]: Send PUBREC Fail! [ERR_CODE]: %d\n", errCode);
								    } else {
								        printf("[Success]: Send PUBREC success!\n");
								    }
								    // 等待 PUBREL
								    int mLen = msgQReceive(msgIdClient1, pSendMessage, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
								    if (mLen > 0) {
								        byteNum = getFixedHeader(pSendMessage, &fixedHeader);
								        if (fixedHeader.messageType == PUBREL) {
								            printf("[INFO]: Get PUBREL!\n");
								            CommonVariableHeader commonVariableHeader;
								            getCommonVariableHeader(pSendMessage + byteNum, &commonVariableHeader);
								            byteNum += 2;
								            if (commonVariableHeader.messageId == publishVariableHeader.messageId) {
								                printf("[INFO]: success messageId of PUBREL!\n");
								                // 发送 PUBCOMP
								                fixedHeader.messageType = PUBCOMP;
														    fixedHeader.flag = 0x00;
														    fixedHeader.remainLength = 2;
														    // 封包：固定报头
														    byteNum = packageFixedHeader(pSendMessage, &fixedHeader);
														    // 可变报头
														    byteNum += packageCommonVariableHeader(pSendMessage + byteNum, commonVariableHeader.messageId);
														    // 开始发送
														    int errCode = msgQSend((msg_q * ) (publishPayload.clientId), pSendMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
														    if (errCode) {
														        printf("[Error]: Send PUBCOMP Fail! [ERR_CODE]: %d\n", errCode);
														    } else {
														        printf("[Success]: Send PUBCOMP success!\n");
														    }
								            }
								        }
								    }
								    // 释放内存
								    memory_management_free((void **) &pSendMessage, MAX_MESSAGE_LENGTH);				
            		}
            }
        }else {
        		printf("[INFO]: Pass a message!\n");
        }
    }
    // 释放内存
    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
}

/**
 * 模拟客户端发送 UNSUBSCRIBE 报文
 */
void testUnsbuscribe(){
    int packageLength = 13;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    MSG_Q_ID brokerMsgId = (MSG_Q_ID) getBrokerId();
    FixedHeader fixedHeader;
    fixedHeader.messageType = UNSUBSCRIBE;
    fixedHeader.flag = 0x02;
    fixedHeader.remainLength = 11;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 可变报头
    *(pMessage + byteNum + 0) = 0x01;
    *(pMessage + byteNum + 1) = 0x01;
    // 客户端标识
    int2CharArray((int) msgIdClient1, pMessage + byteNum + 2);
    // 有效载荷
    *(pMessage + byteNum + 6) = 0x00;
    *(pMessage + byteNum + 7) = 0x03;
    *(pMessage + byteNum + 8) = 0x61;
    *(pMessage + byteNum + 9) = 0x2F;
    *(pMessage + byteNum + 10) = 0x62;
    // 发送 SUBSCRIBE
    int errCode = msgQSend(brokerMsgId, pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: Send UNSUBSCRIBE Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: Send UNSUBSCRIBE success!\n");
    }
    // 释放内存
    memory_management_free((void **) (&pMessage), sizeof(packageLength));
    // 申请内存
    memory_management_allocate(MAX_MESSAGE_LENGTH, (void **) &pMessage);
    // 等待响应
    int mLen = msgQReceive(msgIdClient1, pMessage, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
    if (mLen > 0) {
        byteNum = getFixedHeader(pMessage, &fixedHeader);
        if (fixedHeader.messageType == UNSUBACK) {
            printf("[INFO]: Get UNSUBACK!\n");
            CommonVariableHeader commonVariableHeader;
            getCommonVariableHeader(pMessage + byteNum, &commonVariableHeader);
            byteNum += 2;
            if (commonVariableHeader.messageId == 0x0101) {
                printf("[INFO]: UNSUBACK success!\n");
            }
        }
    } else {
    		printf("[ERROR]: receive UNSUBACK failed!\n");
    }
    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
}

/**
 * 模拟客户端发送 PINGREQ 报文
 */
void testPingReq(){
		testConnectClient1();
		int packageLength = 6;
    char *pMessage;
    // 申请内存
    memory_management_allocate(packageLength, (void **) &pMessage);
    MSG_Q_ID brokerMsgId = (MSG_Q_ID) getBrokerId();
    FixedHeader fixedHeader;
    fixedHeader.messageType = PINGREQ;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 4;
    // 封包：固定报头
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // 封包：客户端标识符
    int2CharArray((int) msgIdClient1, pMessage + byteNum);
		// 发送 DISCONNECT
    int errCode = msgQSend(brokerMsgId, pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: Send PINGREQ Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: Send PINGREQ success!\n");
    }
    // 释放内存
    memory_management_free((void **) (&pMessage), sizeof(packageLength));
    // 等待 PINGRESP
    memory_management_allocate(MAX_MESSAGE_LENGTH, (void **) &pMessage);
    int mLen = msgQReceive(msgIdClient1, pMessage, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
    if (mLen > 0) {
        printf("[INFO]: Get PINGRESP!\n");
    }
    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
}

/**
 * 模拟客户端发送 DISCONNECT 报文
 */
void testDisconnClient1(){
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
    int2CharArray((int) msgIdClient1, pMessage + byteNum);
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