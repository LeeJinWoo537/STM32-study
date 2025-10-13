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
#include <stdlib.h>
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
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
// 전역 변수
MenuState_t current_menu = MENU_MAIN;
StepperState_t stepper_state = {0, 0};
ServoState_t servo_state = {1000, 0};  // 초기값: 닫힘 상태
LedState_t led_state = {0, 0};         // 초기값: 모두 꺼짐
char input_buffer[10];
uint8_t input_index = 0;
uint8_t uart_rx_buffer[1];

// 버튼 플래그 변수들
volatile uint8_t button_pressed = 0;
volatile uint16_t pressed_button_pin = 0;

// 가변저항 관련 변수
uint32_t adc_value = 0;
uint16_t servo_pulse_from_pot = SERVO_MIN_PULSE;
uint32_t last_adc_read = 0;
uint32_t last_adc_value = 0;  // 이전 ADC 값 저장
uint8_t keyboard_control_active = 0;  // 키보드 제어 활성화 플래그
uint32_t keyboard_control_timeout = 0;  // 키보드 제어 타임아웃
#define ADC_DEADZONE 50        // ADC 데드존 (50 이상 변해야 업데이트)
#define ADC_UPDATE_INTERVAL 200  // ADC 업데이트 주기 (200ms)
#define KEYBOARD_CONTROL_TIMEOUT 3000  // 키보드 제어 후 3초간 가변저항 제어 비활성화
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
// 함수 프로토타입
void show_main_menu(void);
void show_servo_menu(void);
void show_led_menu(void);
void show_stepper_menu(void);
// void process_uart_input(char input_char); // 폴링 방식 사용으로 제거
void process_menu_input(char input_char);
void stepper_move_to_angle(int32_t target_angle);
void stepper_step(uint8_t direction, uint32_t steps);
void delay_us(uint32_t us);
void servo_move_to_pulse(uint16_t target_pulse);
void servo_close(void);
void led1_on(void);
void led1_off(void);
void led2_on(void);
void led2_off(void);
void test_buttons(void);
void read_potentiometer(void);
void update_servo_from_potentiometer(void);
// void uart_receive_callback(void); // 폴링 방식 사용으로 제거
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// 기존 인터럽트 코드와 동일한 디바운싱 딜레이 함수
static void delay_int_count(volatile unsigned int nTime)
{
    for (;nTime>0;nTime--);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_PIN)
{
    // LED 버튼만 인터럽트로 처리 (버튼 2개만 사용: LED1용, LED2용)
    // 더 안정적인 디바운싱을 위해 else if 사용
    if (GPIO_PIN == LED_BTN1_PIN)  // PC2 - LED1만 토글
    {
        // LED1만 토글 (LED2는 절대 건드리지 않음)
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_PIN);
        led_state.led1_state = HAL_GPIO_ReadPin(LED1_GPIO_Port, LED1_PIN);
        printf("[BUTTON1] LED1 버튼 눌림 - LED1 상태: %s\r\n", led_state.led1_state ? "켜짐" : "꺼짐");
        delay_int_count(10000000);  // 더 긴 디바운싱 딜레이
    }
    else if (GPIO_PIN == LED_BTN2_PIN)  // PC3 - LED2만 토글
    {
        // LED2만 토글 (LED1은 절대 건드리지 않음)
        HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_PIN);
        led_state.led2_state = HAL_GPIO_ReadPin(LED2_GPIO_Port, LED2_PIN);
        printf("[BUTTON2] LED2 버튼 눌림 - LED2 상태: %s\r\n", led_state.led2_state ? "켜짐" : "꺼짐");
        delay_int_count(10000000);  // 더 긴 디바운싱 딜레이
    }
}

// 버튼 테스트 함수
void test_buttons(void)
{
    printf("[BUTTON_TEST] 버튼 테스트 시작...\r\n");
    
    // 각 버튼 핀 상태 확인 (2개만 사용)
    printf("[BUTTON_TEST] PC2(LED_BTN1) 상태: %d\r\n", HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2));
    printf("[BUTTON_TEST] PC3(LED_BTN2) 상태: %d\r\n", HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_3));
    
    // LED 직접 제어 테스트
    printf("[BUTTON_TEST] LED 직접 제어 테스트...\r\n");
    printf("[BUTTON_TEST] LED1 켜기\r\n");
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_Delay(500);
    printf("[BUTTON_TEST] LED1 끄기\r\n");
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_Delay(500);
    
    printf("[BUTTON_TEST] LED2 켜기\r\n");
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
    HAL_Delay(500);
    printf("[BUTTON_TEST] LED2 끄기\r\n");
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
    
    printf("[BUTTON_TEST] 버튼 테스트 완료\r\n");
}

