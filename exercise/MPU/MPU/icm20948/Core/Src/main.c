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
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* ICM-20948 I2C 주소 (7bit << 1). AD0=LOW면 0x68, AD0=HIGH면 0x69 */
#define ICM20948_ADDR  (0x68 << 1)

/* ICM-20948 Bank 0 레지스터 (MPU6050/MPU9250과 다름!) */
#define REG_BANK_SEL   0x7F
#define WHO_AM_I       0x00
#define USER_CTRL      0x03
#define PWR_MGMT_1     0x06
#define ACCEL_XOUT_H   0x2D   /* 0x2D~0x32: 가속도 6바이트 */
#define GYRO_XOUT_H    0x33   /* 0x33~0x38: 자이로 6바이트 */
#define TEMP_OUT_H     0x39
#define EXT_SLV_SENS_DATA_00 0x3B  /* 0x3B~: I2C 슬레이브(자기장) 데이터 */

/* Bank 3: I2C Master (내부 자력계 AK09916 접근용) */
#define BANK3_I2C_MST_CTRL   0x01
#define BANK3_I2C_SLV0_ADDR  0x03
#define BANK3_I2C_SLV0_REG   0x04
#define BANK3_I2C_SLV0_CTRL  0x05
#define BANK3_REG_BANK_SEL   0x7F
#define USER_BANK_0    0x00
#define USER_BANK_3    0x30
#define ICM20948_WHO_AM_I_VAL 0xEA
#define AK09916_I2C_ADDR  0x0C   /* 내부 자력계 7bit (일부 보드는 0x0E) */
#define AK09916_WIA1      0x00   /* WhoAmI → 0x48 */
#define AK09916_HXL       0x11   /* 자력계 데이터 시작 레지스터 */
#define AK09916_CNTL2     0x31   /* 리셋: 0x01 */
#define AK09916_CNTL3     0x32   /* 모드: 0x02=단일, 0x06=연속2 */
#define AK09916_WIA1_VAL  0x48
#define BANK3_I2C_MST_DELAY_CTRL 0x02  /* Bank3 슬레이브 지연 */
#define BANK3_I2C_SLV0_DO 0x06   /* Bank3 슬레이브 쓰기 데이터 */

/* 필터: 순서 = 저역통과(LPF) 먼저 → 그 다음 이동평균(MA) */
#define MA_SIZE      16    /* 이동평균 윈도우 크기 */
#define LPF_ALPHA    0.65f /* 저역통과 Accel/Mag: 0~1, 작을수록 더 스무딩 */
#define LPF_ALPHA_GYRO 0.5f /* 자이로 전용 LPF (노이즈 많아서 더 강하게) */
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
uint8_t icm20948_i2c_addr = (0x68 << 1);
uint8_t ak09916_i2c_addr = AK09916_I2C_ADDR;
uint8_t mag_available = 0;

/* 이동평균: Accel/Gyro/Mag 각 축별 버퍼 및 합 */
static int16_t ma_accel[3][MA_SIZE], ma_gyro[3][MA_SIZE], ma_mag[3][MA_SIZE];
static uint8_t ma_idx = 0;
static int32_t ma_sum_accel[3], ma_sum_gyro[3], ma_sum_mag[3];
static uint8_t ma_filled = 0;
static uint8_t ma_filled_mag = 0;  /* Mag는 별도 채움 */

