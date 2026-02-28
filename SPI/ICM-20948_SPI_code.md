# 🧩 **ICM-20948 센서 코드**

## **SPI 통신 설정**
```C
/* SPI1 Mode and Configuration */
============ Mode ===============
Mode:                   Full-Duplex Master
Hardware NSS Signal:    Disable


/* Configuration */
======= Basic Parameters ===========
Frame Format:    Motorola
Data Size:       8 Bits
First Bit:       MSB First


======== Clock Parameters ===========
Presacaler (for Baud Rate):     64
Baud Rate:                      1.3125 MBits/s
Clock Polarity (CPOL):          LOW
Clock Phase (CPHA):             1 Edge


======= Advanced Parameters =========
CRC Calculation:    Disabled
NSS Signal Type:    Software
```

### **Mode (모드 설정)**
#### **Full-Duplex Master**
- Full-Duplex: 한 번에 송신과 수신을 동시에 할 수 있는 방식.
- Master: 이 칩이 클럭(SCK)을 만들고 통신을 시작하는 쪽. 슬레이브를 선택하고 데이터를 보내고 받는 주체.

#### **Hardware NSS Signal: Disable**
- NSS (Slave Select): 어떤 슬레이브와 통신할지 선택하는 신호.
- Disable: NSS를 하드웨어가 자동으로 제어하지 않고, 소프트웨어로 GPIO를 직접 제어해서 슬레이브를 선택한다는 뜻. (SPI 하드웨어는 NSS 신호에 관여하지 마라) 자동으로 관여하지 마!

> SPI 하드웨어가 CS(NSS) 핀을 자동으로 제어할 거냐?

를 정하는 옵션이야.

#### **CS 핀은 완전한 일반 GPIO**
코드에서 직접 제어
```C
CS LOW   // 통신 시작
SPI 송수신
CS HIGH  // 통신 종료
```
#### **왜 센서랑 쓸 때 이게 정답이냐면**
- 센서는 CS 타이밍이 엄청 민감
- 주소 + 데이터 동안 CS가 계속 LOW여야 함
- 자동 제어는 타이밍이 끊길 수 있음

👉 **센서 통신 = Disable가 정석**


### 🔹 **Hardware NSS Input Signal**
### **의미**
> ❝ 외부에서 들어오는 NSS 신호를 감시하겠다 ❞

### **이건 언제 쓰냐**
- STM32가 SPI Slave일 때
- 다른 MCU가 Master로서 CS를 내려줌
```Plain text
외부 Master ──CS──▶ STM32 (Slave)
```
#### **지금 상황에서는?**
- STM32 = Master
- ICM-20948 = Slave

👉 **완전히 반대 구조 → 절대 아님**

### 🔹 **Hardware NSS Output Signal**
#### **의미**
> ❝ STM32 하드웨어가 CS를 자동으로 내려주고 올려주겠다 ❞

### **내부적으로 무슨 일이 생기냐면**
- SPI 프레임 단위로 NSS를 자동 토글
- TX/RX 중간에도 상태 바뀔 수 있음

#### **센서 입장에서 보면**
```
CS LOW → 주소 보냄
CS HIGH → ????
CS LOW → 데이터?
```
👉 센서: “이건 새로운 명령인데?” 🤯         <br>
👉 레지스터 접근 전부 깨짐

그래서:
> **Flash, IMU, 디스플레이 같은 SPI Slave + 센서 계열은 거의 전부 사용 금지**

### 2️⃣ **Disable (지금 네 설정)**
👉 **하드웨어 자동 제어를 안 쓰겠다는 뜻**

즉:
- SPI 모듈이 NSS 핀을 자동으로 제어하지 않음
- 대신 GPIO로 직접 제어해야 함

보통 이렇게 사용해:
```C
HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);  // CS Low
HAL_SPI_Transmit(&hspi1, data, size, 100);
HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);    // CS High
```
이걸 **Software NSS 관리**라고 해.

### 🔷 **왜 보통 Disable로 쓰냐?**
대부분의 센서 (예: MPU9250 같은 SPI 센서들)는       <br>
📌 CS 타이밍을 개발자가 직접 제어하는 게 더 안정적이기 때문이야.

