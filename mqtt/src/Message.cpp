#include "Message.h"

// ��ȡʣ�೤��
int getRemainLength(char *pRemainLength) {
    int count = 0;
    int base = 128;
    int remainLength = (pRemainLength[count] & 0x7F);
    while ((pRemainLength[count] >> 7) & 0x01 != 0) {
        count++;
        remainLength += (pRemainLength[count] & 0x7F) * base;
        base *= 128;
    }
    return remainLength;
}

/**
 * ���ڶ�ȡ�����е���������
 */
unsigned int getUnsignedInt(char *pMessage) {
    unsigned int num = 0;
    num += (char2int(pMessage) * 256);
    num += (char2int(pMessage + 1));
    return num;
}

/**
 * ���ڴ��޷���������messageId��תΪ��2�ֽڣ�char
 */
void unint2charArray(unsigned int value, char *pChar) {
    *pChar = (value >> 8) & 0xFF;
    *(pChar + 1) = value & 0xFF;
}

/******************************************************************
 *                       ������ʵ��
 ******************************************************************/

/**
 * �������ȡ�̶���ͷ
 * ���أ��̶���ͷ��ռ���ֽ���
 */
int getFixedHeader(char *pMessage, FixedHeader *pFixedHeader) {
    pFixedHeader->messageType = ((pMessage[0] & 0xF0) >> 4);
    pFixedHeader->flag = (pMessage[0] & 0x0F);
    pFixedHeader->remainLength = getRemainLength(pMessage + 1);
    int byteNum = 1;
    if (pFixedHeader->remainLength <= 127) {
        byteNum += 1;
    } else if (pFixedHeader->remainLength <= 16383) {
        byteNum += 2;
    } else if (pFixedHeader->remainLength <= 2097151) {
        byteNum += 3;
    } else {
        byteNum += 4;
    }
    return byteNum;
}

/**
 * ��ȡ CONNECT �Ŀɱ䱨ͷ
 */
int getConnVariableHeader(char *pMessage, ConnVariableHeader *pConnVariableHeader) {
    pConnVariableHeader->protocolNameLength = getUnsignedInt(pMessage);
    pConnVariableHeader->mqtt[0] = *(pMessage + 2);
    pConnVariableHeader->mqtt[1] = *(pMessage + 3);
    pConnVariableHeader->mqtt[2] = *(pMessage + 4);
    pConnVariableHeader->mqtt[3] = *(pMessage + 5);
    pConnVariableHeader->level = getUnsignedInt(pMessage + 6);
    pConnVariableHeader->userNameFlag = getCharSpecialBit(pMessage + 7, 7);
    pConnVariableHeader->passwordFlag = getCharSpecialBit(pMessage + 7, 6);
    pConnVariableHeader->willRetain = getCharSpecialBit(pMessage + 7, 5);
    UINT8 willQos = getCharSpecialBit(pMessage + 7, 4) * 2;
    willQos += getCharSpecialBit(pMessage + 7, 3);
    pConnVariableHeader->willQos = willQos;
    pConnVariableHeader->willFlag = getCharSpecialBit(pMessage + 7, 2);
    pConnVariableHeader->cleanSession = getCharSpecialBit(pMessage + 7, 1);
    pConnVariableHeader->reserved_ = getCharSpecialBit(pMessage + 7, 0);
    pConnVariableHeader->keepAlive = getUnsignedInt(pMessage + 8);
    return 10;
}

/**
 * ��ȡ CONNECT ����Ч�غ�
 */
void getConnPayload(char *pMessage, ConnPayload *pConnPayload) {
    charArray2Int(pMessage, &pConnPayload->clientId);
}

/**
 * ��ȡ publish �Ŀɱ�ͷ��
 * ���أ��ɱ䱨ͷ�ĳ���
 */
int getPublishVariableHeader(char *pMessage, PublishVariableHeader *pPublishVariableHeader, int qos) {
    pPublishVariableHeader->topicLength = getUnsignedInt(pMessage);
    pPublishVariableHeader->pTopicHead = pMessage + 2;
    if (qos >= 1) {
        pPublishVariableHeader->messageId = getUnsignedInt(pMessage + 2 + pPublishVariableHeader->topicLength);
        return pPublishVariableHeader->topicLength + 4;
    }
    pPublishVariableHeader->messageId = 0;
    return pPublishVariableHeader->topicLength + 2;
}

/**
 * ��ȡ publish ����Ч�غ�
 */
void getPublishPayload(char *pMessage, PublishPayload *pPublishPayload, int remainLength, int variableHeaderLength) {
    // ��ȡ�ͻ��˱�ʶ��
		charArray2Int(pMessage, &pPublishPayload->clientId);
		pPublishPayload->contentLength = remainLength - variableHeaderLength - 4;
		pPublishPayload->pContent = pMessage + 4;
}

