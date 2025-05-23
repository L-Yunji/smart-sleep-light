#ifndef COUNTDOWN_H
#define COUNTDOWN_H

// 실제 함수 정의
void startCountdown(int start);

// 함수 포인터 타입 정의
typedef void (*StartCountdownFunc)(int);

#endif