하드웨어 NSS는:
- 한 프레임 단위로만 자동 제어
- 연속 전송 제어가 애매함
- DMA 사용 시 타이밍 문제 발생 가능

그래서 실제 프로젝트에서는 거의 다:

> ✅ Hardware NSS: Disable          <br>
> ✅ GPIO로 CS 직접 제어

이렇게 씀.

### 🔷 1️⃣ **Disable**
👉 **NSS를 SPI 하드웨어가 전혀 건드리지 않음**

NSS 핀을 SPI 기능으로도 안 씀

보통 GPIO Output으로 설정해서 CS 직접 제어

가장 흔한 설정

📌 사용 상황

Master 모드

센서 / 디스플레이 / 외부 IC 제어
```
CS Low  → SPI 전송 → CS High
```
### 🔷 2️⃣ **Hardware NSS Input Signal**

👉 NSS를 “입력”으로 사용

즉,
📌 외부에서 들어오는 NSS 신호를 감시

언제 쓰냐?

➡ SPI Slave 모드일 때

외부 Master가 NSS를 Low로 내리면

SPI 하드웨어가 자동으로 “선택됨” 상태 인식

NSS가 High면

SPI 비활성 상태

특징

NSS 핀은 GPIO 입력이 아님

SPI 하드웨어 전용 핀으로 동작

소프트웨어로 제어 불가

📌 예:

외부 MCU (Master)
 └── NSS ───> STM32 (Slave)
🔷 3️⃣ Hardware NSS Output Signal

👉 NSS를 “출력”으로 사용

즉,
📌 STM32가 Master가 되어 NSS를 자동으로 제어

동작 방식

SPI 전송 시작 → NSS 자동 Low

SPI 전송 종료 → NSS 자동 High

프레임 단위로 제어됨

제한 사항 (중요 ⚠️)

한 Slave만 가능

프레임 단위 제어라서
여러 레지스터 연속 접근에 불리

DMA 사용 시 타이밍 문제 발생 가능

📌 예:
```C
STM32 (Master)
 └── NSS ───> 외부 Slave
```

<br>

------------------------------------------------------------------------------------------------------------------------------
### 🔹 **Frame Format: Motorola**
👉 **“SPI 통신의 규칙(프로토콜) 종류”**야

SPI라고 다 같은 SPI가 아니고,       <br>
**프레임을 어떻게 주고받을지에 대한 약속**이 몇 가지 있어.

그중에 STM32가 지원하는 대표적인 게 이 두 개야:

1. **Motorola SPI** ✅
2. TI SPI (Texas Instruments 방식)

### 🔸 **Motorola SPI가 뭐냐면**
#### **한 줄 요약**
> **우리가 흔히 말하는 ‘일반적인 SPI’ = Motorola SPI**

ICM-20948, MPU9250, 플래시 메모리, LCD, 대부분의 센서들이       <br>
👉 **전부 Motorola SPI 방식**을 사용해.

### 🔹 **Motorola SPI의 특징 (핵심만)**
#### 1️⃣ **신호선 4개**
```Plain text
CS   (Chip Select)
SCK  (Clock)
MOSI (Master → Slave)
MISO (Slave → Master)
```

이건 네가 지금 쓰는 SPI랑 완전히 같지?      <br>
👉 이게 바로 Motorola SPI 구조야

### 2️⃣ **데이터 흐름 방식**
- 클럭(SCK)은 Master(STM32) 가 만든다
- 클럭 엣지마다:
    - MOSI에서 데이터 나감
    - MISO에서 데이터 들어옴

📌 이 동작 규칙 자체가 Motorola SPI의 정의

### 3️⃣ **주소 + 데이터 방식**
센서 SPI는 보통 이렇게 동작해:
```Plain text
[ 레지스터 주소 ] → [ 데이터 ]
```
예:
```Plain text
0x75 → WHO_AM_I 레지스터
```
- MSB = Read / Write 비트
- 나머지 7비트 = 주소

📌 ICM-20948도 이 규칙 그대로 사용함        <br>
➡️ Motorola SPI 전형적인 구조

### 🔸 **그럼 TI SPI는 뭐가 다르냐?**
(참고만 하고 넘어가도 돼)
- CS 대신 프레임 동기 신호 사용
- 클럭/데이터 타이밍 다름
- 센서에서는 거의 안 씀
- STM32에서도 거의 안 씀

