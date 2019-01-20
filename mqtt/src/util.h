#ifndef MQTT_BROKER_UTIL_H
#define MQTT_BROKER_UTIL_H

/**
* 将指定的字节转换为整数
*/
int char2int(char *pChar);

/**
 * 获取指定字节的第n位。（最右端为0位）
 */
UINT8 getCharSpecialBit(const char *pChar, UINT8 n);

/**
 * 设置字节的指定位为指定值
 */
void setCharSpecialBit(char *pChar, UINT8 n, boolean value);

/**
 * 获取编码长度（指的是 剩余长度 用的编码）
 */
UINT8 getEncodeLength(int value);

/**
 * 获取 主题过滤器的深度
 */
int getTopicFilterDeep(const char *pTopicFilter, int topicFilterLength);

/**
 * 比较两个 字符串 是否一样
 * 前提：已知它们有相同的长度
 */
boolean strCompare(const char *c1, const char *c2, int length);

/**
 * 将整型转为 char[4]
 */
void int2CharArray(int num, char *pCharArrayHead);

/**
 * 从 char[4] 转为 int
 */
void charArray2Int(char *pCharArrayHead, int *pNum);

/**
 * 获取一个时间戳
 */
long getCurrentTime();

#endif //MQTT_BROKER_UTIL_H
