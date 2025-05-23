# 🌙 Smart Sleep Light 프로젝트

조도센서, LED, 부저, 7세그먼트를 활용한 **스마트 조명 및 알람 시스템**입니다.  
TCP 기반으로 클라이언트와 서버가 통신하며, **알람 기능**, **카운트다운 타이머**, **자동 조명 제어** 등의 기능을 제공합니다.


## 📁 프로젝트 구조

```
smart_sleep_light/
├── client/   
│   ├── src/           # 클라이언트 소스코드
│   ├── CMakeLists.txt
│   └── Makefile
│   server/              
│   ├── src/                # 서버 소스코드
│   ├── include/
│   ├── lib/
│   ├── CMakeLists.txt
│   └── Makefile
└── README.md
```




## 🧱 사전 준비

### 필수 패키지 설치

```bash
sudo apt update
sudo apt install build-essential cmake wiringpi
```



## 🛠️ 빌드 방법

### 클라이언트 빌드

```bash
cd client
make
```

### 서버 빌드

```bash
cd server
make
```

### 빌드 정리

```bash
make clean
```
> make clean 명령은 생성된 실행파일 (client, server)을 삭제합니다.

## ⏰ 서버 시간대 설정 (KST)

알람 기능은 서버 시스템 시간(KST 기준)을 기준으로 작동합니다.

```bash
sudo timedatectl set-timezone Asia/Seoul
```


## 🖥️ 서버 실행 (.service 등록)

### 1. systemd 서비스 파일 작성

`/etc/systemd/system/smart_sleep_light.service` 파일을 아래와 같이 작성하세요:

```ini
[Unit]
Description=Smart Sleep Light TCP Server
After=network.target

[Service]
Type=forking
ExecStart=/home/iam/smart_sleep_light/build/server
PIDFile=/tmp/smart_sleep_light.pid
Restart=always
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

> `ExecStart` 경로는 실제 server 실행파일 위치에 맞게 수정하세요.

---

### 2. 서비스 등록 및 실행

```bash
sudo systemctl daemon-reexec
sudo systemctl daemon-reload
sudo systemctl enable smart_sleep_light.service
sudo systemctl start smart_sleep_light.service
```

### 3. 상태 확인

```bash
sudo systemctl status smart_sleep_light.service
```

### 4. 로그 보기

```bash
sudo journalctl -u smart_sleep_light.service -n 50
```

## 🔌 프로그램 실행 방법

### ✅ 서버 실행

서버는 systemd 서비스를 통해 자동 실행됩니다:

```bash
sudo systemctl start smart_sleep_light.service
```

- 서버는 백그라운드에서 데몬 형태로 실행됩니다.
- 부팅 시 자동 실행되도록 설정했다면, 별도 명령 없이 자동 시작됩니다.

⚠️ 테스트 용도로 수동 실행하고 싶은 경우:

```bash
cd server/build
./server
```

직접 실행 시에도 `server.c` 내부에서 자동으로 데몬화되므로,  
화면에 출력되지 않고 백그라운드에서 실행됩니다.  
정상 실행 여부는 `ps`, `netstat`, `journalctl` 등을 통해 확인할 수 있습니다.

```bash
ps aux | grep server
netstat -an | grep 5100
sudo journalctl -u smart_sleep_light.service -n 20
```

---

### ✅ 클라이언트 실행

클라이언트는 빌드 후 다음과 같이 실행합니다:

```bash
cd client/build
./client
```

- 실행하면 메뉴가 출력되고, 숫자 선택을 통해 서버에 명령을 전송할 수 있습니다.
- 클라이언트는 **오직 Ctrl+C (SIGINT)** 를 통해 종료됩니다.
- 기타 종료 신호 (`SIGTERM`, `SIGQUIT` 등)는 무시됩니다.

