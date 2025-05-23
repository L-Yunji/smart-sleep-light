#include <wiringPi.h>
#include <softPwm.h>
#include "../include/led.h"

void initLED(int pin) {
    wiringPiSetupGpio();
    pinMode(pin, OUTPUT);
    softPwmCreate(pin, 0, 100);
    softPwmWrite(pin, 100);  // ���� ���� (�� PWM)
}

void ledControl(int pin, int brightness) {
    softPwmWrite(pin, 100 - brightness);  // ��� ���� (30~100)
}

void turnOffLED(int pin) {
    softPwmWrite(pin, 100);
}
