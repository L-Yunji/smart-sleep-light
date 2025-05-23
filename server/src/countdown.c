#include <wiringPi.h>
#include <softTone.h>
#include <unistd.h>
#include <stdio.h>
#include "../include/countdown.h"

#define BCD_A 5
#define BCD_B 6
#define BCD_C 12
#define BCD_D 16
#define BUZZER_PIN 23

static void setDigit(int digit) {
    digitalWrite(BCD_A, (digit & 0x1) ? HIGH : LOW);
    digitalWrite(BCD_B, (digit >> 1 & 0x1) ? HIGH : LOW);
    digitalWrite(BCD_C, (digit >> 2 & 0x1) ? HIGH : LOW);
    digitalWrite(BCD_D, (digit >> 3 & 0x1) ? HIGH : LOW);
}

static void playCountdownMelody() {
    softToneCreate(BUZZER_PIN);

    int melody[] = {523, 659, 784, 1046};
    int duration = 200;
    int length = sizeof(melody) / sizeof(int);

    for (int i = 0; i < length; ++i) {
        softToneWrite(BUZZER_PIN, melody[i]);
        delay(duration);
    }

    softToneWrite(BUZZER_PIN, 0);
}

void startCountdown(int start) {
    wiringPiSetupGpio();
    pinMode(BCD_A, OUTPUT);
    pinMode(BCD_B, OUTPUT);
    pinMode(BCD_C, OUTPUT);
    pinMode(BCD_D, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    for (int i = start; i >= 0; i--) {
        setDigit(i);
        printf("[INFO] Countdown: %d\n", i);
        sleep(1);
    }

    printf("[INFO] Countdown finished. Playing melody!\n");
    playCountdownMelody();
}
