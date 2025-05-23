#ifndef BUZZER_H
#define BUZZER_H

// 실제 함수 정의
void initBuzzer(int pin);
void startBuzzer(int pin);
void stopBuzzer();

// 함수 포인터 타입 정의
typedef void (*InitBuzzerFunc)(int);
typedef void (*StartBuzzerFunc)(int);
typedef void (*StopBuzzerFunc)(void);

#endif