// LED1 켜기
void led1_on(void)
{
    printf("[DEBUG] led1_on() 함수 호출됨\r\n");
    printf("[DEBUG] 현재 LED1 상태: %d\r\n", led_state.led1_state);
    printf("[DEBUG] LED1 핀 설정: GPIO_PIN=%d, GPIO_Port=0x%08lX\r\n", LED1_PIN, (uint32_t)LED1_GPIO_Port);
    
    if (led_state.led1_state) {
        printf("LED1이 이미 켜져 있습니다.\r\n");
        return;
    }
    
    printf("[DEBUG] LED1 핀을 HIGH로 설정\r\n");
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_PIN, GPIO_PIN_SET);
    led_state.led1_state = 1;
    printf("LED1이 켜져 있습니다.\r\n");
}

// LED1 끄기
void led1_off(void)
{
    printf("[DEBUG] led1_off() 함수 호출됨\r\n");
    printf("[DEBUG] 현재 LED1 상태: %d\r\n", led_state.led1_state);
    
    if (!led_state.led1_state) {
        printf("LED1이 이미 꺼져 있습니다.\r\n");
        return;
    }
    
    printf("[DEBUG] LED1 핀을 LOW로 설정\r\n");
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_PIN, GPIO_PIN_RESET);
    led_state.led1_state = 0;
    printf("LED1이 꺼져 있습니다.\r\n");
}

// LED2 켜기
void led2_on(void)
{
    printf("[DEBUG] led2_on() 함수 호출됨\r\n");
    printf("[DEBUG] 현재 LED2 상태: %d\r\n", led_state.led2_state);
    printf("[DEBUG] LED2 핀 설정: GPIO_PIN=%d, GPIO_Port=0x%08lX\r\n", LED2_PIN, (uint32_t)LED2_GPIO_Port);
    
    if (led_state.led2_state) {
        printf("LED2가 이미 켜져 있습니다.\r\n");
        return;
    }
    
    printf("[DEBUG] LED2 핀을 HIGH로 설정\r\n");
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_PIN, GPIO_PIN_SET);
    led_state.led2_state = 1;
    printf("LED2가 켜져 있습니다.\r\n");
}

// LED2 끄기
void led2_off(void)
{
    printf("[DEBUG] led2_off() 함수 호출됨\r\n");
    printf("[DEBUG] 현재 LED2 상태: %d\r\n", led_state.led2_state);
    
    if (!led_state.led2_state) {
        printf("LED2가 이미 꺼져 있습니다.\r\n");
        return;
    }
    
    printf("[DEBUG] LED2 핀을 LOW로 설정\r\n");
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_PIN, GPIO_PIN_RESET);
    led_state.led2_state = 0;
    printf("LED2가 꺼져 있습니다.\r\n");
}

// 서보모터를 지정된 펄스로 이동
void servo_move_to_pulse(uint16_t target_pulse)
{
    printf("[DEBUG] servo_move_to_pulse() 함수 호출됨: target_pulse=%d\r\n", target_pulse);
    
    // 펄스 범위 제한 (1000 이하, 2350 이상에서는 회전 중지)
    if (target_pulse < SERVO_MIN_PULSE) {
        printf("[WARNING] 펄스값이 최소값(%d) 미만입니다. 서보모터 회전 중지.\r\n", SERVO_MIN_PULSE);
        target_pulse = SERVO_MIN_PULSE;
    }
    if (target_pulse > SERVO_MAX_PULSE) {
        printf("[WARNING] 펄스값이 최대값(%d) 초과입니다. 서보모터 회전 중지.\r\n", SERVO_MAX_PULSE);
        target_pulse = SERVO_MAX_PULSE;
    }
    
    printf("[DEBUG] 펄스 범위 제한 후: target_pulse=%d\r\n", target_pulse);
    
    // 현재 상태와 같은지 확인
    if (servo_state.current_pulse == target_pulse) {
        printf("서보모터가 이미 해당 위치에 있습니다.\r\n");
        return;
    }
    
    printf("[DEBUG] PWM 출력 설정: TIM2->CCR1 = %d\r\n", target_pulse);
    // PWM 출력
    TIM2->CCR1 = target_pulse;
    
    // 상태 업데이트
    servo_state.current_pulse = target_pulse;
    servo_state.is_open = (target_pulse >= 1675) ? 1 : 0;  // 중간값 기준
    
    printf("[DEBUG] 상태 업데이트: current_pulse=%d, is_open=%d\r\n", servo_state.current_pulse, servo_state.is_open);
    
    // 상태 출력
    if (servo_state.is_open) {
        printf("가스가 열렸습니다. (펄스: %d)\r\n", target_pulse);
    } else {
        printf("가스가 닫혔습니다. (펄스: %d)\r\n", target_pulse);
    }
}

