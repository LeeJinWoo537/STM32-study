# Interrupt

특정 이벤트(예: 버튼을 눌렀을 떄나 혹은 타이머같은 일정 시간이 지날 때나)가 발생했을 때 해당 작업중인걸 잠깐 중단하고 이벤트 발생된걸 실행하고 그다음에 다시 돌아와 작업을 한다.
예시)
    ● 모터를 계속 돌려야하는 상황 속에서 버튼을 누르고 LED 키고싶은 상황이면 버튼을 눌렀을 때 인터럽트 발생을 하면 버튼쪽으로 가서 LED 키고 실행하고 다음다시 돌아와 해당 작업했던걸 계속 실행!


## 문법
main.c
```C
static void delay_int_count(volatile unsigned int nTime)     // 초(sec) 값을 받는 변수
{
	for (;nTime>0;nTime--);     // 해당 초(값)가 들어오면 0이될 때까지 계속 반복
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_PIN)  // 값을 받으면
{
	if (GPIO_PIN == BNT1_Pin)       // 해당 GPIO핀과 비교
	{
		HAL_GPIO_WritePin(GPIOC, LED1_Pin, GPIO_PIN_SET);
		delay_int_count(9000000);       // 초 함수 호출
		HAL_GPIO_WritePin(GPIOC, LED1_Pin, GPIO_PIN_RESET);
	}

	if (GPIO_PIN == BNT2_Pin)
	{
		HAL_GPIO_WritePin(GPIOC, LED2_Pin, GPIO_PIN_SET);
		delay_int_count(9000000);       // 초 함수 호출
		HAL_GPIO_WritePin(GPIOC, LED2_Pin, GPIO_PIN_RESET);
	}
}
```

stm32f4xx_hal_gpio.c
```C
void HAL_GPIO_EXTI_IRQHandler(uint16_t GPIO_Pin)    // 해당 인터럽트가 발생하면
{
  /* EXTI line interrupt detected */
  if(__HAL_GPIO_EXTI_GET_IT(GPIO_Pin) != RESET)     //__ 이거는 내부적으로 작동하는 것으로 지금 현재 문법은 인터럽트를 얻어오는 함수
  {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);             // 이 문법은 인터럽트 값을 초기화 해주는 문법
    HAL_GPIO_EXTI_Callback(GPIO_Pin);    // 호출
  }
}

/**
  * @brief  EXTI line detection callbacks.
  * @param  GPIO_Pin Specifies the pins connected EXTI line
  * @retval None
  */
__weak void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)   // 사용자가 코드 작성하는 곳 __weak는 사용자가 작성하라고 알려주는 키워드 그리고 GPIO 인터럽트 생성해놓고 안쓰면 오류가 뜸
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(GPIO_Pin); 
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_GPIO_EXTI_Callback could be implemented in the user file
   */
}
```