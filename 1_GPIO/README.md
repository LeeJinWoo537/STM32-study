# GPIO 공부

2025.09.25~2025.09.26

## GPIO란
GPIO는 "General Purpose Input/Output"의 약자로, 마이크로컨트롤러나 컴퓨터 보드에서 외부 장치와 데이터를 주고받기 위해 다양한 용도로 사용하는 입출력 핀을 의미합니다.
디지털로 이야기를 하면 3.3V || 5V를 줄 때 HIGH가 되고 0이 될 때 LOW가 되는 핀을 사용할 수 있는 입출력 핀
## 핵심 코드 설명

### HAL_GPIO_WritePin(GPIO 포트, GPIO_PIN, SET 또는 RESET);  // SET == HIGH(1), RESET == LOW(0)
이거는 OUTPUT을 출력을 할 수 있는 HAL 함수

### HAL_GPIO_ReadPin(GPIO 포트, GPIO_PIN);
이거는 INPUT값을 읽을 수 있는 HAL 함수

### HAL_GPIO_TogglePin(GPIO 포트, GPIO_PIN);
not이라고 생각하면 쉬움 0이면 1로 변하고 1이면 0으로 변하는 HAL 함수