// 서보모터 열기 함수 제거됨 - 가변저항으로만 제어

// 서보모터 닫기
void servo_close(void)
{
    printf("[DEBUG] servo_close() 함수 호출됨\r\n");
    printf("[DEBUG] 현재 서보 상태: is_open=%d, current_pulse=%d\r\n", servo_state.is_open, servo_state.current_pulse);
    
    if (!servo_state.is_open) {
        printf("가스가 이미 닫혀있습니다.\r\n");
        return;
    }
    
    printf("[DEBUG] 가변저항으로 설정된 위치(%d)에서 닫기 위치(%d)로 이동\r\n", servo_state.current_pulse, SERVO_MIN_PULSE);
    
    // 키보드 제어 활성화 (3초간 가변저항 제어 비활성화)
    keyboard_control_active = 1;
    keyboard_control_timeout = HAL_GetTick();
    
    servo_move_to_pulse(SERVO_MIN_PULSE);
    printf("[SERVO] 가변저항 제어에서 키보드 제어로 전환: 닫기 완료\r\n");
    printf("[SERVO] 키보드 제어 활성화 - 3초간 가변저항 제어 비활성화\r\n");
}

// 가변저항 값 읽기 (간단한 ADC 직접 접근)
void read_potentiometer(void)
{
    // ADC1 초기화 (한번만)
    static uint8_t adc_initialized = 0;
    if (!adc_initialized) {
        // ADC1 클럭 활성화
        __HAL_RCC_ADC1_CLK_ENABLE();
        
        // ADC1 설정
        ADC1->CR2 |= ADC_CR2_ADON;  // ADC 활성화
        ADC1->CR2 |= ADC_CR2_CONT;  // 연속 변환 모드
        ADC1->SQR3 = ADC_CHANNEL_1; // 채널 1 선택
        ADC1->CR2 |= ADC_CR2_SWSTART; // 변환 시작
        
        adc_initialized = 1;
    }
    
    // ADC 값 읽기 (출력 제거)
    adc_value = ADC1->DR;
}

// 가변저항 값에 따라 서보모터 각도 업데이트 (데드존 적용)
void update_servo_from_potentiometer(void)
{
    // 키보드 제어가 활성화되어 있으면 가변저항 제어 비활성화
    if (keyboard_control_active) {
        // 키보드 제어 타임아웃 체크
        if (HAL_GetTick() - keyboard_control_timeout > KEYBOARD_CONTROL_TIMEOUT) {
            keyboard_control_active = 0;
            printf("[SERVO] 키보드 제어 타임아웃 - 가변저항 제어 재활성화\r\n");
        } else {
            return;  // 키보드 제어 중이면 가변저항 제어 무시
        }
    }
    
    // ADC 값 변화량 계산
    uint32_t adc_diff = (adc_value > last_adc_value) ? 
                       (adc_value - last_adc_value) : 
                       (last_adc_value - adc_value);
    
    // 데드존 체크: 변화량이 충분할 때만 업데이트
    if (adc_diff < ADC_DEADZONE) {
        return;  // 변화량이 작으면 무시
    }
    
    // ADC 값을 서보모터 펄스로 변환 (0-4095 -> 1000-2350)
    uint16_t new_pulse = SERVO_MIN_PULSE + (adc_value * SERVO_PULSE_RANGE) / 4095;
    
    // 펄스 범위 제한
    if (new_pulse < SERVO_MIN_PULSE) new_pulse = SERVO_MIN_PULSE;
    if (new_pulse > SERVO_MAX_PULSE) new_pulse = SERVO_MAX_PULSE;
    
    // 값이 변경되었을 때만 서보모터 업데이트
    if (new_pulse != servo_pulse_from_pot) {
        servo_pulse_from_pot = new_pulse;
        servo_move_to_pulse(new_pulse);
        printf("[POTENTIOMETER] 가변저항으로 서보모터 제어: ADC=%lu, 펄스=%d (변화량=%lu)\r\n", 
               adc_value, new_pulse, adc_diff);
        printf("[SERVO] 가변저항 제어 활성화 - 키보드 'c' 입력으로 닫기 가능\r\n");
    }
    
    // 이전 ADC 값 업데이트
    last_adc_value = adc_value;
}

