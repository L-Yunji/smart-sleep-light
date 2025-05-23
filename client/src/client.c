#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 5100
#define SERVER_IP "192.168.0.82"
#define BUFFER_SIZE 1024

const char* COMMANDS[] = {
    "LED ON HIGH",
    "LED ON MEDIUM",
    "LED ON LOW",
    "LED OFF",
    "LIGHT AUTO",
    "LIGHT AUTO OFF",
    "BUZZER ON",
    "BUZZER OFF",
    "COUNTDOWN",
    "exit",
    "STATUS",
    "ALARM",
    "ALARM STOP",
    "ALARM CLEAR"
};

void printMenu() {
    printf("\n========== Smart Sleep Light Client ==========\n");

    printf("\n-- [LED 제어] -------------------------------\n");
    printf(" [1]  LED ON HIGH       [2]  LED ON MEDIUM\n");
    printf(" [3]  LED ON LOW        [4]  LED OFF\n");

    printf("\n-- [조도센서 기반 자동 제어] ---------------\n");
    printf(" [5]  LIGHT AUTO ON     [6]  LIGHT AUTO OFF\n");

    printf("\n-- [BUZZER 제어] ----------------------------\n");
    printf(" [7]  BUZZER ON         [8]  BUZZER OFF\n");

    printf("\n-- [타이머 기능] ----------------------------\n");
    printf(" [9]  COUNTDOWN (0~9)\n");

    printf("\n-- [알람 기능] ------------------------------\n");
    printf(" [10] SET ALARM         → 입력 형식: HH MM (예: 07 30)\n");
    printf(" [11] STOP ALARM\n");
    printf(" [12] CLEAR ALARM\n");

    printf("\n-- [기타] ----------------------------------\n");
    printf(" [13] STATUS            [14] EXIT\n");

    printf("==============================================\n");
    printf("(종료하려면 Ctrl+C 를 누르세요)\n");
}

void sigint_handler(int sig) {
    printf("\n[INFO] 프로그램을 종료합니다.\n");
    exit(0);
}

void ignore_signal(int sig) {
    // 무시
}

int main() {
    signal(SIGINT, sigint_handler);      // Ctrl+C만 허용
    signal(SIGTERM, ignore_signal);      // 기타 종료 신호 무시
    signal(SIGQUIT, ignore_signal);
    signal(SIGHUP, ignore_signal);

    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    int choice;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket creation failed");
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("invalid address");
        return 1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection failed");
        return 1;
    }

    printf("Connected to server!\n");

    while (1) {
        printMenu();
        printf("Enter your choice (1-14): ");
        fflush(stdout);

        if (scanf("%d", &choice) != 1) {
            printf("[ERROR] Invalid input. Try again.\n");
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');

        if (choice == 14) {
            printf("Exiting client...\n");
            break;
        }

        if (choice >= 1 && choice <= 8) {
            send(sock, COMMANDS[choice - 1], strlen(COMMANDS[choice - 1]), 0);
        } else if (choice == 9) {
            int countdown;
            printf("Enter countdown start value (0~9): ");
            if (scanf("%d", &countdown) != 1 || countdown < 0 || countdown > 9) {
                printf("[ERROR] Invalid countdown value. Must be 0~9.\n");
                while (getchar() != '\n');
                continue;
            }
            while (getchar() != '\n');
            sprintf(buffer, "COUNTDOWN %d", countdown);
            send(sock, buffer, strlen(buffer), 0);
        } else if (choice == 10) {
            int hh, mm;
            printf("Enter alarm time (예: 22 05): ");
            if (scanf("%d %d", &hh, &mm) != 2 || hh < 0 || hh > 23 || mm < 0 || mm > 59) {
                printf("[ERROR] Invalid time.\n");
                while (getchar() != '\n');
                continue;
            }
            while (getchar() != '\n');
            sprintf(buffer, "ALARM %02d:%02d", hh, mm);
            send(sock, buffer, strlen(buffer), 0);
        } else if (choice == 11) {
            send(sock, COMMANDS[12], strlen(COMMANDS[12]), 0);
        } else if (choice == 12) {
            send(sock, COMMANDS[13], strlen(COMMANDS[13]), 0);
        } else if (choice == 13) {
            send(sock, COMMANDS[10], strlen(COMMANDS[10]), 0);
        } else {
            printf("[ERROR] Invalid menu choice. Try 1~14.\n");
            continue;
        }

        int len = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (len > 0) {
            buffer[len] = '\0';
            printf("\n[Server Response]\n%s\n", buffer);
        } else {
            printf("[ERROR] Failed to receive response.\n");
        }
    }

    close(sock);
    return 0;
}
