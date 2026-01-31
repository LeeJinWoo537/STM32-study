/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "stm32f4xx_hal.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */



// MPU9250 I2C 주소
#define MPU9250_ADDR    0xD0  // 7-bit 주소 0x68, Write = 0xD0, Read = 0xD1

// 가속도, 자이로 데이터 레지스터 주소
#define ACCEL_XOUT_H    0x3B
#define ACCEL_YOUT_H    0x3D
#define ACCEL_ZOUT_H    0x3F
#define GYRO_XOUT_H     0x43
#define GYRO_YOUT_H     0x45
#define GYRO_ZOUT_H     0x47

// 필터 설정
#define MA_WINDOW_SIZE  8   // 이동평균 창 크기 (클수록 더 스무스, 반응 느림)
#define LPF_ALPHA       0.85f   // 저역통과 필터 계수 0~1 (높을수록 더 스무스)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_rx;
DMA_HandleTypeDef hdma_i2c1_tx;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
// 데이터 배열
uint8_t rxData[14];  // 가속도계(6) + 자이로(6) = 12바이트 (14로 여유 있게)

// DMA 전송 완료 플래그
volatile uint8_t dmaTransferComplete = 0;

// 이동평균 필터용 버퍼/상태
static int16_t accel_ma_buf[3][MA_WINDOW_SIZE], gyro_ma_buf[3][MA_WINDOW_SIZE];
static int32_t accel_ma_sum[3] = {0}, gyro_ma_sum[3] = {0};
static uint8_t ma_index = 0;
static uint8_t ma_count = 0;  // 버퍼가 채워진 샘플 수 (MA_WINDOW_SIZE까지 증가 후 유지)

// 저역통과 필터 이전 출력값
static float accel_lpf[3] = {0}, gyro_lpf[3] = {0};
static uint8_t lpf_initialized = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
// 함수 프로토타입
void I2C_Read_DMA(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint16_t len);
void MPU9250_Init(void);
void MPU9250_Read_Accel_Gyro_DMA(void);

