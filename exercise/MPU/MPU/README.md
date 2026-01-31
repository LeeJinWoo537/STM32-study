# 🛜 **I2C 설정 방법과 DMA설정 방법 설명!**
- 일단 핀은 STM32IDE 생성하면 자동으로 PB6, PB7에 I2C(SDA, SCL)이 생성성됨 그거 쓰면 됨

## 😁 **I2C 설정!**
1. ioc파일에 가면 Pinout & Configuration이 있음! 그 밑에
2. categories 클릭하고!
3. 여러가지가 있는데 Connectivity가 있는데 그거 클릭하고 I2C1로 누르면 설정이 나옴!
4. 오른쪽에 Mode가 있는데 거기 보면 `Disable`로 되어있을 텐데 I2C로 바꿈!
5. 그리고 그 밑에 `Configuration` 밑에 보면 `Parameter Settings`가 있으면 그거 누르면
6. 여러가지가 나오는데 일단 이렇게 하면 됨
```stm32
Master Features
    |
    |
    ----I2C Speed Mode = Standard Mode
    |
    ----I2C Clock Speed (Hz) = 100000


Slave Features
    |
    |
    ----Clock No Stretch Mode = Disabled
    |
    |
    ----Primary Address Length selection = 7-bit
    |
    |
    ----Dual Address Acknowledged = Disabled
    |
    |
    ----Primary slave address = 0x68(MPU9250 주소값)
    |
    |
    ----General Call address detection = Disabled
```
**각각 뜻은?**
### **Master Features**
- I2C Speed Mode = Standard Mode
I2C 통신 속도가 Standard Mode로 설정되었습니다. Standard Mode에서는 통신 속도가 100 kHz로 설정됩니다. I2C는 여러 가지 속도 모드를 지원하는데, Standard Mode는 가장 기본적인 속도 모드입니다.

- I2C Clock Speed (Hz) = 100000
I2C 버스의 클럭 속도가 100 kHz로 설정되어 있다는 뜻입니다. 즉, I2C 마스터가 통신할 때, 클럭 주파수는 100,000 Hz입니다.

### **Slave Features**
- Clock No Stretch Mode = Disabled
Clock Stretching은 I2C 슬레이브가 데이터를 준비하는 동안 클럭 신호를 멈추게 하는 기능입니다. 이 기능이 Disabled라는 것은, 슬레이브가 데이터를 준비하는 동안 클럭을 멈추지 않고 계속해서 마스터가 클럭을 제공해야 한다는 뜻입니다. 기본적으로는 I2C 슬레이브는 데이터를 준비하는 동안 클럭을 "늘려" 마스터가 기다릴 수 있도록 할 수 있습니다.

- Primary Address Length selection = 7-bit
I2C 주소가 7비트 주소로 설정된 것입니다. I2C 주소는 일반적으로 7비트 주소나 10비트 주소를 사용합니다. 7비트 주소는 I2C 장치가 최대 128개까지 사용할 수 있게 해줍니다. 보통 7비트 주소를 사용합니다.

- Dual Address Acknowledged = Disabled
이 설정은 I2C 슬레이브가 두 개의 주소를 동시에 인식하는 기능입니다. Disabled로 설정되어 있으면, 슬레이브 장치는 하나의 기본 주소만 사용하며 두 개의 주소를 동시에 사용할 수 없습니다.

- Primary slave address = 0x68
I2C 슬레이브의 기본 주소는 0x68입니다. 이 주소는 MPU9250 센서와 같은 I2C 장치에서 사용되는 주소입니다. MPU9250 센서는 일반적으로 이 주소를 사용하며, 이 주소를 통해 마스터가 슬레이브와 통신을 하게 됩니다.

- General Call address detection = Disabled
General Call 주소는 I2C 버스에서 모든 장치가 응답하는 특별한 주소인 0x00을 말합니다. Disabled로 설정된 것은 마스터가 General Call 주소를 사용해 모든 슬레이브 장치에 동시에 명령을 내릴 수 없다는 뜻입니다. 기본적으로는 모든 슬레이브가 0x00 주소에 응답하는 것을 막습니다.

