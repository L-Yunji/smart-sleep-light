#ifndef LED_H
#define LED_H

#define BRIGHT_LOW     30
#define BRIGHT_MEDIUM  60
#define BRIGHT_HIGH    100

// ���� �Լ� ���� (���� ���̺귯�� ���� �Լ�)
void initLED(int pin);
void ledControl(int pin, int brightness);
void turnOffLED(int pin);

// �Լ� ������ Ÿ�� ���� (�������� dlsym���� �ҷ��� �� ���)
typedef void (*InitLEDFunc)(int);
typedef void (*ControlLEDFunc)(int, int);
typedef void (*TurnOffLEDFunc)(int);

#endif
