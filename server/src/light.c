#include <wiringPi.h>
#include "../include/light.h"

void initLightSensor(int pin) {
    wiringPiSetupGpio();
    pinMode(pin, INPUT);
}

int readLightSensor(int pin) {
    return digitalRead(pin);  // 1: 어두움, 0: 밝음
}
