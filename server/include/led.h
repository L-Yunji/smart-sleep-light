#ifndef LED_H
#define LED_H

#define BRIGHT_LOW     30
#define BRIGHT_MEDIUM  60
#define BRIGHT_HIGH    100

// 실제 함수 정의 (공유 라이브러리 내부 함수)
void initLED(int pin);
void ledControl(int pin, int brightness);
void turnOffLED(int pin);

// 함수 포인터 타입 정의 (서버에서 dlsym으로 불러올 때 사용)
typedef void (*InitLEDFunc)(int);
typedef void (*ControlLEDFunc)(int, int);
typedef void (*TurnOffLEDFunc)(int);

#endif
