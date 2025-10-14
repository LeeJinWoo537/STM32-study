/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include <string.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
// 센서 데이터 저장 변수
float water_level_voltage = 0.0f;
float mq2_voltage = 0.0f;
float dust_voltage = 0.0f;
float dust_density = 0.0f;
float temperature = 0.0f;
float humidity = 0.0f;

// 타이머 플래그
volatile uint8_t timer_flag = 0;

// ADC 버퍼
uint32_t adc_buffer[3] = {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
int __io_putchar(int ch)
{
	HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, 0xFFFF);
	return ch;
}

// 센서 관련 함수들
void Read_ADC_Sensors(void);
void Read_Dust_Sensor(void);
void delay_us(uint32_t us);
uint8_t Read_DHT11(float *temperature, float *humidity);
void Process_Sensor_Data(void);
void Print_Sensor_Data(void);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
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
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  
  // 타이머 시작
  HAL_TIM_Base_Start_IT(&htim2);
  
  printf("STM32 센서 모니터링 시스템 시작\r\n");
  printf("센서: 물수위, MQ2 가스, 미세먼지, DHT11 온습도\r\n");
  printf("측정 주기: 3초\r\n\r\n");
  
  // 디버깅: 타이머 상태 확인
  printf("타이머 상태 확인:\r\n");
  printf("TIM2 CR1: 0x%08X\r\n", TIM2->CR1);
  printf("TIM2 DIER: 0x%08X\r\n", TIM2->DIER);
  printf("TIM2 PSC: %lu\r\n", TIM2->PSC);
  printf("TIM2 ARR: %lu\r\n", TIM2->ARR);
  printf("TIM2 CNT: %lu\r\n", TIM2->CNT);
  printf("NVIC ISER: 0x%08X\r\n", NVIC->ISER[0]);
  printf("==============================\r\n\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // 타이머 인터럽트가 발생하면 센서 데이터 읽기
    if (timer_flag) {
      timer_flag = 0;
      
      printf("타이머 인터럽트 발생! 센서 데이터 읽기 시작...\r\n");
      
      // 미세먼지 센서 LED 제어 (ADC 읽기 전에 실행)
      Read_Dust_Sensor();
      
      // ADC 센서들 읽기
      Read_ADC_Sensors();
      
      // DHT11 온습도 센서 읽기
      Read_DHT11(&temperature, &humidity);
      
      // 센서 데이터 처리
      Process_Sensor_Data();
      
      // 센서 데이터 출력
      Print_Sensor_Data();
    }
    
    // 1초마다 상태 확인 (디버깅용)
    static uint32_t last_check = 0;
    if (HAL_GetTick() - last_check > 1000) {
      last_check = HAL_GetTick();
      printf("시스템 동작 중... 타이머 플래그: %d, TIM2 CNT: %lu\r\n", timer_flag, TIM2->CNT);
    }
    
    HAL_Delay(10);  // CPU 부하 감소
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
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 3;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;  // 물수위 센서 (PA0)
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;  // MQ2 가스 센서 (PA1)
  sConfig.Rank = 2;
  sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_4;  // 미세먼지 센서 (PA4)
  sConfig.Rank = 3;
  sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8399;  // 84MHz / 8400 = 10kHz
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 30000;    // 10kHz / 30000 = 3초
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
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
  __HAL_RCC_ADC1_CLK_ENABLE();
  __HAL_RCC_TIM2_CLK_ENABLE();

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
  
  // ADC 센서 핀들 설정 (PA0, PA1, PA4)
  GPIO_InitStruct.Pin = WATER_LEVEL_SENSOR_Pin | MQ2_GAS_SENSOR_Pin | DUST_SENSOR_ANALOG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  // DHT11 핀 설정 (PA6) - 디지털 입출력
  GPIO_InitStruct.Pin = DHT11_TEMP_HUMIDITY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_TEMP_HUMIDITY_GPIO_Port, &GPIO_InitStruct);
  
  // 미세먼지 센서 LED 핀 설정 (PA7)
  GPIO_InitStruct.Pin = DUST_SENSOR_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DUST_SENSOR_LED_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
  * @brief ADC 센서들 읽기
  * @retval None
  */
void Read_ADC_Sensors(void)
{
  printf("ADC 센서 읽기 시작...\r\n");
  
  // ADC 변환 시작
  if (HAL_ADC_Start(&hadc1) != HAL_OK) {
    printf("ADC 시작 실패!\r\n");
    return;
  }
  
  // 모든 채널을 한 번에 읽기 (스캔 모드)
  if (HAL_ADC_PollForConversion(&hadc1, 1000) == HAL_OK) {
    adc_buffer[0] = HAL_ADC_GetValue(&hadc1);  // 물수위 센서 (Rank 1)
    printf("물수위 ADC: %lu\r\n", adc_buffer[0]);
  } else {
    printf("물수위 센서 읽기 실패!\r\n");
  }
  
  if (HAL_ADC_PollForConversion(&hadc1, 1000) == HAL_OK) {
    adc_buffer[1] = HAL_ADC_GetValue(&hadc1);  // MQ2 가스 센서 (Rank 2)
    printf("MQ2 ADC: %lu\r\n", adc_buffer[1]);
  } else {
    printf("MQ2 센서 읽기 실패!\r\n");
  }
  
  if (HAL_ADC_PollForConversion(&hadc1, 1000) == HAL_OK) {
    adc_buffer[2] = HAL_ADC_GetValue(&hadc1);  // 미세먼지 센서 (Rank 3)
    printf("미세먼지 ADC: %lu\r\n", adc_buffer[2]);
  } else {
    printf("미세먼지 센서 읽기 실패!\r\n");
    adc_buffer[2] = 0;  // 실패 시 0으로 설정
  }
  
  HAL_ADC_Stop(&hadc1);
  printf("ADC 센서 읽기 완료!\r\n");
}

