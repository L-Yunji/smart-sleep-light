#ifndef LIGHT_H
#define LIGHT_H

// 실제 함수 정의
void initLightSensor(int pin);
int readLightSensor(int pin);  // 1: 어두움, 0: 밝음

// 함수 포인터 타입 정의
typedef void (*InitLightSensorFunc)(int);
typedef int (*ReadLightSensorFunc)(int);

#endif