👉 **센서 SPI**에서는 쓸 일 없음

<br>

### 🔹 **Data Size: 8 Bits**
👉 **“SPI가 한 번에 주고받는 데이터 덩어리의 크기”**

### 🔸 **SPI에서 “한 번”이란?**
SPI는 이렇게 동작해:
```Plain text
SCK:  ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐
      │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │
      └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘
       ↑   ↑   ↑   ↑   ↑   ↑   ↑   ↑
      bit7 bit6 bit5 bit4 bit3 bit2 bit1 bit0
```
👉 **클럭 8번 = 데이터 1개**

이 **1개가 바로 Data Size**야.

### 🔹 **8 Bits란 말의 진짜 의미**
- SPI 클럭 8번 동안
- MOSI / MISO로
- **8비트 데이터 하나**를 전송

예:
```C
uint8_t data = 0x75;   // 0111 0101
HAL_SPI_Transmit(&hspi1, &data, 1, 100);
```
➡️ 위 코드는 **클럭 8번 발생**

### 🔸 **왜 하필 8비트냐?**
### 1️⃣ **센서 레지스터 구조 때문**
ICM-20948 같은 센서는 내부가 이렇게 생겼어:
```Plain text
레지스터 주소: 8비트
레지스터 값  : 8비트
```
예:
```Plain text
0x75 → WHO_AM_I
0xEA → 값
```
📌 **주소도 8비트, 데이터도 8비트**

➡️ SPI도 8비트 단위가 가장 자연스러움

### 2️⃣ **SPI Read / Write 프로토콜**
ICM-20948 SPI Read는 이렇게 생겼어:
```Plain text
[ 1bit R/W | 7bit Address ]  →  8bit
```
예:
```Plain text
1 1110101 = 0xF5
```
👉 이것도 딱 **8비트**

### 🔸 **만약 16 Bits로 설정하면?**
이게 진짜 중요해 ⚠️

#### ❌ **Data Size = 16 Bits 설정 시**
STM32는 이렇게 생각해:
```Plain text
"아, 클럭 16번 동안 하나의 데이터구나"
```
그러면 실제 전송은:
```Plain text
[ 상위 8비트 ] [ 하위 8비트 ]
```
하지만 ❗       <br>
ICM-20948은 이렇게 생각해:
```Plain text
"어? CS는 그대로인데 데이터가 이상하다?"
```
### **결과:**
- 레지스터 주소 해석 실패
- 읽기/쓰기 안 됨
- WHO_AM_I = 0xFF / 0x00 나옴
- 통신되는 것처럼 보이지만 값이 이상함

➡️ **센서는 16비트를 하나의 명령으로 이해하지 못함**

### 🔹 **CS(Chip Select)랑도 연결됨**
센서는 보통 이렇게 인식해:
```Plain text
CS ↓  → "새 명령 시작"
CS ↑  → "명령 끝"
```
- 8비트 단위로 명령을 쪼개야 함
- 16비트로 보내면 **센서가 명령 경계를 못 잡음**

### ❌ **16 Bits**
- 디스플레이, DAC 같은 특수 장치용
- 센서에는 거의 안 씀

<br>

### 🔹 **First Bit: MSB First**
👉 **“8비트 중에서 어떤 비트를 먼저 보낼 것인가”**

### 🔸 **MSB / LSB가 뭐냐면**
예를 들어 이 값이 있다고 해보자:
```Plain text
0xA5 = 1010 0101
        ↑        ↑
       MSB      LSB
```
- **MSB (Most Significant Bit)**        <br>
    → 가장 왼쪽 비트 (bit7)

- **LSB (Least Significant Bit)**       <br>
    → 가장 오른쪽 비트 (bit0)


### 🔹 **MSB First란?**
```Plain text
전송 순서:
bit7 → bit6 → bit5 → bit4 → bit3 → bit2 → bit1 → bit0
```
즉,
```Plain text
1 → 0 → 1 → 0 → 0 → 1 → 0 → 1
```
### 🔸 **이게 SPI 신호에서는 이렇게 됨**
```Plain text
SCK:  ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐
MOSI: 1   0   1   0   0   1   0   1
```
👉 첫 번째 클럭에서 **bit7**이 나감