int __io_putchar(int ch)	// printf 출력을 할 수 있는 기능
{
	HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, 0xFFFF);
	return ch;
}

// 마이크로초 지연 함수
void delay_us(uint32_t us)
{
    uint32_t start = HAL_GetTick() * 1000; // 밀리초를 마이크로초로 변환
    while ((HAL_GetTick() * 1000 - start) < us);
}

// 28BYJ-48 스테퍼 모터 스텝 실행 함수 (4상 제어)
void stepper_step(uint8_t direction, uint32_t steps)
{
    // 4상 스테퍼 모터 시퀀스 (하프 스텝 모드)
    uint8_t step_sequence[8][4] = {
        {1, 0, 0, 0},  // 스텝 0
        {1, 1, 0, 0},  // 스텝 1
        {0, 1, 0, 0},  // 스텝 2
        {0, 1, 1, 0},  // 스텝 3
        {0, 0, 1, 0},  // 스텝 4
        {0, 0, 1, 1},  // 스텝 5
        {0, 0, 0, 1},  // 스텝 6
        {1, 0, 0, 1}   // 스텝 7
    };
    
    static uint8_t current_step = 0;
    
    for (uint32_t i = 0; i < steps; i++)
    {
        // 현재 스텝에 해당하는 핀 상태 설정
        HAL_GPIO_WritePin(STEPPER_IN1_GPIO_Port, STEPPER_IN1_PIN, 
                          step_sequence[current_step][0] ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(STEPPER_IN2_GPIO_Port, STEPPER_IN2_PIN, 
                          step_sequence[current_step][1] ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(STEPPER_IN3_GPIO_Port, STEPPER_IN3_PIN, 
                          step_sequence[current_step][2] ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(STEPPER_IN4_GPIO_Port, STEPPER_IN4_PIN, 
                          step_sequence[current_step][3] ? GPIO_PIN_SET : GPIO_PIN_RESET);
        
        // 다음 스텝으로 이동
        if (direction) {
            current_step = (current_step + 1) % 8;  // 시계방향
        } else {
            current_step = (current_step + 7) % 8;  // 반시계방향
        }
        
        // 스텝 간 지연
        HAL_Delay(STEP_DELAY_MS);
    }
}

// 28BYJ-48 스테퍼 모터를 지정된 각도로 이동 (상대적 이동)
void stepper_move_to_angle(int32_t target_angle)
{
    if (target_angle < 0) target_angle = 0;
    if (target_angle > MAX_ANGLE) target_angle = MAX_ANGLE;
    
    int32_t angle_diff = target_angle - stepper_state.current_position;
    
    if (angle_diff == 0) {
        printf("이미 해당 위치에 있습니다.\r\n");
        return;
    }
    
    // 각도를 스텝 수로 변환 (100 = 한바퀴 = 2048스텝)
    uint32_t steps_to_move = (abs(angle_diff) * STEPS_PER_REVOLUTION) / MAX_ANGLE;
    uint8_t direction = (angle_diff > 0) ? 1 : 0;
    
    printf("28BYJ-48 스테퍼 모터 이동: %ld도 -> %ld도 (%lu 스텝, %s방향)\r\n",
           stepper_state.current_position, target_angle, steps_to_move,
           direction ? "시계" : "반시계");
    
    // 스텝 실행
    stepper_step(direction, steps_to_move);
    
    // 현재 위치 업데이트
    stepper_state.current_position = target_angle;
    
    printf("이동 완료. 현재 위치: %ld도\r\n", stepper_state.current_position);
}

// 메인 메뉴 표시
void show_main_menu(void)
{
    printf("\r\n=== 메뉴 선택 ===\r\n");
    printf("1. 서보모터 회전\r\n");
    printf("2. LED 제어\r\n");
    printf("3. 스텝모터 회전\r\n");
    printf("선택하세요 (1, 2 또는 3): ");
}

// LED 메뉴 표시
void show_led_menu(void)
{
    printf("\r\n=== LED 제어 ===\r\n");
    printf("현재 상태: LED1=%s, LED2=%s\r\n", 
           led_state.led1_state ? "켜짐" : "꺼짐",
           led_state.led2_state ? "켜짐" : "꺼짐");
    printf("1. LED1 켜기 (1 + 엔터)\r\n");
    printf("2. LED1 끄기 (2 + 엔터)\r\n");
    printf("3. LED2 켜기 (3 + 엔터)\r\n");
    printf("4. LED2 끄기 (4 + 엔터)\r\n");
    printf("메인 메뉴로 돌아가기 (m + 엔터)\r\n");
    printf("명령을 입력하세요 (엔터로 확인): ");
}

// 서보모터 메뉴 표시
void show_servo_menu(void)
{
    printf("\r\n=== 서보모터 제어 ===\r\n");
    printf("현재 상태: %s (펄스: %d)\r\n", 
           servo_state.is_open ? "열림" : "닫힘", servo_state.current_pulse);
    printf("제어 상태: %s\r\n", keyboard_control_active ? "키보드 제어 중 (3초간)" : "가변저항 제어 중");
    printf("가변저항 제어:\r\n");
    printf("- 가변저항으로 각도 제어 (1000-2350 펄스 범위)\r\n");
    printf("- 1000 이하, 2350 이상에서는 회전 중지\r\n");
    printf("- 가변저항으로 열기 → 키보드 'c'로 닫기 가능\r\n");
    printf("키보드 제어:\r\n");
    printf("- 'c' 또는 'C': 닫기 (%d 펄스) - 3초간 가변저항 제어 비활성화\r\n", SERVO_MIN_PULSE);
    printf("- 'm' 또는 'M': 메인 메뉴로 돌아가기\r\n");
    printf("명령을 입력하세요 (엔터로 확인): ");
}

// 28BYJ-48 스테퍼 모터 메뉴 표시
void show_stepper_menu(void)
{
    printf("\r\n=== 28BYJ-48 스테퍼 모터 제어 ===\r\n");
    printf("현재 위치: %ld도 (100 = 한바퀴)\r\n", stepper_state.current_position);
    printf("목표 각도를 입력하세요 (0-100, 엔터로 확인): ");
}

// UART 입력 처리 (폴링 방식 사용으로 비활성화)
/*
void process_uart_input(char input_char)
{
    if (current_menu == MENU_STEPPER_ANGLE_INPUT)
    {
        if (input_char == '\r' || input_char == '\n')
        {
            // 엔터키 입력 시 각도 처리
            input_buffer[input_index] = '\0';
            int32_t angle = atoi(input_buffer);
            
            if (angle >= 0 && angle <= MAX_ANGLE)
            {
                stepper_move_to_angle(angle);
                printf("\r\n다시 회전하시겠습니까? (y/n): ");
                current_menu = MENU_STEPPER;
            }
            else
            {
                printf("\r\n잘못된 각도입니다. 0-100 범위로 입력하세요: ");
            }
            
            input_index = 0;
            memset(input_buffer, 0, sizeof(input_buffer));
        }
        else if (input_char >= '0' && input_char <= '9')
        {
            // 숫자 입력
            if (input_index < sizeof(input_buffer) - 1)
            {
                input_buffer[input_index++] = input_char;
                printf("%c", input_char);
            }
        }
        else if (input_char == '\b' || input_char == 127)
        {
            // 백스페이스
            if (input_index > 0)
            {
                input_index--;
                input_buffer[input_index] = '\0';
                printf("\b \b");
            }
        }
    }
    else
    {
        process_menu_input(input_char);
    }
}
*/

// 메뉴 입력 처리
void process_menu_input(char input_char)
{
    printf("[DEBUG] 메뉴 입력 처리: '%c' (0x%02X)\r\n", input_char, input_char);
    
    switch (current_menu)
    {
        case MENU_MAIN:
            if (input_char == '1')
            {
                printf("서보모터 메뉴 선택됨\r\n");
                current_menu = MENU_SERVO;
                show_servo_menu();
            }
            else if (input_char == '2')
            {
                printf("LED 메뉴 선택됨\r\n");
                current_menu = MENU_LED;
                show_led_menu();
            }
            else if (input_char == '3')
            {
                printf("스테퍼 모터 메뉴 선택됨\r\n");
                current_menu = MENU_STEPPER;
                show_stepper_menu();
            }
            else
            {
                printf("잘못된 입력입니다. 1, 2 또는 3을 입력하세요.\r\n");
                show_main_menu();
            }
            break;
            
        case MENU_SERVO:
            if (input_char == 'm' || input_char == 'M')
            {
                current_menu = MENU_MAIN;
                show_main_menu();
            }
            break;
            
        case MENU_LED:
            if (input_char == '1')
            {
                led1_on();
                show_led_menu();
            }
            else if (input_char == '2')
            {
                led1_off();
                show_led_menu();
            }
            else if (input_char == '3')
            {
                led2_on();
                show_led_menu();
            }
            else if (input_char == '4')
            {
                led2_off();
                show_led_menu();
            }
            else if (input_char == 'm' || input_char == 'M')
            {
                current_menu = MENU_MAIN;
                show_main_menu();
            }
            else
            {
                printf("잘못된 입력입니다. 1, 2, 3, 4 또는 m을 입력하세요.\r\n");
                show_led_menu();
            }
            break;
            
        case MENU_SERVO_INPUT:
            // 서보모터 입력 모드에서는 메인 루프에서 처리
            break;
            
        case MENU_LED_INPUT:
            // LED 입력 모드에서는 메인 루프에서 처리
            break;
            
        case MENU_STEPPER:
            if (input_char >= '0' && input_char <= '9')
            {
                // 숫자 입력 시 바로 각도 입력 모드로 전환
                current_menu = MENU_STEPPER_ANGLE_INPUT;
                input_index = 0;
                memset(input_buffer, 0, sizeof(input_buffer));
                input_buffer[input_index++] = input_char;
                printf("\r\n목표 각도: %c", input_char);
            }
            else if (input_char == 'm' || input_char == 'M')
            {
                current_menu = MENU_MAIN;
                show_main_menu();
            }
            else
            {
                printf("잘못된 입력입니다. 0-9 숫자 또는 'm'을 입력하세요.\r\n");
                show_stepper_menu();
            }
            break;
            
        case MENU_STEPPER_ANGLE_INPUT:
            // 각도 입력 모드에서는 메인 루프에서 처리
            break;
    }
}

// UART 수신 콜백 함수 (폴링 방식 사용으로 비활성화)
/*
void uart_receive_callback(void)
{
    char received_char = uart_rx_buffer[0];
    
    // 디버그용: 수신된 문자 출력
    printf("[DEBUG] 수신된 문자: 0x%02X ('%c')\r\n", received_char, received_char);
    
    process_uart_input(received_char);
    
    // 다음 수신을 위해 다시 시작
    HAL_UART_Receive_IT(&huart2, uart_rx_buffer, 1);
}
*/
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
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  
  // 서보모터 초기화 (닫힘 상태)
  servo_move_to_pulse(SERVO_MIN_PULSE);
  
  // 28BYJ-48 스테퍼 모터 초기화
  HAL_GPIO_WritePin(STEPPER_IN1_GPIO_Port, STEPPER_IN1_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(STEPPER_IN2_GPIO_Port, STEPPER_IN2_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(STEPPER_IN3_GPIO_Port, STEPPER_IN3_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(STEPPER_IN4_GPIO_Port, STEPPER_IN4_PIN, GPIO_PIN_RESET);
  
  // UART 폴링 방식 사용 (인터럽트 방식 제거)
  // HAL_UART_Receive_IT(&huart2, uart_rx_buffer, 1);
  
  // 입력 버퍼 초기화
  input_index = 0;
  memset(input_buffer, 0, sizeof(input_buffer));
  
  // UART 상태 확인
  printf("[INFO] UART 초기화 완료\r\n");
  printf("[INFO] UART 폴링 방식으로 입력 처리\r\n");
  
  // 가변저항 초기값 설정
  read_potentiometer();
  last_adc_value = adc_value;
  
  // 초기 메뉴 표시
  printf("\r\n=== STM32 모터 제어 시스템 ===\r\n");
  printf("[INIT] 시스템 초기화 완료\r\n");
  printf("[INIT] UART 통신 준비 완료\r\n");
  printf("[INIT] LED 버튼: PC2(LED_BTN1), PC3(LED_BTN2) - 인터럽트+디바운싱 (GPIO_PULLUP, FALLING)\r\n");
  printf("[INIT] LED 핀들: PB8(LED1), PB9(LED2)\r\n");
  printf("[INIT] 가변저항: PA1(ADC1_IN1) - 서보모터 각도 제어\r\n");
  printf("[INIT] LED 핀 상태: LED1=%s, LED2=%s\r\n", 
         led_state.led1_state ? "켜짐" : "꺼짐",
         led_state.led2_state ? "켜짐" : "꺼짐");
  
  printf("\r\n=== 버튼 연결 가이드 ===\r\n");
  printf("PC2(LED_BTN1) ──[버튼]── GND (LED1 토글, 풀업 사용)\r\n");
  printf("PC3(LED_BTN2) ──[버튼]── GND (LED2 토글, 풀업 사용)\r\n");
  printf("PA1 ──[가변저항]── 3.3V (서보모터 각도 제어)\r\n");
  printf("========================\r\n");
  
  // LED 테스트
  printf("[TEST] LED 테스트 시작...\r\n");
  printf("[TEST] LED1 켜기 테스트\r\n");
  led1_on();
  HAL_Delay(1000);
  printf("[TEST] LED1 끄기 테스트\r\n");
  led1_off();
  HAL_Delay(1000);
  printf("[TEST] LED2 켜기 테스트\r\n");
  led2_on();
  HAL_Delay(1000);
  printf("[TEST] LED2 끄기 테스트\r\n");
  led2_off();
  printf("[TEST] LED 테스트 완료\r\n");
  
  // 버튼 테스트
  printf("[TEST] 버튼 테스트 시작...\r\n");
  test_buttons();
  
  show_main_menu();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // 가변저항 읽기 및 서보모터 제어 (200ms마다, 데드존 적용)
    if (HAL_GetTick() - last_adc_read > ADC_UPDATE_INTERVAL)
    {
        read_potentiometer();
        update_servo_from_potentiometer();
        last_adc_read = HAL_GetTick();
    }
    
    // 가스 열림 상태 모니터링 (5초마다)
    static uint32_t last_gas_monitor = 0;
    if (HAL_GetTick() - last_gas_monitor > 5000)  // 5초마다
    {
        if (servo_state.current_pulse > SERVO_MIN_PULSE)  // 1001 이상이면
        {
            printf("[GAS_MONITOR] 가스가 열려있음 (펄스: %d)\r\n", servo_state.current_pulse);
        }
        last_gas_monitor = HAL_GetTick();
    }
    
    // 버튼 상태 모니터링 제거 (불필요한 출력 방지)
    
    // UART 폴링 방식으로 입력 처리
    char received_char;
    if (HAL_UART_Receive(&huart2, (uint8_t*)&received_char, 1, 10) == HAL_OK)
    {
      // 디버그용: 수신된 문자 출력
      printf("[DEBUG] 수신된 문자: 0x%02X ('%c'), 현재 메뉴: %d, 입력 인덱스: %d\r\n", 
             received_char, received_char, current_menu, input_index);
      
      // 간단한 입력 처리
      if (received_char == '1' && current_menu == MENU_MAIN)
      {
        printf("서보모터 메뉴 선택됨\r\n");
        current_menu = MENU_SERVO;
        show_servo_menu();
      }
      else if (received_char == '2' && current_menu == MENU_MAIN)
      {
        printf("LED 메뉴 선택됨\r\n");
        current_menu = MENU_LED;
        show_led_menu();
      }
      else if (received_char == '3' && current_menu == MENU_MAIN)
      {
        printf("스테퍼 모터 메뉴 선택됨\r\n");
        current_menu = MENU_STEPPER;
        show_stepper_menu();
      }
      else if (received_char == 'm' || received_char == 'M')
      {
        // 입력 버퍼 초기화
        input_index = 0;
        memset(input_buffer, 0, sizeof(input_buffer));
        
        printf("메인 메뉴로 돌아가기\r\n");
        current_menu = MENU_MAIN;
        show_main_menu();
      }
      else if ((received_char == 'c' || received_char == 'C') && current_menu == MENU_SERVO)
      {
        // 서보모터 닫기 명령 입력
        if (input_index < sizeof(input_buffer) - 1)
        {
          input_buffer[input_index++] = received_char;
          printf("%c", received_char);
        }
      }
      else if ((received_char == '1' || received_char == '2' || received_char == '3' || received_char == '4') && current_menu == MENU_LED)
      {
        // LED 명령 입력 처리
        if (input_index < sizeof(input_buffer) - 1)
        {
          input_buffer[input_index++] = received_char;
          printf("%c", received_char);
        }
      }
      else if ((received_char == '\r' || received_char == '\n') && current_menu == MENU_LED && input_index > 0)
      {
        // 엔터키 입력 시 LED 명령 처리
        input_buffer[input_index] = '\0';
        char command = input_buffer[0];
        
        printf("\r\nLED 명령 실행: %c\r\n", command);
        
        if (command == '1')
        {
          led1_on();
        }
        else if (command == '2')
        {
          led1_off();
        }
        else if (command == '3')
        {
          led2_on();
        }
        else if (command == '4')
        {
          led2_off();
        }
        else
        {
          printf("잘못된 LED 명령입니다. 1, 2, 3, 4 중 하나를 입력하세요.\r\n");
        }
        
        // 입력 버퍼 초기화
        input_index = 0;
        memset(input_buffer, 0, sizeof(input_buffer));
        show_led_menu();
      }
      else if ((received_char == '\r' || received_char == '\n') && current_menu == MENU_SERVO && input_index > 0)
      {
        // 엔터키 입력 시 서보모터 명령 처리
        input_buffer[input_index] = '\0';
        char command = input_buffer[0];
        
        printf("\r\n서보모터 명령 실행: %c\r\n", command);
        
        if (command == 'c' || command == 'C')
        {
          servo_close();
        }
        else
        {
          printf("잘못된 명령입니다. 'c'를 입력하세요.\r\n");
        }
        
        // 입력 버퍼 초기화
        input_index = 0;
        memset(input_buffer, 0, sizeof(input_buffer));
        show_servo_menu();
      }
      else if (received_char == '\b' && current_menu == MENU_SERVO && input_index > 0)
      {
        // 백스페이스 처리
        input_index--;
        input_buffer[input_index] = '\0';
        printf("\b \b");
      }
      else if (current_menu == MENU_SERVO && input_index > 0)
      {
        // 서보모터 메뉴에서 명령 입력 중에는 다른 키 무시
        printf("\r\n'c'만 입력하세요. 현재 입력: %s\r\n", input_buffer);
      }
      else if (received_char >= '0' && received_char <= '9' && current_menu == MENU_STEPPER)
      {
        // 숫자 입력 처리 (다중 숫자 지원)
        if (input_index < sizeof(input_buffer) - 1)
        {
          input_buffer[input_index++] = received_char;
          printf("%c", received_char);
        }
        else
        {
          printf("\r\n입력이 너무 깁니다. 최대 3자리까지 입력 가능합니다.\r\n");
        }
      }
      else if ((received_char == '\r' || received_char == '\n') && current_menu == MENU_STEPPER && input_index > 0)
      {
        // 엔터키 입력 시 각도 처리
        input_buffer[input_index] = '\0';
        int32_t angle = atoi(input_buffer);
        
        printf("\r\n각도 입력 완료: %ld도\r\n", angle);
        
        if (angle >= 0 && angle <= 100)
        {
          stepper_move_to_angle(angle);
        }
        else
        {
          printf("잘못된 각도입니다. 0-100 범위로 입력하세요.\r\n");
        }
        
        // 입력 버퍼 초기화
        input_index = 0;
        memset(input_buffer, 0, sizeof(input_buffer));
        show_stepper_menu();
      }
      else if (received_char == '\b' && current_menu == MENU_STEPPER && input_index > 0)
      {
        // 백스페이스 처리
        input_index--;
        input_buffer[input_index] = '\0';
        printf("\b \b");
      }
      else if (current_menu == MENU_STEPPER && input_index > 0)
      {
        // 스테퍼 모터 메뉴에서 숫자 입력 중에는 다른 키 무시
        printf("\r\n숫자만 입력하세요. 현재 입력: %s\r\n", input_buffer);
      }
      else
      {
        printf("잘못된 입력: '%c' (0x%02X)\r\n", received_char, received_char);
        if (current_menu == MENU_MAIN)
        {
          show_main_menu();
        }
        else if (current_menu == MENU_STEPPER)
        {
          show_stepper_menu();
        }
      }
    }
    
    // 짧은 지연
    HAL_Delay(1);
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
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
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 84-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 20000-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(STEPPER_IN1_GPIO_Port, STEPPER_IN1_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(STEPPER_IN2_GPIO_Port, STEPPER_IN2_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(STEPPER_IN3_GPIO_Port, STEPPER_IN3_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(STEPPER_IN4_GPIO_Port, STEPPER_IN4_PIN, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED_BTN1_PIN LED_BTN2_PIN */
  GPIO_InitStruct.Pin = LED_BTN1_PIN|LED_BTN2_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;  // 버튼을 눌렀을 때 LOW가 되도록 변경
  GPIO_InitStruct.Pull = GPIO_PULLUP;            // 내부 풀업 저항 사용
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin LED1_PIN LED2_PIN */
  GPIO_InitStruct.Pin = LD2_Pin|LED1_PIN|LED2_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : STEPPER_IN1_Pin STEPPER_IN2_Pin STEPPER_IN3_Pin STEPPER_IN4_Pin */
  GPIO_InitStruct.Pin = STEPPER_IN1_PIN|STEPPER_IN2_PIN|STEPPER_IN3_PIN|STEPPER_IN4_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : POTENTIOMETER_Pin */
  GPIO_InitStruct.Pin = POTENTIOMETER_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(POTENTIOMETER_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// UART 폴링 방식 사용으로 인터럽트 콜백 함수들 비활성화
/*
// UART 수신 완료 인터럽트 콜백
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        uart_receive_callback();
    }
}

// UART 에러 인터럽트 콜백
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        printf("[ERROR] UART 에러 발생\r\n");
        // 에러 후 다시 수신 시작
        HAL_UART_Receive_IT(&huart2, uart_rx_buffer, 1);
    }
}
*/
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