--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

I2C Speed Mode: Standard Mode vs. Fast Mode

Standard Mode (100 kHz):
I2C 통신에서 기본적인 속도인 Standard Mode는 100 kHz로 설정됩니다. 이 속도는 대부분의 I2C 장치에서 안정적으로 동작하는 속도예요. 예를 들어, MPU9250과 같은 센서들은 보통 100 kHz에서 잘 작동하고, 통신 안정성도 높습니다. 주로 느리게, 안정적인 통신이 필요할 때 사용해요.

Fast Mode (400 kHz):
만약 통신 속도를 더 빠르게 하고 싶다면, Fast Mode를 선택할 수 있어요. 이때 I2C 속도가 400 kHz로 올라가게 됩니다. 예를 들어, 여러 개의 I2C 장치가 있고, 빠르게 데이터를 주고받아야 하는 상황에서 Fast Mode를 사용하면 더 효율적입니다. 하지만, 모든 장치가 400 kHz를 지원하는 건 아니기 때문에, 사용하려면 모든 장치가 그 속도를 지원하는지 확인해야 해요.
Fast Mode는 속도와 반응 시간을 더 중요하게 여길 때 유용하죠.

Clock No Stretch Mode: Enabled vs. Disabled

Disabled (기본 설정):
Clock Stretching은 슬레이브 장치가 데이터를 준비할 시간 동안 마스터가 클럭을 멈추게 하는 기능입니다. 만약 Clock No Stretch Mode가 Disabled라면, 슬레이브 장치는 마스터가 보내는 클럭을 멈추지 않아요. 그래서 데이터를 준비하는 동안 마스터가 계속해서 클럭을 제공해야 합니다. 예를 들어, 슬레이브가 계산을 하느라 시간이 오래 걸리는 경우, 마스터는 계속해서 클럭을 보낼 수 있어야 해요.
이 설정은 슬레이브가 데이터를 바로바로 처리할 수 있을 때, 즉 슬레이브가 빠르게 반응할 수 있을 때 사용합니다.

Enabled:
만약 Clock Stretching을 활성화하면, 슬레이브가 데이터를 준비하는 동안 마스터가 클럭을 멈추게 할 수 있어요. 예를 들어, 슬레이브 장치가 복잡한 계산을 할 때 데이터가 준비될 때까지 기다릴 수 있게 됩니다. 이럴 때 사용하면, 슬레이브가 데이터 준비를 마칠 때까지 마스터는 기다리고 클럭 신호를 멈추게 됩니다.

Primary Address Length: 7-bit vs. 10-bit

7-bit (보통 설정):
I2C 주소는 기본적으로 7비트 주소를 사용해요. 예를 들어, MPU9250 같은 센서들은 기본적으로 7비트 주소를 사용합니다. 7비트 주소는 최대 128개의 장치를 지원할 수 있어요. 대부분의 장치가 이 7비트 주소를 사용하므로 일반적으로 7비트 주소가 더 많이 쓰입니다.
만약 장치가 7비트 주소를 사용한다면, 그냥 기본 설정대로 7-bit을 선택하면 돼요.

10-bit:
만약 I2C 버스에 장치가 1024개까지 연결될 수 있도록 하려면 10비트 주소를 사용할 수 있어요. 하지만 7비트 주소만으로도 대부분의 경우 충분히 장치를 연결할 수 있기 때문에, 10비트 주소는 특별한 경우에만 필요합니다.

Dual Address Acknowledged: Enabled vs. Disabled

Disabled:
기본적으로 I2C 슬레이브는 하나의 Primary Address만을 사용합니다. 예를 들어, 0x68 주소를 사용하는 MPU9250 센서는 그 주소만을 사용해서 통신을 해요. Dual Address Acknowledged가 Disabled라면, 슬레이브는 단 하나의 주소만을 인식하고 응답합니다.

