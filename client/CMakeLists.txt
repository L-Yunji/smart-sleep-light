cmake_minimum_required(VERSION 3.10)
project(SmartSleepLight C)

set(CMAKE_C_STANDARD 99)

# 클라이언트 빌드 설정
add_executable(client src/client.c)