/* 저역통과 필터 상태 */
static float lpf_accel[3] = {0}, lpf_gyro[3] = {0}, lpf_mag[3] = {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
static void ICM20948_SelectBank(uint8_t bank);
static void AK09916_Init(void);
HAL_StatusTypeDef ICM20948_Init(void);
HAL_StatusTypeDef ICM20948_Read_AccelData(void);
HAL_StatusTypeDef ICM20948_Read_GyroData(void);
HAL_StatusTypeDef ICM20948_Read_MagData(void);
static void Filter_Update(int16_t ax, int16_t ay, int16_t az, int16_t gx, int16_t gy, int16_t gz, int16_t mx, int16_t my, int16_t mz);
static void Filter_GetOutput(float *ax, float *ay, float *az, float *gx, float *gy, float *gz, float *mx, float *my, float *mz);
void UART_Print_SensorData(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// 데이터 배열
uint8_t accel_data[6];  // 가속도계 데이터
uint8_t gyro_data[6];   // 자이로스코프 데이터
uint8_t mag_data[6];    // 자기장 데이터

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
  /* printf가 UART로 바로 나가도록 버퍼 끔 (안 하면 출력 안 보일 수 있음) */
  setvbuf(stdout, NULL, _IONBF, 0);

  printf("ICM20948 start\r\n");

  if (ICM20948_Init() != HAL_OK) {
      printf("ICM20948 init fail\r\n");
      while (1);
  }
  printf("ICM20948 init OK\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  int16_t ax, ay, az, gx, gy, gz, mx, my, mz;
	  if (ICM20948_Read_AccelData() == HAL_OK &&
	      ICM20948_Read_GyroData() == HAL_OK) {
	      if (mag_available)
	          ICM20948_Read_MagData();
	      ax = (int16_t)((accel_data[0] << 8) | accel_data[1]);
	      ay = (int16_t)((accel_data[2] << 8) | accel_data[3]);
	      az = (int16_t)((accel_data[4] << 8) | accel_data[5]);
	      gx = (int16_t)((gyro_data[0] << 8) | gyro_data[1]);
	      gy = (int16_t)((gyro_data[2] << 8) | gyro_data[3]);
	      gz = (int16_t)((gyro_data[4] << 8) | gyro_data[5]);
	      mx = mag_available ? (int16_t)((mag_data[1] << 8) | mag_data[0]) : 0;
	      my = mag_available ? (int16_t)((mag_data[3] << 8) | mag_data[2]) : 0;
	      mz = mag_available ? (int16_t)((mag_data[5] << 8) | mag_data[4]) : 0;
	      Filter_Update(ax, ay, az, gx, gy, gz, mx, my, mz);
	      UART_Print_SensorData();
	  } else {
	      printf("Data read error\r\n");
	  }
	  HAL_Delay(1000);
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
  hi2c1.Init.OwnAddress1 = 0;
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
/* Bank 선택 (0~3). icm20948_i2c_addr 사용 */
static void ICM20948_SelectBank(uint8_t bank) {
    uint8_t data[2] = { REG_BANK_SEL, bank };
    HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, data, 2, 1000);
}

/* ICM-20948 내부 I2C Master로 AK09916에 1바이트 쓰기 (ak09916_i2c_addr 사용) */
static void AK09916_WriteReg(uint8_t reg, uint8_t value) {
    uint8_t cfg[2];
    ICM20948_SelectBank(USER_BANK_3);
    cfg[0] = BANK3_I2C_SLV0_ADDR;
    cfg[1] = (ak09916_i2c_addr & 0x7F);  /* RNW=0 (Write) */
    HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, cfg, 2, 1000);
    cfg[0] = BANK3_I2C_SLV0_REG;
    cfg[1] = reg;
    HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, cfg, 2, 1000);
    cfg[0] = BANK3_I2C_SLV0_DO;
    cfg[1] = value;
    HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, cfg, 2, 1000);
    cfg[0] = BANK3_I2C_SLV0_CTRL;
    cfg[1] = 0x80;
    HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, cfg, 2, 1000);
    ICM20948_SelectBank(USER_BANK_0);
    HAL_Delay(2);
}

