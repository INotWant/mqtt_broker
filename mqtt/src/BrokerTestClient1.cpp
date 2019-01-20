#include "Broker.h"
#include "Define.h"

MSG_Q_ID msgIdClient1 = msgQCreate(MAX_MESSAGE_NUM, MAX_MESSAGE_LENGTH, MSG_Q_PRIORITY);

/**
 * ���Գ�ʼ��
 */
void testInit(){
		initBroker();
    startBroker();
}

/**
 * ģ��ͻ��˷��� CONNECT ����
 */
void testConnectClient1() {
    int packageLength = 16;
    char *pMessage;
    // �����ڴ�
    memory_management_allocate(packageLength, (void **) &pMessage);
    MSG_Q_ID brokerMsgId = (MSG_Q_ID) getBrokerId();
    FixedHeader fixedHeader;
    fixedHeader.messageType = CONNECT;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 14;
    // ������̶���ͷ
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // ������ɱ䱨ͷ
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
    // ������ͻ��˱�ʶ��
    int2CharArray((int) msgIdClient1, pMessage + byteNum + 10);
    // ���� CONNECT
    int errCode = msgQSend(brokerMsgId, pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: Send CONNECT Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: Send CONNECT success!\n");
    }
    // �ͷ��ڴ�
    memory_management_free((void **) (&pMessage), sizeof(packageLength));
    // �����ڴ�
    memory_management_allocate(MAX_MESSAGE_LENGTH, (void **) &pMessage);
    int mLen = msgQReceive(msgIdClient1, pMessage, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
    if (mLen > 0) {
        printf("[INFO]: Get CONNACK!\n");
    }
    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
}

/**
 * ģ��ͻ��˷�������
 */
void testSubscribe() {
		testConnectClient1();
    int packageLength = 14;
    char *pMessage;
    // �����ڴ�
    memory_management_allocate(packageLength, (void **) &pMessage);
    MSG_Q_ID brokerMsgId = (MSG_Q_ID) getBrokerId();
    FixedHeader fixedHeader;
    fixedHeader.messageType = SUBSCRIBE;
    fixedHeader.flag = 0x02;
    fixedHeader.remainLength = 12;
    // ������̶���ͷ
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // �ɱ䱨ͷ
    *(pMessage + byteNum + 0) = 0x00;
    *(pMessage + byteNum + 1) = 0x01;
    // �ͻ��˱�ʶ
    int2CharArray((int) msgIdClient1, pMessage + byteNum + 2);
    // ��Ч�غ�
    *(pMessage + byteNum + 6) = 0x00;
    *(pMessage + byteNum + 7) = 0x03;
    *(pMessage + byteNum + 8) = 0x61;
    *(pMessage + byteNum + 9) = 0x2F;
    *(pMessage + byteNum + 10) = 0x62;
    *(pMessage + byteNum + 11) = 0x02;
    // ���� SUBSCRIBE
    int errCode = msgQSend(brokerMsgId, pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: Send SUBSCRIBE Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: Send SUBSCRIBE success!\n");
    }
    // �ͷ��ڴ�
    memory_management_free((void **) (&pMessage), sizeof(packageLength));
    // �����ڴ�
    memory_management_allocate(MAX_MESSAGE_LENGTH, (void **) &pMessage);
    // �ȴ���Ӧ
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
    // �ȴ����ܶ��ĵ�����
		// �����ڴ�
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
            				// ���� PUBACK
            				char *pSendMessage;
								    // �����ڴ�
								    memory_management_allocate(4, (void **) &pSendMessage);
								    fixedHeader.messageType = PUBACK;
								    fixedHeader.flag = 0x00;
								    fixedHeader.remainLength = 2;
								    // ������̶���ͷ
								    byteNum = packageFixedHeader(pSendMessage, &fixedHeader);
								    // �ɱ䱨ͷ
								    byteNum += packageCommonVariableHeader(pSendMessage + byteNum, publishVariableHeader.messageId);
								    // ��ʼ����
								    int errCode = msgQSend((msg_q * ) (publishPayload.clientId), pSendMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
								    if (errCode) {
								        printf("[Error]: Send PUBACK Fail! [ERR_CODE]: %d\n", errCode);
								    } else {
								        printf("[Success]: Send PUBACK success!\n");
								    }
								    // �ͷ��ڴ�
								    memory_management_free((void **) &pSendMessage, MAX_MESSAGE_LENGTH);
            		}else {
            				// qos == 2 ����������� PUBREC & PUBCOMP
            				// ���� PUBACK
            				char *pSendMessage;
								    // �����ڴ�
								    memory_management_allocate(4, (void **) &pSendMessage);
								    fixedHeader.messageType = PUBREC;
								    fixedHeader.flag = 0x00;
								    fixedHeader.remainLength = 2;
								    // ������̶���ͷ
								    byteNum = packageFixedHeader(pSendMessage, &fixedHeader);
								    // �ɱ䱨ͷ
								    byteNum += packageCommonVariableHeader(pSendMessage + byteNum, publishVariableHeader.messageId);
								    // ��ʼ����
								    int errCode = msgQSend((msg_q *) (publishPayload.clientId), pSendMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
								    if (errCode) {
								        printf("[Error]: Send PUBREC Fail! [ERR_CODE]: %d\n", errCode);
								    } else {
								        printf("[Success]: Send PUBREC success!\n");
								    }
								    // �ȴ� PUBREL
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
								                // ���� PUBCOMP
								                fixedHeader.messageType = PUBCOMP;
														    fixedHeader.flag = 0x00;
														    fixedHeader.remainLength = 2;
														    // ������̶���ͷ
														    byteNum = packageFixedHeader(pSendMessage, &fixedHeader);
														    // �ɱ䱨ͷ
														    byteNum += packageCommonVariableHeader(pSendMessage + byteNum, commonVariableHeader.messageId);
														    // ��ʼ����
														    int errCode = msgQSend((msg_q * ) (publishPayload.clientId), pSendMessage, byteNum, NO_WAIT, MSG_PRI_NORMAL);
														    if (errCode) {
														        printf("[Error]: Send PUBCOMP Fail! [ERR_CODE]: %d\n", errCode);
														    } else {
														        printf("[Success]: Send PUBCOMP success!\n");
														    }
								            }
								        }
								    }
								    // �ͷ��ڴ�
								    memory_management_free((void **) &pSendMessage, MAX_MESSAGE_LENGTH);				
            		}
            }
        }else {
        		printf("[INFO]: Pass a message!\n");
        }
    }
    // �ͷ��ڴ�
    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
}

