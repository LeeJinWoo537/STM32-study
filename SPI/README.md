# 🛜 **SPI 통신**
1. **칩 선택 (Chip Select) 제어**
가장 먼저 배우신 **CS(SDA)** 핀을 끄고 켜는 함수입니다. 이건 SPI 전용 함수가 아니라 일반 GPIO 제어 함수를 사용합니다.
- `HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)`
    - `GPIOx`: 포트 이름 (예: `GPIOA`)
    - `GPIO_Pin`: 핀 번호 (예: `GPIO_PIN_4`)
    - `PinState`: `GPIO_PIN_RESET(Low, 0V)` 또는 `GPIO_PIN_SET(High, 3.3V)`

**팁:** 코드 가독성을 위해 `#define CS_LOW() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)` 처럼 이름을 붙여서 씁니다.

-------------------------------------------------------------------------------------------------------------------------------------------

2. **데이터 송수신 핵심 함수 (The Big 3)**
STM32 HAL 라이브러리에서 SPI 데이터 전송을 위해 가장 많이 쓰는 함수 3가지입니다.

#### ① **데이터를 보내기만 할 때**
`HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)`
- **언제 쓰나:** RC522의 특정 방에 값을 저장(Write)할 때.
- `hspi`: SPI 핸들러 (보통 `&hspi1`)
- `pData`: 보낼 데이터가 담긴 변수의 주소 (배열이나 `&val`)
- `Size`: 몇 바이트 보낼 건지
- `Timeout`: 전송 완료까지 기다려줄 시간 (보통 `10~100ms`)

<br>

#### ② **데이터를 받기만 할 때**
`HAL_SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)`
- **언제 쓰나:** 주소를 먼저 보낸 직후, RC522가 주는 값을 받아올 때.
- 사용법은 Transmit과 같지만, pData에 받은 값이 저장됩니다.

#### ③ **보내면서 동시에 받을 때 (가장 정석)**
`HAL_SPI_TransmitReceive(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout)`
- **언제 쓰나:** SPI는 구조적으로 데이터를 보내는 동시에 데이터를 받습니다. 아주 정밀한 제어가 필요할 때 씁니다.

-----------------------------------------------------------------------------------------------------------------------------------

3. **실제 사용 예시 (RC522 레지스터 읽기 시나리오)**
이 함수들을 조합하면 아까 보셨던 "레지스터 읽기" 코드가 완성됩니다.

```C
uint8_t read_val;
uint8_t addr = 0x02 | 0x80; // 주소 + 읽기 비트 설정

// 1. 대화 시작
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); 

// 2. "나 0x02번 방(addr) 보고 싶어" 라고 말하기 (전송)
HAL_SPI_Transmit(&hspi1, &addr, 1, 10);

// 3. "알겠어 여기 있어" 라고 주는 값 받기 (수신)
HAL_SPI_Receive(&hspi1, &read_val, 1, 10);

// 4. 대화 종료
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
```

-----------------------------------------------------------------------------------------------------------------------------------

### **핀 번호 예시!**

| **STM32 핀 번호** | **설정 (Configuration)** | **RC522 핀 이름** | **역할** |
| :--- | :--- | :--- | :--- |
| **PA4** | **GPIO_Output** | **SDA (SS)** | 슬레이브 선택 (Low: 활성 / High: 대기) |
| **PA5** | **SPI1_SCK** | **SCK** | SPI 통신용 클럭 신호 |
| **PA6** | **SPI1_MISO** | **MISO** | 마스터 수신 (RC522 -> STM32) |
| **PA7** | **SPI1_MOSI** | **MOSI** | 마스터 송신 (STM32 -> RC522) |
| **PB0** | **GPIO_Output** | **RST** | 모듈 하드웨어 리셋 (초기화) |
| **3.3V** | **Power** | **3.3V** | 메인 전원 (5V 연결 주의) |
| **GND** | **Ground** | **GND** | 접지 |


-----------------------------------------------------------------------------------------------------------------
<br><br>

1. **SPI 데이터 송수신 핵심 코드**
STM32에서 SPI로 1바이트를 주고받을 때는 `HAL_SPI_TransmitReceive` 함수를 주로 사용합니다.

```c
// SPI 핸들러 (CubeMX에서 생성됨)
extern SPI_HandleTypeDef hspi1;

// CS(SDA) 핀 제어를 위한 매크로 (PA4 사용 가정)
#define CS_LOW()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define CS_HIGH() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)

// RC522 특정 레지스터에 1바이트를 쓰는 함수 예시
void RC522_WriteRegister(uint8_t addr, uint8_t val) {
    CS_LOW(); // 통신 시작 (칩 선택)

    // 주소 전송 (RC522 주소 포맷에 맞춰 가공 필요)
    uint8_t addr_byte = (addr << 1) & 0x7E; 
    HAL_SPI_Transmit(&hspi1, &addr_byte, 1, 100);
    
    // 값 전송
    HAL_SPI_Transmit(&hspi1, &val, 1, 100);

    CS_HIGH(); // 통신 종료
}

// RC522 특정 레지스터에서 1바이트를 읽는 함수 예시
uint8_t RC522_ReadRegister(uint8_t addr) {
    uint8_t val;
    CS_LOW();

    // 주소 전송 (읽기 모드 비트 설정)
    uint8_t addr_byte = ((addr << 1) & 0x7E) | 0x80;
    HAL_SPI_Transmit(&hspi1, &addr_byte, 1, 100);
    
    // 데이터 읽기 (더미 데이터를 보내면서 값을 받아옴)
    HAL_SPI_Receive(&hspi1, &val, 1, 100);

    CS_HIGH();
    return val;
}
```

2. **코드에서 사용법 (HAL 라이브러리)**
이렇게 설정하면 코드에서 다음과 같이 RC522를 리셋할 수 있습니다.

```c
// RC522 리셋 (Low로 만들었다가 다시 High로)
HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
HAL_Delay(10); // 잠시 대기
HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
```

SPI 통신은 여러 개의 기기(슬레이브)가 하나의 버스에 연결될 수 있습니다. 이때 MCU가 **"야, 너랑 말할 거야!"**라고 지목하는 방법이 바로 CS 핀을 **0(Low)**으로 떨어뜨리는 것입니다.
- **CS Low (0V):** "지금부터 너랑 통신할게. 준비해!" **(통신 시작 / 선택됨)**
- **CS High (3.3V):** "이제 볼일 끝났어. 쉬어." **(통신 종료 / 선택 해제)**

그래서 코드에서 `CS_HIGH()`를 호출하는 것은 해당 기기와의 대화를 끝내고 기기를 '대기 상태'로 돌려보내는 것을 의미합니다.