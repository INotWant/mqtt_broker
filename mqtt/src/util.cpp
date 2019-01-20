#include "Define.h"

/**
* 将指定的字节转换为整数
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
 * 获取指定字节的第n位。（最右端为0位）
 */
UINT8 getCharSpecialBit(const char *pChar, UINT8 n) {
    return *pChar >> n & 0x01;
}

/**
 * 设置字节的指定位为指定值
 */
void setCharSpecialBit(char *pChar, UINT8 n, boolean value) {
    if (value == 1) {
        *pChar |= (1 << n);
    } else {
        *pChar &= ~(1 << n);
    }
}

/**
 * 获取编码长度（指的是 剩余长度 用的编码）
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
 * 获取 主题过滤器的深度
 */
int getTopicFilterDeep(const char *pTopicFilter, int topicFilterLength) {
    int lastPos = -1;   // 上一个 / 的位置
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
 * 比较两个 字符串 是否一样
 * 前提：已知它们有相同的长度
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
 * 将整型转为 char[4]
 */
void int2CharArray(int num, char *pCharArrayHead) {
    memcpy(pCharArrayHead, &num, 4);
}

/**
 * 从 char[4] 转为 int
 */
void charArray2Int(char *pCharArrayHead, int *pNum) {
    memcpy(pNum, pCharArrayHead, 4);
}

/**
 * 获取一个时间戳
 */
long getCurrentTime() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}