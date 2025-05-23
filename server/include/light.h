#ifndef LIGHT_H
#define LIGHT_H

// ���� �Լ� ����
void initLightSensor(int pin);
int readLightSensor(int pin);  // 1: ��ο�, 0: ����

// �Լ� ������ Ÿ�� ����
typedef void (*InitLightSensorFunc)(int);
typedef int (*ReadLightSensorFunc)(int);

#endif