/* AK09916 WIA1(0x00) 읽기 → 0x48이면 정상. 주소 0x0C/0x0E 둘 다 시도 */
static uint8_t AK09916_ReadWIA1(uint8_t mag_addr) {
    uint8_t cfg[2];
    uint8_t buf[1];
    ICM20948_SelectBank(USER_BANK_3);
    cfg[0] = BANK3_I2C_SLV0_ADDR;
    cfg[1] = (mag_addr & 0x7F) | 0x80;  /* Read */
    HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, cfg, 2, 1000);
    cfg[0] = BANK3_I2C_SLV0_REG;
    cfg[1] = AK09916_WIA1;
    HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, cfg, 2, 1000);
    cfg[0] = BANK3_I2C_SLV0_CTRL;
    cfg[1] = 0x80 | 0x01;  /* Enable, 1 byte read */
    HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, cfg, 2, 1000);
    ICM20948_SelectBank(USER_BANK_0);
    HAL_Delay(5);
    cfg[0] = EXT_SLV_SENS_DATA_00;
    HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, cfg, 1, 1000);
    if (HAL_I2C_Master_Receive(&hi2c1, icm20948_i2c_addr, buf, 1, 1000) != HAL_OK)
        return 0x00;
    return buf[0];
}

/* AK09916 리셋 + 연속 모드. 주소 0x0C/0x0E 시도 후 WIA1=0x48인 주소 사용 */
static void AK09916_Init(void) {
    uint8_t mag_addrs[] = { 0x0C, 0x0E };
    uint8_t cfg[2];
    int j;

    /* I2C Master 슬레이브 지연 활성화 (Mag 응답 대기용) */
    ICM20948_SelectBank(USER_BANK_3);
    cfg[0] = BANK3_I2C_MST_DELAY_CTRL;
    cfg[1] = 0x01;  /* SLV0_DLY_EN */
    HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, cfg, 2, 1000);
    ICM20948_SelectBank(USER_BANK_0);
    HAL_Delay(1);

    for (j = 0; j < 2; j++) {
        ak09916_i2c_addr = mag_addrs[j];
        if (AK09916_ReadWIA1(ak09916_i2c_addr) == AK09916_WIA1_VAL)
            break;
    }
    if (j >= 2) {
        ak09916_i2c_addr = AK09916_I2C_ADDR;
        printf("Mag N/A (6-axis or no mag)\r\n");  /* 자력계 없음 → Mag 생략 */
        return;
    }

    mag_available = 1;  /* 자력계 있음 */

    AK09916_WriteReg(AK09916_CNTL2, 0x01);  /* 리셋 */
    HAL_Delay(10);
    AK09916_WriteReg(AK09916_CNTL3, 0x06);  /* 연속 측정 모드 2 (10Hz) */
    HAL_Delay(20);  /* 첫 샘플 준비 대기 */
}

/* ICM-20948 초기화: 0x68/0x69 둘 다 시도, 전원 대기, 실패 시 WHO_AM_I 출력 */
HAL_StatusTypeDef ICM20948_Init(void) {
    uint8_t data[2];
    HAL_StatusTypeDef status;
    uint8_t addr;
    int i;
    const uint8_t addrs[] = { (0x68 << 1), (0x69 << 1) };

    HAL_Delay(100);  /* 센서 전원 안정화 대기 */

    for (i = 0; i < 2; i++) {
        addr = addrs[i];
        icm20948_i2c_addr = addr;

        ICM20948_SelectBank(USER_BANK_0);
        data[0] = PWR_MGMT_1;
        data[1] = 0x00;
        status = HAL_I2C_Master_Transmit(&hi2c1, addr, data, 2, 1000);
        if (status != HAL_OK)
            continue;

        data[0] = USER_CTRL;
        data[1] = 0x20;
        status = HAL_I2C_Master_Transmit(&hi2c1, addr, data, 2, 1000);
        if (status != HAL_OK)
            continue;

        data[0] = WHO_AM_I;
        status = HAL_I2C_Master_Transmit(&hi2c1, addr, data, 1, 1000);
        if (status != HAL_OK)
            continue;
        status = HAL_I2C_Master_Receive(&hi2c1, addr, data, 1, 1000);
        if (status != HAL_OK)
            continue;

        if (data[0] == ICM20948_WHO_AM_I_VAL) {
            AK09916_Init();  /* 자력계 리셋 후 연속 측정 모드 */
            return HAL_OK;
        }
    }

    printf("WHO_AM_I=0x%02X (expect 0xEA)\r\n", data[0]);
    return HAL_ERROR;
}