### 🔹 **왜 MSB First가 기본이냐?**
### 1️⃣ **센서 데이터시트 규칙**
ICM-20948 SPI 명령 구조:
```Plain text
[ R/W 비트 | 주소(7비트) ]
```
이때:
- R/W 비트 = MSB
- 주소는 그 다음 7비트

📌 즉,
```Plain text
bit7 = 읽기/쓰기
bit6~0 = 주소
```
➡️ MSB First 아니면 **명령 자체가 깨짐**

### 2️⃣ **실제 예제로 보면**
WHO_AM_I 읽기:
- 레지스터 주소: 0x75
- 읽기 명령 → MSB = 1
```Plain text
1 1110101 = 0xF5
```
MSB First → 센서가 이렇게 해석:
```Plain text
"아, Read 명령이구나"
```

### 🔸 **만약 LSB First로 설정하면?**
⚠️ 여기서 사고 터짐

### ❌ **LSB First 전송**
```Plain text
0xF5 = 1111 0101
전송 순서:
1 → 0 → 1 → 0 → 1 → 1 → 1 → 1
```
센서 입장:
```Plain text
"이게 뭐야… 주소가 아니잖아?"
```
### **결과**
- R/W 비트 위치 붕괴
- 주소 해석 불가
- 통신은 되는 것처럼 보이지만 값은 쓰레기
- WHO_AM_I가 0xEA가 아니라 이상한 값

### 🔹 **CS랑도 연결됨**
센서는 보통:
```Plain text
CS ↓ → bit7부터 해석 시작
```
즉:
- 첫 비트가 무엇인지가 매우 중요
- MSB First는 이 전제를 만족

<br>

### 🤔 **그러면 앤디안 방식하고 비슷한거 아니야??**
👉 **맞아! 앤디안하고 비슷해 하지만 같은건 아니야!**

### 🔹 1️⃣ **MSB First는 뭐냐?**
👉 **“한 바이트(8비트) 안에서 비트를 어떤 순서로 보낼 것인가”**

예:
```C
0xA5 = 1010 0101
```
MSB First:
```C
1 → 0 → 1 → 0 → 0 → 1 → 0 → 1
```
LSB First:
```C
1 → 0 → 1 → 0 → 0 → 1 → 0 → 1  (거꾸로)
```
- ✔ **비트 전송 순서 설정**
- ✔ SPI 하드웨어 레벨 이야기
- ✔ 클럭 한 번마다 나가는 비트 순서

### 🔹 2️⃣ **엔디안(Endian)은 뭐냐?**
👉 **“여러 바이트(16bit, 32bit)를 메모리에 어떻게 저장할 것인가”**

예를 들어:
```C
0x1234
```

### **Little Endian (STM32)**
메모리:
```C
주소 낮음 → 0x34
주소 높음 → 0x12
Big Endian
주소 낮음 → 0x12
주소 높음 → 0x34
```
- ✔ CPU 메모리 저장 방식
- ✔ 바이트 단위 순서
- ✔ SPI 설정이 아님

<br>

### 🤔 **오케이 이해했어 그러면 LSB First는 왜있는거야??**
SPI는 원래 **Motorola가 만든 통신 규격**인데,       <br>
모든 디지털 시스템이 MSB를 먼저 쓰는 건 아니었어.

예전 일부:
- 쉬프트 레지스터 기반 칩
- 간단한 시리얼 IC
- 특정 DSP
- 일부 RF 모듈

이런 애들은 내부 구조가 **LSB부터 계산/출력**되도록 설계된 경우가 있었어.

👉 그 칩들과 호환하려고 LSB First 옵션이 존재하는 거야.

### 🔹 2️⃣ **하드웨어 구조 때문**
SPI는 내부적으로 이렇게 동작해:
```C
Shift Register
```
데이터를 한 비트씩 밀어내는데,

설계에 따라:
- 왼쪽으로 쉬프트 (MSB First)
- 오른쪽으로 쉬프트 (LSB First)

둘 다 구현이 가능해.

STM32는 범용 MCU라서    <br>
👉 “어떤 장치든 붙일 수 있게” 옵션을 열어둔 것.