HAL_StatusTypeDef I2C_Read(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint16_t len)
{
    return HAL_I2C_Mem_Read(&hi2c1, devAddr, regAddr, 1, data, len, HAL_MAX_DELAY);
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  MPU9250_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // DMA로 가속도계와 자이로 데이터 읽기
	  MPU9250_Read_Accel_Gyro_DMA();

	  HAL_Delay(500);  // 500ms 딜레이
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 208;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// MPU9250 초기화 함수
void MPU9250_Init(void)
{
    uint8_t check;
    uint8_t data;

    // MPU9250이 제대로 연결되어 있는지 확인
    I2C_Read(MPU9250_ADDR, 0x75, &check, 1); // WHO_AM_I 레지스터 읽기
    if (check == 0x71) {
        // 정상적으로 연결되어 있다면 초기화 시작
        // PWR_MGMT_1 레지스터 설정 (센서 켜기)
        data = 0x00; // No sleep mode
        HAL_I2C_Mem_Write(&hi2c1, MPU9250_ADDR, 0x6B, 1, &data, 1, HAL_MAX_DELAY);
    }

}

// 가속도계와 자이로 데이터 읽기 (이동평균 + 저역통과 필터 적용)
void MPU9250_Read_Accel_Gyro_DMA(void)
{
    // I2C를 통해 DMA로 데이터를 읽습니다.
    I2C_Read_DMA(MPU9250_ADDR, ACCEL_XOUT_H, rxData, 14);  // 가속도계와 자이로 데이터(12바이트) 읽기

    // DMA 전송 완료 대기 (타임아웃 100ms - 실패 시 무한 루프 방지)
    uint32_t tick = HAL_GetTick();
    while (!dmaTransferComplete && (HAL_GetTick() - tick < 100));
    if (!dmaTransferComplete) {
        printf("MPU9250 DMA timeout\r\n");
        return;
    }

    // 가속도계와 자이로 raw 데이터를 16비트 값으로 변환
    int16_t accel_raw[3], gyro_raw[3];
    accel_raw[0] = (int16_t)((rxData[0] << 8) | rxData[1]);
    accel_raw[1] = (int16_t)((rxData[2] << 8) | rxData[3]);
    accel_raw[2] = (int16_t)((rxData[4] << 8) | rxData[5]);
    gyro_raw[0] = (int16_t)((rxData[6] << 8) | rxData[7]);
    gyro_raw[1] = (int16_t)((rxData[8] << 8) | rxData[9]);
    gyro_raw[2] = (int16_t)((rxData[10] << 8) | rxData[11]);

    // ---- 1) 이동평균 필터 ----
    int16_t accel_ma[3], gyro_ma[3];
    for (int i = 0; i < 3; i++) {
        if (ma_count < MA_WINDOW_SIZE) {
            accel_ma_buf[i][ma_count] = accel_raw[i];
            accel_ma_sum[i] += accel_raw[i];
            accel_ma[i] = (int16_t)(accel_ma_sum[i] / (ma_count + 1));
        } else {
            accel_ma_sum[i] -= accel_ma_buf[i][ma_index];
            accel_ma_buf[i][ma_index] = accel_raw[i];
            accel_ma_sum[i] += accel_raw[i];
            accel_ma[i] = (int16_t)(accel_ma_sum[i] / MA_WINDOW_SIZE);
        }
    }
    for (int i = 0; i < 3; i++) {
        if (ma_count < MA_WINDOW_SIZE) {
            gyro_ma_buf[i][ma_count] = gyro_raw[i];
            gyro_ma_sum[i] += gyro_raw[i];
            gyro_ma[i] = (int16_t)(gyro_ma_sum[i] / (ma_count + 1));
        } else {
            gyro_ma_sum[i] -= gyro_ma_buf[i][ma_index];
            gyro_ma_buf[i][ma_index] = gyro_raw[i];
            gyro_ma_sum[i] += gyro_raw[i];
            gyro_ma[i] = (int16_t)(gyro_ma_sum[i] / MA_WINDOW_SIZE);
        }
    }
    if (ma_count < MA_WINDOW_SIZE)
        ma_count++;
    else
        ma_index = (ma_index + 1) % MA_WINDOW_SIZE;

    // ---- 2) 저역통과 필터 (1차 IIR: y = alpha*y_prev + (1-alpha)*x) ----
    for (int i = 0; i < 3; i++) {
        if (!lpf_initialized) {
            accel_lpf[i] = (float)accel_ma[i];
            gyro_lpf[i] = (float)gyro_ma[i];
        } else {
            accel_lpf[i] = LPF_ALPHA * accel_lpf[i] + (1.0f - LPF_ALPHA) * (float)accel_ma[i];
            gyro_lpf[i] = LPF_ALPHA * gyro_lpf[i] + (1.0f - LPF_ALPHA) * (float)gyro_ma[i];
        }
    }
    lpf_initialized = 1;

    // 필터 적용된 값 출력
    printf("-------------------------------------------------------------------------------------\r\n");
    printf("Accel X: %d, Y: %d, Z: %d\r\n", (int)accel_lpf[0], (int)accel_lpf[1], (int)accel_lpf[2]);
    printf("Gyro X: %d, Y: %d, Z: %d\r\n", (int)gyro_lpf[0], (int)gyro_lpf[1], (int)gyro_lpf[2]);
    printf("-------------------------------------------------------------------------------------\r\n");

    // DMA 전송 완료 플래그 리셋
    dmaTransferComplete = 0;
}

// I2C Mem Read DMA 완료 콜백 - HAL이 이 콜백을 호출함 (HAL_DMA_XferCpltCallback은 I2C 드라이버가 덮어써서 호출되지 않음)
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        dmaTransferComplete = 1;
    }
}

// I2C로 데이터 읽기 DMA 함수 (Mem_Read_DMA: 레지스터 주소 전송 + Repeated Start + 데이터 수신을 한 번에 처리)
void I2C_Read_DMA(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint16_t len)
{
    dmaTransferComplete = 0;  // 새 전송 전 플래그 리셋
    HAL_I2C_Mem_Read_DMA(&hi2c1, devAddr, regAddr, I2C_MEMADD_SIZE_8BIT, data, len);
}

PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART6 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
