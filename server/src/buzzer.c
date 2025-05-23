#include <wiringPi.h>
#include <softTone.h>
#include <pthread.h>
#include <unistd.h>
#include "../include/buzzer.h"

#define MAX_NOTES 100  // 멜로디 최대 길이
#define BUZZER_DELAY_MARGIN 50  // 음 사이의 여유 시간

static pthread_t buzzerThread;
static int buzzerPin = 0;
static int buzzerRunning = 0;

// "작은 별" 멜로디 (Twinkle Twinkle Little Star)
// 음계: C C G G A A G / F F E E D D C / ...
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

// 부저 초기화
void initBuzzer(int pin) {
    wiringPiSetupGpio();
    buzzerPin = pin;
    softToneCreate(buzzerPin);
    softToneWrite(buzzerPin, 0);  // 무음으로 시작
}

// 자장가 재생 스레드
void* buzzerMelodyThread(void* arg) {
    while (buzzerRunning) {
        for (int i = 0; i < melodyLength && buzzerRunning; ++i) {
            softToneWrite(buzzerPin, melody[i]);
            delay(noteDurations[i]);
            softToneWrite(buzzerPin, 0);  // 음 사이 여유
            delay(BUZZER_DELAY_MARGIN);
        }
    }
    softToneWrite(buzzerPin, 0);  // 정지 시 무음
    return NULL;
}

// BUZZER ON → 자장가 시작
void startBuzzer(int pin) {
    if (buzzerRunning) return;  // 이미 실행 중이면 무시
    buzzerRunning = 1;
    pthread_create(&buzzerThread, NULL, buzzerMelodyThread, NULL);
}

// BUZZER OFF → 멈춤
void stopBuzzer() {
    if (!buzzerRunning) return;
    buzzerRunning = 0;
    pthread_join(buzzerThread, NULL);  // 스레드 종료 대기
    softToneWrite(buzzerPin, 0);       // 무음
}
