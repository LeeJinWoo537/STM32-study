# STM32 물 수위 센서 ADC 프로젝트 (ADC_wer)

STM32F411 보드를 사용한 물 수위 센서 ADC 읽기 및 UART 출력 프로젝트입니다.

## 프로젝트 개요

이 프로젝트는 기존 ADC_wer STM32IDE 프로젝트를 수정하여 물 수위 센서의 아날로그 값을 ADC를 통해 읽어들이고, UART를 통해 실시간으로 출력하는 시스템입니다.

## 하드웨어 연결

### 물 수위 센서 연결
- **센서 VCC**: 3.3V 또는 5V (센서 사양에 따라)
- **센서 GND**: GND
- **센서 아날로그 출력**: PA0 (ADC1_IN0)

### UART 연결 (디버깅용)
- **USART2_TX**: PA2
- **USART2_RX**: PA3
- **보드레이트**: 115200 bps

### LED 연결
- **PA5**: 내장 LED (상태 표시용)

## 주요 기능

1. **ADC 변환**: 물 수위 센서의 아날로그 값을 12비트 디지털 값으로 변환
2. **전압 변환**: ADC 값을 실제 전압으로 변환 (0-3.3V)
3. **수위 계산**: 전압을 기반으로 물 수위를 퍼센트로 계산
4. **UART 출력**: 센서 값, 전압, 수위 퍼센트를 실시간으로 출력
5. **1초 간격**: 매 1초마다 센서 값을 읽고 출력

## 출력 형식

```
STM32 Water Level Sensor Started
Reading water level every 1 second...

ADC Value: 2048
Voltage: 1.65 V
Water Level: 50.0%
--------------------------------
ADC Value: 2100
Voltage: 1.69 V
Water Level: 51.2%
--------------------------------
```

## 코드 구조

### 주요 함수

- `WaterLevel_Read()`: ADC를 통해 센서 값을 읽고 전압 및 수위 퍼센트 계산
- `WaterLevel_Print()`: UART를 통해 측정값들을 포맷팅하여 출력
- `MX_ADC1_Init()`: ADC1 초기화 (PA0 채널)
- `MX_USART2_UART_Init()`: UART2 초기화 (115200 bps)

### 설정값

- **ADC 해상도**: 12비트 (0-4095)
- **참조 전압**: 3.3V
- **ADC 채널**: ADC1_IN0 (PA0)
- **샘플링 시간**: 3 사이클
- **MCU**: STM32F411RET6

## 센서 보정

센서의 특성에 따라 수위 계산 공식을 조정해야 할 수 있습니다:

```c
// 현재 공식 (선형 변환)
water_level_percentage = (voltage / VREF) * 100.0;

// 센서별 보정 예시
// 예: 최소 전압이 0.5V인 경우
water_level_percentage = ((voltage - 0.5) / (VREF - 0.5)) * 100.0;
```

## 컴파일 및 업로드

### STM32IDE에서 사용하는 경우:

1. STM32IDE 1.18.1을 실행
2. File → Open Projects from File System
3. ADC_wer 프로젝트 폴더 선택
4. 프로젝트를 빌드 (Ctrl+B)
5. STM32 보드에 업로드

### 프로젝트 구조:
```
ADC_wer/
├── Core/
│   ├── Inc/
│   │   └── main.h
│   └── Src/
│       ├── main.c
│       ├── stm32f4xx_hal_msp.c
│       ├── stm32f4xx_it.c
│       ├── syscalls.c
│       ├── sysmem.c
│       └── system_stm32f4xx.c
├── Drivers/
├── ADC_wer.ioc
└── README.md
```

## 필요한 라이브러리

- STM32F4xx HAL Driver
- CMSIS (Cortex Microcontroller Software Interface Standard)

## 주의사항

1. **센서 전원**: 센서의 동작 전압을 확인하고 적절한 전원을 공급하세요
2. **신호 레벨**: 센서 출력이 3.3V를 초과하지 않도록 주의하세요
3. **연결**: 센서 연결 시 접촉 불량을 방지하세요
4. **보정**: 실제 사용 환경에서 센서 보정이 필요할 수 있습니다

## 문제 해결

### 센서 값이 변하지 않는 경우:
- 센서 연결 확인
- 전원 공급 확인
- ADC 채널 설정 확인 (PA0)

### UART 출력이 없는 경우:
- UART 연결 확인 (PA2, PA3)
- 보드레이트 설정 확인 (115200 bps)
- 터미널 프로그램 설정 확인

### 부정확한 수위 측정:
- 센서 보정 필요
- 참조 전압 확인
- 센서 특성 데이터 시트 확인

## 확장 가능성

- 다중 센서 지원
- LCD 디스플레이 추가
- 데이터 로깅 기능
- 무선 통신 (WiFi/Bluetooth)
- 웹 인터페이스
- 알람 기능 (수위 임계값)

## 수정된 파일들

1. **main.c**: 물 수위 센서 읽기 및 출력 기능 추가
2. **stm32f4xx_hal_msp.c**: ADC 핀 설정 추가

## 라이선스

이 프로젝트는 교육 및 개인 사용 목적으로 제공됩니다.