### 🔹 3️⃣ **실제로 LSB First 쓰는 경우**
현실에서 거의 없지만, 예외는 있음:
- ✅ 일부 LED 드라이버
- ✅ 일부 오래된 ADC/DAC
- ✅ 특정 RF 트랜시버
- ✅ 특수 산업용 IC

하지만 ❗

👉 IMU, 센서, 플래시 메모리, LCD        <br>
→ 거의 100% MSB First

### 🔹 4️⃣ **이걸 이해하는 핵심 포인트**
MSB First가 기본인 이유는:
- 사람이 숫자를 쓰는 방식이 MSB 기준
- 대부분의 통신 프로토콜이 MSB 기준
- 데이터시트도 MSB 기준으로 설명됨

LSB First는:
> "특정 하드웨어와의 호환성을 위한 옵션"

이야.

### 🔹 5️⃣ **만약 ICM-20948에서 LSB First로 하면?**
예:
```C
보내려는 값: 0x75
01110101
```
LSB First로 보내면:
```C
10101110
```
센서 입장:
```C
주소 완전 다르게 인식
```
→ 통신 망가짐

### 🔥 **결론**
> LSB First는                       <br>
> "특정 특수 장치를 위한 호환 옵션"

> 센서 SPI에서는            <br>
> 거의 사용 안 함

<br>

### 🔹 **Prescaler (for Baud Rate): 64**
👉 **“SPI 클럭(SCK)을 얼마나 나눌 것인가”**

STM32에서 SPI 속도는 **직접 숫자를 넣는 게 아니라,**            <br>
**분주비(Prescaler)** 를 고르는 방식이야.

### 🔸 **SPI 클럭 생성 구조**
STM32F411RE에서 SPI1은:
- APB2 클럭을 입력으로 사용
- 기본값: 84 MHz
```Plain text
SPI Clock = APB2 Clock / Prescaler
```

### 🔹 **Prescaler = 64 의미**
```Plain text
SPI SCK = 84 MHz / 64 = 1.3125 MHz
```
👉 이 값이 **SPI 클럭 속도**

### 🔹 **Baud Rate: 1.3125 Mbits/s**
👉 **“Prescaler를 적용한 결과로 나오는 실제 SPI 전송 속도”**
- STM32CubeIDE가 **자동 계산해서 보여주는 값**
- 네가 직접 설정하는 값 ❌
- **읽기 전용 표시값**이라고 생각하면 됨

### 🔸 **왜 Bit/s냐?**
SPI는:
- 클럭 1번 = 비트 1개 전송

즉:
```Plain text
SCK 1 MHz = 1 Mbit/s
```
그래서 **클럭 주파수 = Baud Rate**

### 🔹 **이 둘의 관계 한 줄 요약**
```Plain text
Prescaler → 원인
Baud Rate → 결과
```

### 🔹 **왜 Prescaler를 직접 고르게 하냐?**
### 1️⃣ **하드웨어 한계 때문**
STM32 SPI는 **2의 배수 분주기만 지원**
```Plain text
2, 4, 8, 16, 32, 64, 128, 256
```
연속적인 숫자 ❌

### 2️⃣ **Slave(센서) 최대 속도 때문**
ICM-20948 SPI 속도 제한:
- Read: 최대 7 MHz
- Write: 권장 ≤ 1 MHz

👉 처음엔 **64**가 아주 안전

### 🔹 **만약 Prescaler를 바꾸면?**

| **Prescaler** | **SPI 속도** |
| :--- | :--- |
| 128 | 656 kHz |
| 64 | 1.3125 MHz |
| 32 | 2.625 MHz |
| 16 | 5.25 MHz |
| 8 | 10.5 MHz ❌ (위험) |

📌 32나 16은 나중에 OK      <br>
📌 8은 센서 사양 초과 가능

### 🔹 **실제 파형 느낌**
Prescaler = 64:
```Plain text
SCK: ┌─┐   ┌─┐   ┌─┐   ┌─┐
     └─┘   └─┘   └─┘   └─┘
```
Prescaler = 8:
```Plain text
SCK: ┌┐┌┐┌┐┌┐┌┐┌┐ (너무 빠름)
```

### 🔥 **핵심 정리**
> **Prescaler는 원인, Baud Rate는 결과**

> SPI 속도는        <br>
> `APB 클럭 ÷ Prescaler`

<br>

