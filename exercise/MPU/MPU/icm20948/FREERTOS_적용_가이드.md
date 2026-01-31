# FreeRTOS 적용 가이드 (icm20948)

## 1. 현재 상태
- **RTOS 미사용** (bare metal, `HAL_Delay` 사용)
- FreeRTOS 적용 시: 센서 읽기는 **태스크**에서, `HAL_Delay(1000)` → **osDelay(1000)** 사용

---

## 2. STM32CubeIDE에서 FreeRTOS 켜기

1. **프로젝트에서 `icm20948.ioc` 더블클릭** → CubeMX 설정 화면 열림  
2. 왼쪽 **Pinout & Configuration** 탭에서  
   - **Middleware** (또는 **Software Packs** → **Select Components**)  
   - **FREERTOS** 체크  
3. **FREERTOS** 선택 후:
   - **Interface**: `CMSIS_V2` (권장) 또는 `CMSIS_V1`
   - **Tasks and Queues**: 기본 태스크 1개 있음 (이름 예: `defaultTask`)
   - **Config parameters**: `configTOTAL_HEAP_SIZE` 필요 시 조정 (예: 4096)
4. **File → Save** (또는 Ctrl+S)  
5. **Project → Generate Code** (또는 톱니바퀴 아이콘)  
   - 기존 `main.c`가 재생성됩니다. **USER CODE** 블록 밖의 수정은 사라지므로, 아래 3단계에서 다시 넣어야 합니다.

---

## 3. 코드 생성 후 넣을 코드

### 3-1. main.c — USER CODE BEGIN 2

`MX_FREERTOS_Init()` 호출 **위쪽**에 아래를 넣습니다 (기존 while(1) 센서 루프는 제거됨).

```c
  /* USER CODE BEGIN 2 */
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("ICM20948 start\r\n");

  if (ICM20948_Init() != HAL_OK) {
      printf("ICM20948 init fail\r\n");
      while (1);
  }
  printf("ICM20948 init OK\r\n");
  /* USER CODE END 2 */
```

- **주의**: 재생성된 main에는 `osKernelStart()`가 있어서, **while(1) 센서 루프는 main에 두지 말고** 아래 `defaultTask` 안에 넣습니다.

---

### 3-2. freertos.c — defaultTask (센서 1초 주기)

코드 생성 후 `Core/Src/freertos.c`에 `StartDefaultTask`(또는 `defaultTask`) 함수가 생깁니다.  
그 **안의 for(;;)** 루프를 아래처럼 바꿉니다.

```c
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
      if (ICM20948_Read_AccelData() == HAL_OK &&
          ICM20948_Read_GyroData() == HAL_OK) {
          if (mag_available)
              ICM20948_Read_MagData();
          UART_Print_SensorData();
      } else {
          printf("Data read error\r\n");
      }
      osDelay(1000);  /* 1초 (HAL_Delay 대신) */
  }
  /* USER CODE END StartDefaultTask */
}
```

- `freertos.c` 상단에 `#include "main.h"` 가 있으면, `main.h`에 선언된 ICM20948 함수·변수를 그대로 쓸 수 있습니다.  
- `main.h`에 없으면 `main.c`에 있는 ICM20948 선언을 `main.h`로 옮기거나, `freertos.c`에서 `extern`으로 선언해 사용하세요.

---

## 4. HAL Timebase (SysTick)

FreeRTOS 사용 시 SysTick은 커널이 씁니다.  
CubeMX가 **HAL Timebase Source**를 **SysTick** → **Timer** 등으로 바꾸면, 별도 타이머가 HAL 딜레이용으로 쓰입니다.  
재생성 후 **System Core → SYS → Timebase Source** 확인해 두면 좋습니다.

---

## 5. 요약

| 항목        | 기존 (bare metal)   | FreeRTOS 적용 후        |
|-------------|---------------------|--------------------------|
| 1초 대기    | `HAL_Delay(1000)`   | `osDelay(1000)` (태스크) |
| 센서 루프   | main의 while(1)     | defaultTask의 for(;;)    |
| 초기화      | main USER CODE 2    | 그대로 main USER CODE 2  |

위 순서대로 하면 ICM20948 동작을 유지한 채 FreeRTOS만 도입할 수 있습니다.
