#include "Define.h"

/**
* ��ָ�����ֽ�ת��Ϊ����
*/
int char2int(char *pChar) {
    int base = 1;
    int num = 0;
    for (int i = 0; i <= 7; i++) {
        if ((*pChar >> i) & 0x01 != 0) {
            num += base;
        }
        base *= 2;
    }
    return num;
}

/**
 * ��ȡָ���ֽڵĵ�nλ�������Ҷ�Ϊ0λ��
 */
UINT8 getCharSpecialBit(const char *pChar, UINT8 n) {
    return *pChar >> n & 0x01;
}

/**
 * �����ֽڵ�ָ��λΪָ��ֵ
 */
void setCharSpecialBit(char *pChar, UINT8 n, boolean value) {
    if (value == 1) {
        *pChar |= (1 << n);
    } else {
        *pChar &= ~(1 << n);
    }
}

/**
 * ��ȡ���볤�ȣ�ָ���� ʣ�೤�� �õı��룩
 */
UINT8 getEncodeLength(int value) {
    int byteNum = 0;
    if (value <= 127) {
        byteNum += 1;
    } else if (value <= 16383) {
        byteNum += 2;
    } else if (value <= 2097151) {
        byteNum += 3;
    } else {
        byteNum += 4;
    }
    return byteNum;
}

/**
 * ��ȡ ��������������
 */
int getTopicFilterDeep(const char *pTopicFilter, int topicFilterLength) {
    int lastPos = -1;   // ��һ�� / ��λ��
    int num = 0;
    for (int i = 0; i < topicFilterLength; i++) {
        if (pTopicFilter[i] == '/' || i == topicFilterLength - 1) {
            if (i - lastPos > 1) {
                ++num;
            }
        }
        lastPos = i;
    }
    return num;
}

/**
 * �Ƚ����� �ַ��� �Ƿ�һ��
 * ǰ�᣺��֪��������ͬ�ĳ���
 */
boolean strCompare(const char *c1, const char *c2, int length) {
    for (int i = 0; i < length; ++i) {
        if (c1[i] != c2[i]) {
            return False;
        }
    }
    return True;
}

/**
 * ������תΪ char[4]
 */
void int2CharArray(int num, char *pCharArrayHead) {
    memcpy(pCharArrayHead, &num, 4);
}

/**
 * �� char[4] תΪ int
 */
void charArray2Int(char *pCharArrayHead, int *pNum) {
    memcpy(pNum, pCharArrayHead, 4);
}

/**
 * ��ȡһ��ʱ���
 */
long getCurrentTime() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}