Enabled:
만약 Dual Address Acknowledged를 Enabled로 설정하면, 슬레이브 장치는 두 개의 주소를 동시에 사용할 수 있습니다. 예를 들어, 동일한 장치가 0x68과 0x69 두 가지 주소를 동시에 받아들이고 응답할 수 있게 되죠. 이 설정은 한 장치에 여러 주소를 할당해야 할 때 유용합니다. 하지만 보통은 하나의 주소로 충분히 통신할 수 있기 때문에 일반적으로 Disabled로 두는 경우가 많아요.

Primary Slave Address = 0x68

이건 해당 장치의 기본 I2C 주소입니다. 예를 들어, MPU9250 센서의 기본 주소가 0x68입니다. 이 설정을 통해 마스터가 이 장치와 통신할 때 0x68 주소를 사용하게 되죠. 이 주소는 보통 해당 장치의 데이터시트에 나와 있습니다.

General Call Address Detection: Enabled vs. Disabled

Disabled:
General Call 주소는 0x00 주소로, 마스터가 모든 슬레이브 장치에게 동시에 메시지를 보내는 기능입니다. 예를 들어, I2C 버스에 연결된 모든 장치에 동시에 데이터를 보내고 싶을 때 사용됩니다. 만약 General Call Address Detection이 Disabled로 설정되어 있으면, 마스터는 0x00 주소를 사용해 모든 장치에게 명령을 보낼 수 없습니다.

Enabled:
General Call Address Detection을 Enabled로 설정하면, 마스터가 0x00 주소로 보내는 신호를 모든 슬레이브가 받을 수 있게 됩니다. 이 설정은 모든 장치에 동시에 명령을 보내거나 초기화하려고 할 때 유용합니다.

정리하자면:

I2C Speed Mode (Standard Mode vs. Fast Mode):

Standard Mode (100 kHz) – 안정적이고 대부분의 장치에서 잘 작동.

Fast Mode (400 kHz) – 속도가 중요한 경우, 더 빠르게 통신 가능.

Clock No Stretch Mode (Disabled vs. Enabled):

Disabled – 슬레이브가 데이터를 빨리 처리할 수 있을 때 사용.

Enabled – 슬레이브가 데이터를 준비하는 동안 클럭을 멈추게 할 때 사용.

Primary Address Length (7-bit vs. 10-bit):

7-bit – 대부분의 장치에서 사용, 128개 장치까지 지원.

10-bit – 매우 많은 장치가 필요할 때 사용.

Dual Address Acknowledged (Disabled vs. Enabled):

Disabled – 하나의 주소만 사용.

Enabled – 두 개의 주소를 사용할 때.

Primary Slave Address:

장치의 기본 I2C 주소.

General Call Address Detection (Disabled vs. Enabled):

Disabled – 모든 장치에 동시에 명령을 보내지 않음.

Enabled – 마스터가 모든 장치에 동시에 명령을 보낼 수 있음.

------------------------------------------------------------------------------------------------------------------------------------------------------------

## 💻 **DMA 설정**
1. 똑같이 Pinout & Configuration 여기에 categories 클릭하고!
2. 이번에는 `System Core`를 누름
3. `DMA`를 누르면 `Mode`에는 아무것도 없고 그 밑에 `Configuration` 그밑에
4.  `DMA1`을 클릭 그리고 그 밑에 `Add`버튼 클릭하면 `Select`가 나옴
5. 거기서 `DMA Request`밑에 `I2C1_RX`를 선택 그리고 그 옆에 `Stream`(우선순위)은 `DMA1 Stream 0`
6. 그옆에 `Direction`은 Peripheral To Memory를 선택하고 그 옆에
7. `Priority`는 `Medium`을 선택
8. `I2C1_RX`를 누르면 그 밑에 `DMA Request Settings`가 나옴
9. 그 밑에 `Mode`에서 `Normal`이 아니라 `Circular`로 바꿈
10. 그 밑에 `Use Fifo`는 체크안함
11. 그 옆에 위에 `Increment Address`가 있는데 `Peripheral`과 `Memory`가 있음 `Memory`만 체크!
12. 그 밑에 `Data Width`가 있는데 그냥 둘다(Peripheral, Memory) `Byte`로 설정함 끝!

**각각 뜻!**