/* 가속도 데이터 읽기 (블로킹 수신) */
HAL_StatusTypeDef ICM20948_Read_AccelData(void) {
    uint8_t reg = ACCEL_XOUT_H;
    ICM20948_SelectBank(USER_BANK_0);
    if (HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, &reg, 1, 1000) != HAL_OK)
        return HAL_ERROR;
    return HAL_I2C_Master_Receive(&hi2c1, icm20948_i2c_addr, accel_data, 6, 1000);
}

/* 자이로 데이터 읽기 (블로킹 수신) */
HAL_StatusTypeDef ICM20948_Read_GyroData(void) {
    uint8_t reg = GYRO_XOUT_H;
    ICM20948_SelectBank(USER_BANK_0);
    if (HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, &reg, 1, 1000) != HAL_OK)
        return HAL_ERROR;
    return HAL_I2C_Master_Receive(&hi2c1, icm20948_i2c_addr, gyro_data, 6, 1000);
}

/* 자력계(AK09916) 읽기: ak09916_i2c_addr 사용, 읽기 전 20ms 대기 */
HAL_StatusTypeDef ICM20948_Read_MagData(void) {
    uint8_t cfg[2];
    HAL_StatusTypeDef status;

    ICM20948_SelectBank(USER_BANK_3);
    cfg[0] = BANK3_I2C_SLV0_ADDR;
    cfg[1] = (ak09916_i2c_addr & 0x7F) | 0x80;  /* Read */
    status = HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, cfg, 2, 1000);
    if (status != HAL_OK) return status;

    cfg[0] = BANK3_I2C_SLV0_REG;
    cfg[1] = AK09916_HXL;
    status = HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, cfg, 2, 1000);
    if (status != HAL_OK) return status;

    cfg[0] = BANK3_I2C_SLV0_CTRL;
    cfg[1] = 0x80 | 0x06;  /* Enable, 6 bytes read */
    status = HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, cfg, 2, 1000);
    if (status != HAL_OK) return status;

    ICM20948_SelectBank(USER_BANK_0);
    HAL_Delay(20);  /* I2C Master가 Mag 읽기 완료 대기 (짧으면 0만 나옴) */

    cfg[0] = EXT_SLV_SENS_DATA_00;
    status = HAL_I2C_Master_Transmit(&hi2c1, icm20948_i2c_addr, cfg, 1, 1000);
    if (status != HAL_OK) return status;
    return HAL_I2C_Master_Receive(&hi2c1, icm20948_i2c_addr, mag_data, 6, 1000);
}