/**
  * @brief 미세먼지 센서 읽기 (LED 제어)
  * @retval None
  */
void Read_Dust_Sensor(void)
{
  printf("미세먼지 센서 LED 제어 시작...\r\n");
  
  // LED 켜기
  HAL_GPIO_WritePin(DUST_SENSOR_LED_GPIO_Port, DUST_SENSOR_LED_Pin, GPIO_PIN_RESET);
  
  // 280us 대기 (정밀한 타이밍을 위해 루프 사용)
  for (volatile uint32_t i = 0; i < 280; i++) {
    __NOP();  // 약 1us 지연
  }
  
  // LED 끄기
  HAL_GPIO_WritePin(DUST_SENSOR_LED_GPIO_Port, DUST_SENSOR_LED_Pin, GPIO_PIN_SET);
  
  // 40us 대기
  for (volatile uint32_t i = 0; i < 40; i++) {
    __NOP();  // 약 1us 지연
  }
  
  printf("미세먼지 센서 LED 제어 완료!\r\n");
}

/**
  * @brief 마이크로초 단위 지연 함수
  * @param us: 지연할 마이크로초
  * @retval None
  */
void delay_us(uint32_t us)
{
  for (volatile uint32_t i = 0; i < us; i++) {
    __NOP();  // 약 1us 지연
  }
}

/**
  * @brief DHT11 온습도 센서 읽기 (간소화된 버전)
  * @param temperature: 온도 값을 저장할 포인터
  * @param humidity: 습도 값을 저장할 포인터
  * @retval 1: 성공, 0: 실패
  */
uint8_t Read_DHT11(float *temperature, float *humidity)
{
  printf("DHT11 센서 읽기 시작...\r\n");
  
  // 간단한 더미 데이터로 테스트
  *humidity = 60.0f;
  *temperature = 23.5f;
  
  // 정수로 변환하여 출력
  uint32_t temp_int = (uint32_t)(*temperature * 10);
  uint32_t hum_int = (uint32_t)(*humidity * 10);
  
  printf("DHT11 센서 읽기 완료! 온도: %lu.%lu°C, 습도: %lu.%lu%%\r\n", 
         temp_int / 10, temp_int % 10, hum_int / 10, hum_int % 10);
  return 1;
}

/**
  * @brief 센서 데이터 처리
  * @retval None
  */
void Process_Sensor_Data(void)
{
  printf("센서 데이터 처리 시작...\r\n");
  
  // ADC 값을 전압으로 변환
  water_level_voltage = (adc_buffer[0] * VREF) / ADC_RESOLUTION;
  mq2_voltage = (adc_buffer[1] * VREF) / ADC_RESOLUTION;
  dust_voltage = (adc_buffer[2] * VREF) / ADC_RESOLUTION;
  
  // 미세먼지 농도 계산 (GP2Y1010AU0F)
  // 전압을 먼저 디지털 값으로 변환 (0-1023)
  float dust_digital = (dust_voltage * 1023.0f) / VREF;
  
  // GP2Y1010AU0F 공식 적용
  if (dust_digital > 0.6f) {
    dust_density = (dust_digital * 0.17f) - 0.1f;
  } else {
    dust_density = 0.0f;
  }
  
  // 음수 값 방지
  if (dust_density < 0) dust_density = 0;
  
  printf("센서 데이터 처리 완료!\r\n");
}

/**
  * @brief 센서 데이터 출력
  * @retval None
  */
void Print_Sensor_Data(void)
{
  printf("=== 센서 데이터 (3초 주기) ===\r\n");
  
  // 전압값을 정수로 변환하여 출력 (소수점 3자리)
  uint32_t water_voltage_int = (uint32_t)(water_level_voltage * 1000);
  uint32_t mq2_voltage_int = (uint32_t)(mq2_voltage * 1000);
  uint32_t dust_voltage_int = (uint32_t)(dust_voltage * 1000);
  uint32_t dust_density_int = (uint32_t)(dust_density * 100);
  uint32_t temp_int = (uint32_t)(temperature * 10);
  uint32_t hum_int = (uint32_t)(humidity * 10);
  
  printf("물수위 센서: %lu.%03luV (ADC: %lu)\r\n", 
         water_voltage_int / 1000, water_voltage_int % 1000, adc_buffer[0]);
  printf("MQ2 가스센서: %lu.%03luV (ADC: %lu)\r\n", 
         mq2_voltage_int / 1000, mq2_voltage_int % 1000, adc_buffer[1]);
  printf("미세먼지 센서: %lu.%03luV (ADC: %lu)\r\n", 
         dust_voltage_int / 1000, dust_voltage_int % 1000, adc_buffer[2]);
  printf("미세먼지 농도: %lu.%02lu mg/m³\r\n", 
         dust_density_int / 100, dust_density_int % 100);
  printf("온도: %lu.%lu°C\r\n", 
         temp_int / 10, temp_int % 10);
  printf("습도: %lu.%lu%%\r\n", 
         hum_int / 10, hum_int % 10);
  printf("==============================\r\n\r\n");
}

/**
  * @brief 타이머 인터럽트 콜백 함수
  * @param htim: 타이머 핸들
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2) {
    printf("TIM2 인터럽트 발생!\r\n");
    timer_flag = 1;
  }
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
#ifdef USE_FULL_ASSERT
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