/**
 * ��ȡ PUBACK or PUBREC or PUBREL or PUBCOMP �ɱ䱨ͷ
 *      or SUBSCRIBE or UNSUBSCRIBE
 */
void getCommonVariableHeader(char *pMessage, CommonVariableHeader *pCommonVariableHeader) {
    pCommonVariableHeader->messageId = getUnsignedInt(pMessage);
}

/**
 * ��ȡ subscribe ����Ч�غ�
 */
void getSubscribePayload(char *pMessage, SubscribePayload *pSubscribePayload, int payloadLength) {
    // ��ȡ�ͻ��˱�ʶ��
    charArray2Int(pMessage, &pSubscribePayload->clientId);
    int pos = 4;
    int i = 0;
    while (pos < payloadLength && i <= 9) {
        int topicFilterLength = getUnsignedInt(pMessage + pos);
        pSubscribePayload->topicFilterLengthArray[i] = topicFilterLength;
        pSubscribePayload->pTopicFilterArray[i] = pMessage + pos + 2;
        pos += (2 + topicFilterLength);
        pSubscribePayload->topicFilterQosArray[i] = getCharSpecialBit(pMessage + pos, 1) * 2;
        pSubscribePayload->topicFilterQosArray[i] += getCharSpecialBit(pMessage + pos, 0);
        pos += 1;
        ++i;
    }
    pSubscribePayload->topicFilterNum = i;
}

/**
 * ��ȡ unsubscribe ����Ч�غ�
 */
void getUnsubscribePayload(char *pMessage, UnsubscribePayload *pUnsubscribePayload, int payloadLength) {
    // ��ȡ�ͻ��˱�ʶ��
    charArray2Int(pMessage, &pUnsubscribePayload->clientId);
    int pos = 4;
    int i = 0;
    while (pos < payloadLength && i <= 9) {
        int topicFilterLength = getUnsignedInt(pMessage + pos);
        pUnsubscribePayload->topicFilterLengthArray[i] = topicFilterLength;
        pUnsubscribePayload->pTopicFilterArray[i] = pMessage + pos + 2;
        pos += (2 + topicFilterLength);
        ++i;
    }
    pUnsubscribePayload->topicFilterNum = i;
}

/******************************************************************
 *                       ������ʵ��
 ******************************************************************/

/**
 * ��װ�̶�ͷ��
 * @param pMessage pMessage ָ���Ѿ����뵽��ָ���ռ�
 * @return �̶���ͷ�ĳ���
 */
int packageFixedHeader(char *pMessage, FixedHeader *pFixedHeader) {
    int byteNum = 1;
    pMessage[0] = (char) ((pFixedHeader->messageType << 4) + (pFixedHeader->flag & 0x0F));
    if (pFixedHeader->remainLength <= 127) {
        byteNum += 1;
        pMessage[1] = (char) pFixedHeader->remainLength;
        setCharSpecialBit(pMessage + 1, 7, 0);
    } else if (pFixedHeader->remainLength <= 16383) {
        byteNum += 2;
        pMessage[1] = (char) (pFixedHeader->remainLength % 128);
        setCharSpecialBit(pMessage + 1, 7, 1);
        pMessage[2] = (char) (pFixedHeader->remainLength / 128);
        setCharSpecialBit(pMessage + 2, 7, 0);
    } else if (pFixedHeader->remainLength <= 2097151) {
        byteNum += 3;
        pMessage[1] = (char) (pFixedHeader->remainLength % 128);
        setCharSpecialBit(pMessage + 1, 7, 1);
        pMessage[2] = (char) (pFixedHeader->remainLength / 128 % 128);
        setCharSpecialBit(pMessage + 2, 7, 1);
        pMessage[3] = (char) (pFixedHeader->remainLength / 128 / 128);
        setCharSpecialBit(pMessage + 3, 7, 0);
    } else {
        byteNum += 4;
        pMessage[1] = (char) (pFixedHeader->remainLength % 128);
        setCharSpecialBit(pMessage + 1, 7, 1);
        pMessage[2] = (char) (pFixedHeader->remainLength / 128 % 128);
        setCharSpecialBit(pMessage + 2, 7, 1);
        pMessage[3] = (char) (pFixedHeader->remainLength / 16384 % 128);
        setCharSpecialBit(pMessage + 3, 7, 1);
        pMessage[4] = (char) (pFixedHeader->remainLength / 2097152);
        setCharSpecialBit(pMessage + 4, 7, 0);
    }
    return byteNum;
}

/**
 * �����ͨ�õĿɱ䱨ͷ
 * @param pMessage �洢��λ��
 * @param messageId ���ı�ʶ��
 * @return �ɱ䱨ͷ�ĳ��ȣ��̶�Ϊ 2
 */
int packageCommonVariableHeader(char *pMessage, unsigned int messageId) {
    unint2charArray(messageId, pMessage);
    return 2;
}