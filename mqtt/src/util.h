#ifndef MQTT_BROKER_UTIL_H
#define MQTT_BROKER_UTIL_H

/**
* ��ָ�����ֽ�ת��Ϊ����
*/
int char2int(char *pChar);

/**
 * ��ȡָ���ֽڵĵ�nλ�������Ҷ�Ϊ0λ��
 */
UINT8 getCharSpecialBit(const char *pChar, UINT8 n);

/**
 * �����ֽڵ�ָ��λΪָ��ֵ
 */
void setCharSpecialBit(char *pChar, UINT8 n, boolean value);

/**
 * ��ȡ���볤�ȣ�ָ���� ʣ�೤�� �õı��룩
 */
UINT8 getEncodeLength(int value);

/**
 * ��ȡ ��������������
 */
int getTopicFilterDeep(const char *pTopicFilter, int topicFilterLength);

/**
 * �Ƚ����� �ַ��� �Ƿ�һ��
 * ǰ�᣺��֪��������ͬ�ĳ���
 */
boolean strCompare(const char *c1, const char *c2, int length);

/**
 * ������תΪ char[4]
 */
void int2CharArray(int num, char *pCharArrayHead);

/**
 * �� char[4] תΪ int
 */
void charArray2Int(char *pCharArrayHead, int *pNum);

/**
 * ��ȡһ��ʱ���
 */
long getCurrentTime();

#endif //MQTT_BROKER_UTIL_H
