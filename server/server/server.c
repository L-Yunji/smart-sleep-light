#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <dlfcn.h>
#include <wiringPi.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <softTone.h>

#include "../include/led.h"
#include "../include/light.h"
#include "../include/buzzer.h"
#include "../include/countdown.h"

#define PORT 5100
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define LED_PIN 18
#define LIGHT_SENSOR_PIN 24
#define BUZZER_PIN 23

// ==== 데몬화 함수 ====

void daemonize() {
    pid_t pid;

    // 1차 fork → 부모 종료
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    // 새 세션 리더
    if (setsid() < 0) exit(EXIT_FAILURE);

    // 2차 fork → 데몬화 확정
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    // 권한 마스크 제거 및 디렉토리 이동
    umask(0);
    chdir("/");

    // stdout/stderr을 로그파일로 리디렉션 (systemd에서 journal 사용 시 생략 가능)
    int fd = open("/tmp/smart_sleep_light.log", O_RDWR | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        dup2(fd, STDOUT_FILENO);  // printf() 출력
        dup2(fd, STDERR_FILENO);  // fprintf(stderr)
        // dup2 이후 close(fd) 해도 됨
    }

    // PID 파일 기록
    FILE *pid_fp = fopen("/tmp/smart_sleep_light.pid", "w");
    if (pid_fp) {
        fprintf(pid_fp, "%d\n", getpid());
        fflush(pid_fp);  // 버퍼 비우기 (중요)
        fsync(fileno(pid_fp));  // 디스크에 즉시 기록
        fclose(pid_fp);
    } else {
        // 실패 로그 기록
        int fd = open("/tmp/smart_sleep_light.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
        dprintf(fd, "Failed to create PID file\n");
        close(fd);
    }

}

typedef enum { LED_OFF, LED_LOW, LED_MEDIUM, LED_HIGH } LedState;
typedef enum { LIGHT_BRIGHT, LIGHT_DARK } LightCondition;

typedef struct {
    LedState led;
    int auto_mode;
    int countdown_running;
    LightCondition light;
} DeviceStatus;

DeviceStatus status;

int server_fd;
int client_socks[MAX_CLIENTS];
pthread_t light_thread, countdownThread;
int auto_mode = 0, stop_auto_mode = 0, countdownRunning = 0;

int alarm_hour = -1;
int alarm_min = -1;
int alarm_triggered = 0;
pthread_t alarmThread;

// 동적 라이브러리
void *led_handle, *light_handle, *buzzer_handle, *countdown_handle;
InitLEDFunc initLED_fp;
ControlLEDFunc turnOnLED_fp;
TurnOffLEDFunc turnOffLED_fp;
InitLightSensorFunc initLightSensor_fp;
ReadLightSensorFunc readLightSensor_fp;
InitBuzzerFunc initBuzzer_fp;
StartBuzzerFunc startBuzzer_fp;
StopBuzzerFunc stopBuzzer_fp;
StartCountdownFunc startCountdown_fp;

// ==== 라이브러리 로딩 함수들 ====
void load_led_library() {
    led_handle = dlopen("/home/iam/smart_sleep_light/lib/libled.so", RTLD_LAZY);
    if (!led_handle) {
        fprintf(stderr, "[ERROR] Failed to load libled.so: %s\n", dlerror());
        exit(1);
    }
    initLED_fp = dlsym(led_handle, "initLED");
    turnOnLED_fp = dlsym(led_handle, "ledControl");
    turnOffLED_fp = dlsym(led_handle, "turnOffLED");
}

void load_light_library() {
    light_handle = dlopen("/home/iam/smart_sleep_light/lib/liblight.so", RTLD_LAZY);
    if (!light_handle) {
        fprintf(stderr, "[ERROR] Failed to load liblight.so: %s\n", dlerror());
        exit(1);
    }
    initLightSensor_fp = dlsym(light_handle, "initLightSensor");
    readLightSensor_fp = dlsym(light_handle, "readLightSensor");
}

void load_buzzer_library() {
    buzzer_handle = dlopen("/home/iam/smart_sleep_light/lib/libbuzzer.so", RTLD_LAZY);
    if (!buzzer_handle) {
        fprintf(stderr, "[ERROR] Failed to load libbuzzer.so: %s\n", dlerror());
        exit(1);
    }
    initBuzzer_fp = dlsym(buzzer_handle, "initBuzzer");
    startBuzzer_fp = dlsym(buzzer_handle, "startBuzzer");
    stopBuzzer_fp = dlsym(buzzer_handle, "stopBuzzer");
}

void load_countdown_library() {
    countdown_handle = dlopen("/home/iam/smart_sleep_light/lib/libcountdown.so", RTLD_LAZY);
    if (!countdown_handle) {
        fprintf(stderr, "[ERROR] Failed to load libcountdown.so: %s\n", dlerror());
        exit(1);
    }
    startCountdown_fp = dlsym(countdown_handle, "startCountdown");
}

void close_libraries() {
    if (led_handle) dlclose(led_handle);
    if (light_handle) dlclose(light_handle);
    if (buzzer_handle) dlclose(buzzer_handle);
    if (countdown_handle) dlclose(countdown_handle);
}



// ===== 장치 제어 스레드 =====

void* lightControlThread(void* arg) {
    while (!stop_auto_mode) {
        int value = readLightSensor_fp(LIGHT_SENSOR_PIN);
        status.light = (value == 1) ? LIGHT_DARK : LIGHT_BRIGHT;
        if (value == 1)
            turnOnLED_fp(LED_PIN, BRIGHT_HIGH);
        else
            turnOffLED_fp(LED_PIN);
        delay(1000);
    }
    return NULL;
}

void* countdownThreadFunc(void* arg) {
    int start = *((int*)arg);
    free(arg);
    countdownRunning = 1;
    status.countdown_running = 1;
    startCountdown_fp(start);
    countdownRunning = 0;
    status.countdown_running = 0;
    return NULL;
}

// ==== 알람 멜로디 함수 ====
void playAlarmMelody(int pin) {
    softToneCreate(pin);
    alarm_triggered = 1;

    int notes[] = {
        659, 659, 659, 523, 659, 784,
        784, 784, 659, 784, 880, 880, 0
    };
    int durations[] = {
        200, 200, 200, 300, 300, 500,
        200, 200, 300, 300, 500, 500, 0
    };

    while (alarm_triggered) {
        for (int i = 0; notes[i] != 0; i++) {
            softToneWrite(pin, notes[i]);
            delay(durations[i]);
            if (!alarm_triggered) break;  // 중간에 끄면 빠르게 종료
        }
    }

    softToneWrite(pin, 0);  // 소리 정지
}


void* alarmWatcher(void* arg) {
    while (1) {
        if (alarm_hour == -1 || alarm_min == -1 || alarm_triggered) {
            sleep(30);
            continue;
        }

        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        if (t->tm_hour == alarm_hour && t->tm_min == alarm_min) {
            playAlarmMelody(BUZZER_PIN);  // 전용 멜로디 재생
        }

        sleep(30);
    }
    return NULL;
}

// ===== 명령 처리 함수 =====
void handleCommand(int client_fd, const char* command) {
    if (strcmp(command, "LED ON HIGH") == 0) {
        auto_mode = 0;
        turnOnLED_fp(LED_PIN, BRIGHT_HIGH);
        status.led = LED_HIGH;
        send(client_fd, "OK\n", 3, 0);
    } else if (strcmp(command, "LED ON MEDIUM") == 0) {
        auto_mode = 0;
        turnOnLED_fp(LED_PIN, BRIGHT_MEDIUM);
        status.led = LED_MEDIUM;
        send(client_fd, "OK\n", 3, 0);
    } else if (strcmp(command, "LED ON LOW") == 0) {
        auto_mode = 0;
        turnOnLED_fp(LED_PIN, BRIGHT_LOW);
        status.led = LED_LOW;
        send(client_fd, "OK\n", 3, 0);
    } else if (strcmp(command, "LED OFF") == 0) {
        auto_mode = 0;
        stop_auto_mode = 1;
        turnOffLED_fp(LED_PIN);
        status.led = LED_OFF;
        send(client_fd, "OK\n", 3, 0);
    } else if (strcmp(command, "LIGHT AUTO") == 0) {
        if (!auto_mode) {
            auto_mode = 1;
            stop_auto_mode = 0;
            pthread_create(&light_thread, NULL, lightControlThread, NULL);
            status.auto_mode = 1;
        }
        send(client_fd, "OK\n", 3, 0);
    } else if (strcmp(command, "LIGHT AUTO OFF") == 0) {
        stop_auto_mode = 1;
        auto_mode = 0;
        status.auto_mode = 0;
        turnOffLED_fp(LED_PIN);
        status.led = LED_OFF;
        send(client_fd, "Auto Mode OFF\n", 15, 0);
    } else if (strcmp(command, "BUZZER ON") == 0) {
        if (alarm_triggered) {
            send(client_fd, "Cannot start buzzer while alarm is active. Use STOP ALARM.\n", 59, 0);
        } else {
            startBuzzer_fp(BUZZER_PIN);
            send(client_fd, "OK\n", 3, 0);
        }
    } else if (strcmp(command, "BUZZER OFF") == 0) {
        if (alarm_triggered) {
            send(client_fd, "Cannot stop buzzer manually while alarm is active. Use STOP ALARM.\n", 71, 0);
        } else {
            stopBuzzer_fp();
            send(client_fd, "OK\n", 3, 0);
        }
    } else if (strncmp(command, "COUNTDOWN ", 10) == 0) {
        int val = atoi(command + 10);
        if (val >= 0 && val <= 9 && !countdownRunning) {
            int* arg = malloc(sizeof(int));
            *arg = val;
            pthread_create(&countdownThread, NULL, countdownThreadFunc, arg);
            send(client_fd, "OK\n", 3, 0);
        } else {
            send(client_fd, "Countdown invalid or already running\n", 38, 0);
        }
    } else if (strcmp(command, "ALARM STOP") == 0) {
        stopBuzzer_fp();
        alarm_triggered = 0;
        send(client_fd, "Alarm stopped\n", 15, 0);
    } else if (strcmp(command, "ALARM CLEAR") == 0) {
        alarm_hour = -1;
        alarm_min = -1;
        alarm_triggered = 0;
        send(client_fd, "Alarm cleared\n", 15, 0);
    } else if (strncmp(command, "ALARM ", 6) == 0) {
        int hh, mm;
        if (sscanf(command + 6, "%d:%d", &hh, &mm) == 2 &&
            hh >= 0 && hh <= 23 && mm >= 0 && mm <= 59) {
            alarm_hour = hh;
            alarm_min = mm;
            alarm_triggered = 0;
            send(client_fd, "Alarm set\n", 10, 0);
        } else {
            send(client_fd, "Invalid time format\n", 21, 0);
        }
    } else if (strcmp(command, "STATUS") == 0) {
        char response[256];
        snprintf(response, sizeof(response),
            "LED: %s\nAutoMode: %s\nLight: %s\nCountdown: %s\n",
            status.led == LED_OFF ? "OFF" :
            status.led == LED_LOW ? "LOW" :
            status.led == LED_MEDIUM ? "MEDIUM" : "HIGH",
            status.auto_mode ? "ON" : "OFF",
            status.light == LIGHT_DARK ? "DARK" : "BRIGHT",
            status.countdown_running ? "ON" : "OFF"
        );
        if (alarm_hour >= 0 && alarm_min >= 0) {
            char alarm_line[64];
            snprintf(alarm_line, sizeof(alarm_line),
                "Alarm: %02d:%02d (%s)\n", alarm_hour, alarm_min,
                alarm_triggered ? "TRIGGERED" : "SET");
            strcat(response, alarm_line);
        }
        send(client_fd, response, strlen(response), 0);
    } else {
        send(client_fd, "Unknown command\n", 17, 0);
    }
}

// ==== 시그널 처리 ====
void handleSigint(int sig) {
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (client_socks[i] > 0) close(client_socks[i]);
    if (server_fd > 0) close(server_fd);
    stop_auto_mode = 1;
    if (auto_mode) pthread_join(light_thread, NULL);
    if (countdownRunning) pthread_join(countdownThread, NULL);
    close_libraries();
    printf("Shutdown complete\n");
    exit(0);
}

// ==== 메인 함수 ====
int main() {
    daemonize();
    signal(SIGINT, handleSigint);

    load_led_library();
    load_light_library();
    load_buzzer_library();
    load_countdown_library();

    initLED_fp(LED_PIN);
    initLightSensor_fp(LIGHT_SENSOR_PIN);
    initBuzzer_fp(BUZZER_PIN);

    status.led = LED_OFF;
    status.auto_mode = 0;
    status.countdown_running = 0;
    status.light = (readLightSensor_fp(LIGHT_SENSOR_PIN) == 1) ? LIGHT_DARK : LIGHT_BRIGHT;
    
    pthread_create(&alarmThread, NULL, alarmWatcher, NULL);

    struct sockaddr_in server_addr;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    for (int i = 0; i < MAX_CLIENTS; i++) client_socks[i] = -1;

    fd_set read_fds;
    char buffer[BUFFER_SIZE];

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = client_socks[i];
            if (fd > 0) {
                FD_SET(fd, &read_fds);
                if (fd > max_fd) max_fd = fd;
            }
        }

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
            continue;

        if (FD_ISSET(server_fd, &read_fds)) {
            int new_sock = accept(server_fd, NULL, NULL);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socks[i] < 0) {
                    client_socks[i] = new_sock;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_socks[i];
            if (sd > 0 && FD_ISSET(sd, &read_fds)) {
                int n = recv(sd, buffer, BUFFER_SIZE - 1, 0);
                if (n <= 0) {
                    close(sd);
                    client_socks[i] = -1;
                } else {
                    buffer[n] = '\0';
                    handleCommand(sd, buffer);
                }
            }
        }
    }
}
