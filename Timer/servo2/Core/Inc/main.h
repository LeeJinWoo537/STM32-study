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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
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
// LED 관련 정의
#define LED1_PIN GPIO_PIN_8
#define LED1_GPIO_Port GPIOB
#define LED2_PIN GPIO_PIN_9
#define LED2_GPIO_Port GPIOB

// LED 제어 버튼 정의 (2개만 사용: LED1용, LED2용)
#define LED_BTN1_PIN GPIO_PIN_2
#define LED_BTN1_GPIO_Port GPIOC
#define LED_BTN2_PIN GPIO_PIN_3
#define LED_BTN2_GPIO_Port GPIOC

// 가변저항 (서보모터 제어용) ADC 핀 정의
#define POTENTIOMETER_PIN GPIO_PIN_1
#define POTENTIOMETER_GPIO_Port GPIOA
#define ADC_CHANNEL_1 1

// 28BYJ-48 스테퍼 모터 핀 정의 (ULN2003 드라이버)
#define STEPPER_IN1_PIN GPIO_PIN_4
#define STEPPER_IN1_GPIO_Port GPIOA
#define STEPPER_IN2_PIN GPIO_PIN_6
#define STEPPER_IN2_GPIO_Port GPIOA
#define STEPPER_IN3_PIN GPIO_PIN_7
#define STEPPER_IN3_GPIO_Port GPIOA
#define STEPPER_IN4_PIN GPIO_PIN_8
#define STEPPER_IN4_GPIO_Port GPIOA

// 28BYJ-48 스테퍼 모터 설정
#define STEPS_PER_REVOLUTION 2048  // 28BYJ-48 하프 스텝 모드
#define MAX_ANGLE 100              // 최대 각도 (100 = 한바퀴)
#define STEP_DELAY_MS 2            // 스텝 간 지연시간 (밀리초)

// 서보모터 제한값 설정
#define SERVO_MIN_PULSE 1000       // 최소 펄스 (닫힘)
#define SERVO_MAX_PULSE 2350       // 최대 펄스 (열림)
#define SERVO_PULSE_RANGE (SERVO_MAX_PULSE - SERVO_MIN_PULSE)  // 펄스 범위

// 메뉴 상태 정의
typedef enum {
    MENU_MAIN,
    MENU_SERVO,
    MENU_SERVO_INPUT,
    MENU_LED,
    MENU_LED_INPUT,
    MENU_STEPPER,
    MENU_STEPPER_ANGLE_INPUT
} MenuState_t;

// 스텝모터 상태 정의
typedef struct {
    int32_t current_position;  // 현재 위치 (0-100)
    uint8_t is_enabled;        // 활성화 상태
} StepperState_t;

// 서보모터 상태 정의
typedef struct {
    uint16_t current_pulse;     // 현재 펄스 값 (1000-2350)
    uint8_t is_open;           // 열림 상태 (1=열림, 0=닫힘)
} ServoState_t;

// LED 상태 정의
typedef struct {
    uint8_t led1_state;  // LED1 상태 (1=켜짐, 0=꺼짐)
    uint8_t led2_state;  // LED2 상태 (1=켜짐, 0=꺼짐)
} LedState_t;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
