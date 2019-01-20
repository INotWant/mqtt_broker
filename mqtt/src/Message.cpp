#include "Message.h"

// 获取剩余长度
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
 * 用于读取报文中的所有整型
 */
unsigned int getUnsignedInt(char *pMessage) {
    unsigned int num = 0;
    num += (char2int(pMessage) * 256);
    num += (char2int(pMessage + 1));
    return num;
}

/**
 * 用于从无符号整数（messageId）转为（2字节）char
 */
void unint2charArray(unsigned int value, char *pChar) {
    *pChar = (value >> 8) & 0xFF;
    *(pChar + 1) = value & 0xFF;
}

/******************************************************************
 *                       拆包相关实现
 ******************************************************************/

/**
 * 拆包：获取固定报头
 * 返回：固定报头所占的字节数
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
 * 获取 CONNECT 的可变报头
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
 * 获取 CONNECT 的有效载荷
 */
void getConnPayload(char *pMessage, ConnPayload *pConnPayload) {
    charArray2Int(pMessage, &pConnPayload->clientId);
}

/**
 * 获取 publish 的可变头部
 * 返回：可变报头的长度
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
 * 获取 publish 的有效载荷
 */
void getPublishPayload(char *pMessage, PublishPayload *pPublishPayload, int remainLength, int variableHeaderLength) {
    // 获取客户端标识符
		charArray2Int(pMessage, &pPublishPayload->clientId);
		pPublishPayload->contentLength = remainLength - variableHeaderLength - 4;
		pPublishPayload->pContent = pMessage + 4;
}

/**
 * 获取 PUBACK or PUBREC or PUBREL or PUBCOMP 可变报头
 *      or SUBSCRIBE or UNSUBSCRIBE
 */
void getCommonVariableHeader(char *pMessage, CommonVariableHeader *pCommonVariableHeader) {
    pCommonVariableHeader->messageId = getUnsignedInt(pMessage);
}

/**
 * 获取 subscribe 的有效载荷
 */
void getSubscribePayload(char *pMessage, SubscribePayload *pSubscribePayload, int payloadLength) {
    // 获取客户端标识符
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
 * 获取 unsubscribe 的有效载荷
 */
void getUnsubscribePayload(char *pMessage, UnsubscribePayload *pUnsubscribePayload, int payloadLength) {
    // 获取客户端标识符
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
 *                       封包相关实现
 ******************************************************************/

/**
 * 封装固定头部
 * @param pMessage pMessage 指向已经申请到的指定空间
 * @return 固定报头的长度
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
 * 封包：通用的可变报头
 * @param pMessage 存储的位置
 * @param messageId 报文标识符
 * @return 可变报头的长度，固定为 2
 */
int packageCommonVariableHeader(char *pMessage, unsigned int messageId) {
    unint2charArray(messageId, pMessage);
    return 2;
}