### 🔹 **Clock Polarity (CPOL)**
👉 **“SPI 클럭(SCK)이 아무 일도 안 할 때 기본 상태가 HIGH냐 LOW냐”**

즉,

> **유휴(idle) 상태의 클럭 레벨**

### 🔸 **CPOL = LOW (0)**
```Plain text
SCK:  ────────┐   ┌───────
              └───┘
```
- 통신 시작 전 / 끝난 후
- SCK가 **LOW로 가만히 있음**
- 첫 변화는 **LOW → HIGH**

📌 네 설정이 바로 이거야

### 🔸 **CPOL = HIGH (1)**
```Plain text
SCK:  ────────┘   └───────
              ┌───┐
```
- 평소에는 **HIGH**
- 첫 변화는 **HIGH → LOW**

### 🔹 **왜 이게 중요하냐면**
SPI는:
- 클럭 **엣지(edge)** 에서 데이터를 읽음
- CPOL은 **엣지의 방향을 결정**

즉:
- CPOL이 바뀌면
- **“첫 번째 엣지”가 바뀜**

### 🔹 **CPOL이 파형에 미치는 영향**
### **CPOL = LOW**
```Plain text
Idle: LOW
1st edge: Rising (↑)
2nd edge: Falling (↓)
```

### **CPOL = HIGH**
```Plain text
Idle: HIGH
1st edge: Falling (↓)
2nd edge: Rising (↑)
```

### 🔹 **CPOL 단독으로는 의미가 반쪽**
⚠️ 여기 중요

CPOL 혼자만으로는       <br>
“언제 데이터를 읽는지” 결정되지 않아.

👉 반드시 **CPHA (Clock Phase)** 와 같이 봐야 함.

### 🔹 **CPOL이 왜 존재하냐?**
#### 1️⃣ **하드웨어 호환성**
어떤 장치는:
- 클럭 LOW를 기준으로 설계  <br>
    어떤 장치는:
- 클럭 HIGH를 기준으로 설계

➡️ MCU가 둘 다 지원해야 함

#### 2️⃣ **노이즈/전력 특성**
- 어떤 회로는 HIGH 유휴가 안정적
- 어떤 회로는 LOW 유휴가 안정적

### 🔹 **ICM-20948에서는?**
📌 ICM-20948 SPI 요구:
- CPOL = 0 (LOW)
- CPHA = 0
- 👉 **SPI Mode 0**

네 설정:
- CPOL = LOW ✅
- CPHA = 1 Edge (Cube 기준 = 0) ✅

<br>

### 🔹 **Clock Phase (CPHA)**
👉 **“클럭의 어느 엣지에서 데이터를 읽을 것인가”**

SPI에서 중요한 건 딱 두 가지야:
1. **언제 데이터를 바꾸고**
2. **언제 데이터를 읽느냐**

CPHA는 ‘읽는 타이밍’ 을 정하는 옵션이야.

### 🔸 **엣지(edge) 다시 짚기**
클럭에는 엣지가 두 개 있어:
```Plain text
Rising Edge  (↑) : LOW → HIGH
Falling Edge (↓) : HIGH → LOW
```

### 🔹 **CPHA = 0 (1st Edge)**
```Plain text
데이터 샘플링 → 첫 번째 엣지
데이터 변경   → 두 번째 엣지
```
### **CPOL = LOW일 때**
```Plain text
Idle: LOW
1st edge (↑) → 데이터 읽음
2nd edge (↓) → 데이터 변경
```
📌 **SPI Mode 0**

### 🔹 **CPHA = 1 (2nd Edge)**
```Plain text
데이터 변경   → 첫 번째 엣지
데이터 샘플링 → 두 번째 엣지
```

### **CPOL = LOW일 때**
```Plain text
Idle: LOW
1st edge (↑) → 데이터 변경
2nd edge (↓) → 데이터 읽음
```

### 🔹 **왜 CPHA가 필요하냐?**
### 1️⃣ **데이터 안정 시간 확보**
- 어떤 장치는:
    - 클럭 엣지 직후 데이터가 안정됨
- 어떤 장치는:
    - 다음 엣지에서 안정됨

➡️ 둘 다 지원하려고 CPHA가 있음

