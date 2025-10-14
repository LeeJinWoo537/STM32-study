/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define LD2_Pin GPIO_PIN_5
#define LD2_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
// 센서 핀 정의 (IOC 파일과 일치)
#define WATER_LEVEL_SENSOR_Pin GPIO_PIN_0
#define WATER_LEVEL_SENSOR_GPIO_Port GPIOA
#define MQ2_GAS_SENSOR_Pin GPIO_PIN_1
#define MQ2_GAS_SENSOR_GPIO_Port GPIOA
#define DUST_SENSOR_ANALOG_Pin GPIO_PIN_4
#define DUST_SENSOR_ANALOG_GPIO_Port GPIOA
#define DHT11_TEMP_HUMIDITY_Pin GPIO_PIN_6
#define DHT11_TEMP_HUMIDITY_GPIO_Port GPIOA
#define DUST_SENSOR_LED_Pin GPIO_PIN_7
#define DUST_SENSOR_LED_GPIO_Port GPIOA

// 기존 핀 정의 (호환성을 위해 유지)
#define WATER_LEVEL_Pin GPIO_PIN_0
#define WATER_LEVEL_GPIO_Port GPIOA
#define MQ2_Pin GPIO_PIN_1
#define MQ2_GPIO_Port GPIOA
#define DUST_Pin GPIO_PIN_4
#define DUST_GPIO_Port GPIOA
#define DHT11_Pin GPIO_PIN_6
#define DHT11_GPIO_Port GPIOA
#define DUST_LED_Pin GPIO_PIN_7
#define DUST_LED_GPIO_Port GPIOA

// 센서 관련 상수
#define ADC_RESOLUTION 4095.0f  // 12-bit ADC
#define VREF 3.3f               // 기준 전압

// DHT11 관련 상수
#define DHT11_TIMEOUT 10000
#define DHT11_RETRY_COUNT 3

// 타이머 관련 상수
#define TIMER_PERIOD 3000  // 3초 (밀리초)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