/* 1) 저역통과(LPF) 먼저 → 2) 그 결과를 이동평균(MA) 버퍼에 넣어서 출력 = MA 평균 */
static void Filter_Update(int16_t ax, int16_t ay, int16_t az, int16_t gx, int16_t gy, int16_t gz, int16_t mx, int16_t my, int16_t mz) {
    uint8_t i, n, nm;
    int16_t lpf_a, lpf_g, lpf_m;
    float alpha = LPF_ALPHA, alpha_g = LPF_ALPHA_GYRO;

    /* 1단계: RAW → 저역통과 (LPF) */
    for (i = 0; i < 3; i++) {
        float ra = (i == 0) ? (float)ax : (i == 1) ? (float)ay : (float)az;
        float rg = (i == 0) ? (float)gx : (i == 1) ? (float)gy : (float)gz;
        lpf_accel[i] = alpha * lpf_accel[i] + (1.0f - alpha) * ra;
        lpf_gyro[i]  = alpha_g * lpf_gyro[i]  + (1.0f - alpha_g) * rg;  /* 자이로 더 강하게 */
        if (mag_available) {
            float rm = (i == 0) ? (float)mx : (i == 1) ? (float)my : (float)mz;
            lpf_mag[i] = alpha * lpf_mag[i] + (1.0f - alpha) * rm;
        }
    }

    /* 2단계: LPF 출력을 이동평균 버퍼에 넣기 (출력 = MA 평균) */
    for (i = 0; i < 3; i++) {
        lpf_a = (int16_t)(lpf_accel[i] + 0.5f);
        lpf_g = (int16_t)(lpf_gyro[i] + 0.5f);
        if (ma_filled < MA_SIZE) {
            ma_accel[i][ma_idx] = lpf_a;
            ma_gyro[i][ma_idx]  = lpf_g;
            ma_sum_accel[i] += lpf_a;
            ma_sum_gyro[i]  += lpf_g;
        } else {
            ma_sum_accel[i] -= ma_accel[i][ma_idx];
            ma_sum_gyro[i]  -= ma_gyro[i][ma_idx];
            ma_accel[i][ma_idx] = lpf_a;
            ma_gyro[i][ma_idx]  = lpf_g;
            ma_sum_accel[i] += lpf_a;
            ma_sum_gyro[i]  += lpf_g;
        }
        if (mag_available) {
            lpf_m = (int16_t)(lpf_mag[i] + 0.5f);
            if (ma_filled_mag < MA_SIZE) {
                ma_mag[i][ma_idx] = lpf_m;
                ma_sum_mag[i] += lpf_m;
            } else {
                ma_sum_mag[i] -= ma_mag[i][ma_idx];
                ma_mag[i][ma_idx] = lpf_m;
                ma_sum_mag[i] += lpf_m;
            }
        }
    }
    ma_idx = (ma_idx + 1) % MA_SIZE;
    if (ma_filled < MA_SIZE)
        ma_filled++;
    if (mag_available && ma_filled_mag < MA_SIZE)
        ma_filled_mag++;
}

/* 출력 = 이동평균(MA) 평균값 (LPF 적용된 값들의 평균) */
static void Filter_GetOutput(float *ax, float *ay, float *az, float *gx, float *gy, float *gz, float *mx, float *my, float *mz) {
    uint8_t n  = (ma_filled < MA_SIZE) ? ma_filled : (uint8_t)MA_SIZE;
    uint8_t nm = (ma_filled_mag < MA_SIZE) ? ma_filled_mag : (uint8_t)MA_SIZE;
    if (n == 0) n = 1;
    if (nm == 0) nm = 1;
    if (ax) *ax = (float)ma_sum_accel[0] / (float)n;
    if (ay) *ay = (float)ma_sum_accel[1] / (float)n;
    if (az) *az = (float)ma_sum_accel[2] / (float)n;
    if (gx) *gx = (float)ma_sum_gyro[0]  / (float)n;
    if (gy) *gy = (float)ma_sum_gyro[1]  / (float)n;
    if (gz) *gz = (float)ma_sum_gyro[2]  / (float)n;
    if (mx) *mx = (float)ma_sum_mag[0]   / (float)nm;
    if (my) *my = (float)ma_sum_mag[1]   / (float)nm;
    if (mz) *mz = (float)ma_sum_mag[2]   / (float)nm;
}

PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}

/* 필터 적용된 센서 데이터 출력 (Accel/Gyro/Mag 모두 이동평균+저역통과) */
void UART_Print_SensorData(void) {
    float ax, ay, az, gx, gy, gz, mx, my, mz;
    Filter_GetOutput(&ax, &ay, &az, &gx, &gy, &gz, &mx, &my, &mz);
    printf("--------------------------------------------------\r\n");
    printf("Accel X: %.0f, Y: %.0f, Z: %.0f\r\n", ax, ay, az);
    printf("Gyro  X: %.0f, Y: %.0f, Z: %.0f\r\n", gx, gy, gz);
    if (mag_available) {
        printf("Mag   X: %.0f, Y: %.0f, Z: %.0f\r\n", mx, my, mz);
        printf("--------------------------------------------------\r\n");
    }
    else
        printf("Mag   N/A (6-axis or no mag)\r\n");
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