/**
 * ģ��ͻ��˷��� UNSUBSCRIBE ����
 */
void testUnsbuscribe(){
    int packageLength = 13;
    char *pMessage;
    // �����ڴ�
    memory_management_allocate(packageLength, (void **) &pMessage);
    MSG_Q_ID brokerMsgId = (MSG_Q_ID) getBrokerId();
    FixedHeader fixedHeader;
    fixedHeader.messageType = UNSUBSCRIBE;
    fixedHeader.flag = 0x02;
    fixedHeader.remainLength = 11;
    // ������̶���ͷ
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // �ɱ䱨ͷ
    *(pMessage + byteNum + 0) = 0x01;
    *(pMessage + byteNum + 1) = 0x01;
    // �ͻ��˱�ʶ
    int2CharArray((int) msgIdClient1, pMessage + byteNum + 2);
    // ��Ч�غ�
    *(pMessage + byteNum + 6) = 0x00;
    *(pMessage + byteNum + 7) = 0x03;
    *(pMessage + byteNum + 8) = 0x61;
    *(pMessage + byteNum + 9) = 0x2F;
    *(pMessage + byteNum + 10) = 0x62;
    // ���� SUBSCRIBE
    int errCode = msgQSend(brokerMsgId, pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: Send UNSUBSCRIBE Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: Send UNSUBSCRIBE success!\n");
    }
    // �ͷ��ڴ�
    memory_management_free((void **) (&pMessage), sizeof(packageLength));
    // �����ڴ�
    memory_management_allocate(MAX_MESSAGE_LENGTH, (void **) &pMessage);
    // �ȴ���Ӧ
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
 * ģ��ͻ��˷��� PINGREQ ����
 */
void testPingReq(){
		testConnectClient1();
		int packageLength = 6;
    char *pMessage;
    // �����ڴ�
    memory_management_allocate(packageLength, (void **) &pMessage);
    MSG_Q_ID brokerMsgId = (MSG_Q_ID) getBrokerId();
    FixedHeader fixedHeader;
    fixedHeader.messageType = PINGREQ;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 4;
    // ������̶���ͷ
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // ������ͻ��˱�ʶ��
    int2CharArray((int) msgIdClient1, pMessage + byteNum);
		// ���� DISCONNECT
    int errCode = msgQSend(brokerMsgId, pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: Send PINGREQ Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: Send PINGREQ success!\n");
    }
    // �ͷ��ڴ�
    memory_management_free((void **) (&pMessage), sizeof(packageLength));
    // �ȴ� PINGRESP
    memory_management_allocate(MAX_MESSAGE_LENGTH, (void **) &pMessage);
    int mLen = msgQReceive(msgIdClient1, pMessage, MAX_MESSAGE_LENGTH, WAIT_FOREVER);
    if (mLen > 0) {
        printf("[INFO]: Get PINGRESP!\n");
    }
    memory_management_free((void **) &pMessage, MAX_MESSAGE_LENGTH);
}

/**
 * ģ��ͻ��˷��� DISCONNECT ����
 */
void testDisconnClient1(){
		int packageLength = 6;
    char *pMessage;
    // �����ڴ�
    memory_management_allocate(packageLength, (void **) &pMessage);
    MSG_Q_ID brokerMsgId = (MSG_Q_ID) getBrokerId();
    FixedHeader fixedHeader;
    fixedHeader.messageType = DISCONNECT;
    fixedHeader.flag = 0x00;
    fixedHeader.remainLength = 4;
    // ������̶���ͷ
    int byteNum = packageFixedHeader(pMessage, &fixedHeader);
    // ������ͻ��˱�ʶ��
    int2CharArray((int) msgIdClient1, pMessage + byteNum);
		// ���� DISCONNECT
    int errCode = msgQSend(brokerMsgId, pMessage, packageLength, NO_WAIT, MSG_PRI_NORMAL);
    if (errCode) {
        printf("[Error]: Send DISCONNECT Fail! [ERR_CODE]: %d\n", errCode);
    } else {
        printf("[Success]: Send DISCONNECT success!\n");
    }
    // �ͷ��ڴ�
    memory_management_free((void **) (&pMessage), sizeof(packageLength));				
}