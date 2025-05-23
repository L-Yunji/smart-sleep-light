cmake_minimum_required(VERSION 3.10)
project(smart_sleep_light C)

set(CMAKE_C_STANDARD 99)
include_directories(include)

# 공유 라이브러리들 빌드
add_library(led SHARED src/led.c)
add_library(light SHARED src/light.c)
add_library(buzzer SHARED src/buzzer.c)
add_library(countdown SHARED src/countdown.c)

set_target_properties(led light buzzer countdown PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
)

# server는 실행 파일만 생성
add_executable(server server/server.c)
target_link_libraries(server wiringPi pthread dl)

set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)
