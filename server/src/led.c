#include <wiringPi.h>
#include <softPwm.h>
#include "../include/led.h"

void initLED(int pin) {
    wiringPiSetupGpio();
    pinMode(pin, OUTPUT);
    softPwmCreate(pin, 0, 100);
    softPwmWrite(pin, 100);  // ²¨Áø »óÅÂ (¿ª PWM)
}

void ledControl(int pin, int brightness) {
    softPwmWrite(pin, 100 - brightness);  // ¹à±â Á¶Àý (30~100)
}

void turnOffLED(int pin) {
    softPwmWrite(pin, 100);
}
