#ifndef BUZZER_H
#define BUZZER_H

// ���� �Լ� ����
void initBuzzer(int pin);
void startBuzzer(int pin);
void stopBuzzer();

// �Լ� ������ Ÿ�� ����
typedef void (*InitBuzzerFunc)(int);
typedef void (*StartBuzzerFunc)(int);
typedef void (*StopBuzzerFunc)(void);

#endif