### 2️⃣ **고속 통신 안정성**
- 클럭 빠를수록
- 엣지 직후 데이터 흔들림 발생

CPHA=1은:
- **한 반주기 쉬었다 읽음**
- 고속에서 더 안정적인 경우도 있음

### 🔹 **ICM-20948에서 CPHA**
📌 데이터시트 요구:
- CPOL = 0
- CPHA = 0  <br>
    ➡️ **SPI Mode 0**

STM32CubeIDE 설정:
- CPOL: LOW
- CPHA: **1 Edge**      <br>
    (Cube에서 “1 Edge” = CPHA 0)

✔ 정확한 설정

### 🔹 **파형으로 한 방에 정리**
### **Mode 0 (네 설정)**
```Plain text
SCK : ___/‾‾\___/‾‾\___
MOSI: ====<DATA>======
        ↑    ↑
     샘플링  샘플링
```

<br>

### 🔹 **CRC Calculation: Disabled**
#### 👉 **“SPI 전송 중 CRC(에러 체크 코드)를 쓰느냐 마느냐”**
#### 🔸 **CRC가 뭐냐면**
CRC(Cyclic Redundancy Check)는:
- 전송 데이터 뒤에 검사용 비트를 붙여서
- 수신 측에서 데이터 깨졌는지 확인하는 방식

예:
```Plain text
[ 데이터 ] + [ CRC ]
```

### 🔹 **SPI에서 CRC를 쓰면 어떻게 되냐?**
STM32에서 CRC를 Enable하면:
- SPI가 자동으로 CRC 바이트를 하나 더 전송
- 하드웨어가 CRC 계산까지 해줌

문제는 ❗

👉 **ICM-20948은 SPI CRC를 전혀 지원하지 않음**

### 🔸 **만약 Enable하면 생기는 일**
```Plain text
STM32: [ 주소 ][ 데이터 ][ CRC ]
센서:  [ 주소 ][ 데이터 ] ????
```
센서 입장:
```Plain text
"어? 왜 데이터가 하나 더 오지?"
```
### **결과**
- 다음 명령부터 프레임 꼬임
- 레지스터 쓰기 실패
- 읽기 값 이상
- 디버깅 지옥 시작 😅

➡️ **센서 SPI에서는 무조건 Disable**

### 🔹 **언제 CRC를 쓰냐?**
거의 안 씀. 하지만 예외:
- STM32 ↔ STM32 통신
- 고신뢰 산업용 통신
- 두 장치 모두 CRC 지원할 때

### 🔹 **NSS Signal Type: Software**
👉 **“CS(NSS)를 누가 제어하느냐”**

### 🔸 **NSS (CS) 다시 정리**
NSS = Chip Select = CS
- LOW → 통신 시작
- HIGH → 통신 종료

### 🔹 **Software 의미**
```Plain text
SPI 하드웨어가 CS를 자동으로 제어 ❌
사용자가 GPIO로 직접 제어 ⭕
```
예:
```C
HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET); // CS LOW
HAL_SPI_Transmit(&hspi1, &tx, 1, 100);
HAL_SPI_Receive(&hspi1, &rx, 1, 100);
HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);   // CS HIGH
```

### 🔸 **왜 Software가 센서에서는 필수냐?**
#### 1️⃣ **CS 타이밍이 민감함**
ICM-20948은:
- CS LOW 동안 = 하나의 명령
- 중간에 올라가면 명령 끊김

하드웨어 NSS는:
- 전송마다 CS를 깜빡 올렸다 내릴 수 있음 ❌

#### 2️⃣ **멀티바이트 Read**
센서 레지스터 읽을 때:
```Plain text
CS ↓
[ 주소 ]
[ 데이터1 ]
[ 데이터2 ]
[ 데이터3 ]
CS ↑
```
👉 CS를 **길게 유지**해야 함

➡️ Software 아니면 제어 불가

#### 3️⃣ **여러 Slave 사용 시**
SPI 버스 하나에:
- 센서 A
- 센서 B

각각 CS를 **개별 GPIO**로 제어해야 함

### 🔹 **Hardware NSS는 언제 쓰냐?**
거의 안 씀. 예외:
- STM32 ↔ STM32
- 단순 1바이트 통신
- CS 타이밍이 중요하지 않을 때

------------------------------------------------------------------------------