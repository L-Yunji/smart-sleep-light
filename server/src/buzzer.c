#include <wiringPi.h>
#include <softTone.h>
#include <pthread.h>
#include <unistd.h>
#include "../include/buzzer.h"

#define MAX_NOTES 100  // ��ε� �ִ� ����
#define BUZZER_DELAY_MARGIN 50  // �� ������ ���� �ð�

static pthread_t buzzerThread;
static int buzzerPin = 0;
static int buzzerRunning = 0;

// "���� ��" ��ε� (Twinkle Twinkle Little Star)
// ����: C C G G A A G / F F E E D D C / ...
static int melody[] = {
    261, 261, 392, 392, 440, 440, 392,
    349, 349, 329, 329, 294, 294, 261,
    392, 392, 349, 349, 329, 329, 294,
    392, 392, 349, 349, 329, 329, 294,
    261, 261, 392, 392, 440, 440, 392,
    349, 349, 329, 329, 294, 294, 261
};
static int noteDurations[] = {
    400, 400, 400, 400, 400, 400, 800,
    400, 400, 400, 400, 400, 400, 800,
    400, 400, 400, 400, 400, 400, 800,
    400, 400, 400, 400, 400, 400, 800,
    400, 400, 400, 400, 400, 400, 800,
    400, 400, 400, 400, 400, 400, 800
};
static int melodyLength = sizeof(melody) / sizeof(int);

// ���� �ʱ�ȭ
void initBuzzer(int pin) {
    wiringPiSetupGpio();
    buzzerPin = pin;
    softToneCreate(buzzerPin);
    softToneWrite(buzzerPin, 0);  // �������� ����
}

// ���尡 ��� ������
void* buzzerMelodyThread(void* arg) {
    while (buzzerRunning) {
        for (int i = 0; i < melodyLength && buzzerRunning; ++i) {
            softToneWrite(buzzerPin, melody[i]);
            delay(noteDurations[i]);
            softToneWrite(buzzerPin, 0);  // �� ���� ����
            delay(BUZZER_DELAY_MARGIN);
        }
    }
    softToneWrite(buzzerPin, 0);  // ���� �� ����
    return NULL;
}

// BUZZER ON �� ���尡 ����
void startBuzzer(int pin) {
    if (buzzerRunning) return;  // �̹� ���� ���̸� ����
    buzzerRunning = 1;
    pthread_create(&buzzerThread, NULL, buzzerMelodyThread, NULL);
}

// BUZZER OFF �� ����
void stopBuzzer() {
    if (!buzzerRunning) return;
    buzzerRunning = 0;
    pthread_join(buzzerThread, NULL);  // ������ ���� ���
    softToneWrite(buzzerPin, 0);       // ����
}
