## 타이머에 대하여
- 주파수: 어떤 진동이 1초 동안에 몇 번 반복되는지를 나타내는 값
- 주기(Period)는 한 사이클이 반복되는 전체 시간입니다. 예를 들어, 1초에 한 번 반복되는 주기는 1초가 됩니다.

```c
void HAL_SYSTICK_Callback(void)
{
	if((timer_count%1000)==0)
	{
		printf("\n\r 1000ms interation");
		HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_0);
	}
	timer_count++;
}

int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
```
	→ 타이머가 동작이 되면 IRQ가 호출되기전에 HAL_SYSTICK_Callback 해당 이 함수가 실행이 된다.



# 📌 STM32 Timer 설정 방법 기초부터~
 - 1. 해당 STM32툴에 ioc에 가면 System Core가 있는데 
 - 2. 거기에 있는 NVIC로 들어가서 
 - 3. 해당 밑에 NVIC를 보면 Time base: System tick timer이 있다 거기서
 - 4. sub Priority 부분에 0에서 1로 바꾼다 이게 뭐냐면
	- 똑같은 인터럽트 신호가 계속해서 받아야하는데 지금 Preemption Priority에서 보면 지금 지금 0이라는 값들이 다 우선순위를 정하는거다
	- 어?? 근데 그러면 다 우선순위가 똑같으면 비교를 해야하잖아? 그치? 그래서 시스템에서 내부적으로 비교를 하는데 이걸 주 인터럽트라고 한다 여기서 비교를 못하면 부 인터럽트로 넘어가는데 그게 그 오른쪽에있는 Sub Prioity이다 여기서 우선순위를 정하는건데 1로 하는 이유는 이 타이머를 두번째로 하기 위해서이다. 

- 쉽게 이야기해서
```
ioc -> System Core -> NVIC -> Time base: System tick timer -> Preemption Priority -> Sub Prioity 1로 변환
```

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
	printf("\n\r TIM1=> 1000ms interation!!");
	HAL_GPIO_TogglePin(GPIOC, LED_Pin);
}

int main(void)
{
  /* USER CODE BEGIN 1 */
	float volt = 0.0;
  /* USER CODE END 1 */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim1);
  printf("\n TIM1 test start!");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
```

```c
HAL_TIM_Base_Start_IT(&htim1);
```
- 이건 타이머가 시작을 한다는걸 의미한다. 
- 역시나 타이머가 시작이되고 타이머가 동작한다는걸 알면 그 때서야 HAL_TIM_PeriodElapsedCallback 이게 호출이 된다.

📌 이제 Hz관련되서 설명을 할 생각이다.
```
● ioc에 가보면 왼쪽에 있는 나열되어있는 말고 그 위에 Pinout & Configuration이랑 Clock Configuration이랑 Project Manager이랑 Tools가 있다 위에 4개가 있는데

● 그 위에있는 Clock Configuration여기에 들어가면 설정을 해줘야하는데 차근차근 설명을 하겠다.

1. 일단 가운데 보면 SYSCLK(MHz)가 있는데 여기서 72로 바꿔야한다 그 오른쪽에 AHB Prescaler는 1로 그 오른쪽에 있는 HCLK(MHz)이걸 72로 변환
2. 그 오른쪽에 있는 ARB1 Prescaler를 2로 그리고 APB1 Peripheral clocks (MHz)을 36으로 그 밑에있는걸 72로 바꾼다
3. APB2 Peripheral clocks (MHz) 이것도 18 그리고 그 밑에있는 것도 36로 바꾼다
```
- ARB1의 값은 Timer2에 가고 APB2는 Timer1에 값이 간다.
```
그리고 TIM1을 사용을 할건데 TIM1에 들어가서 
```


```
Timers에 들어가서 보면 TIM1에 들어가서 Clock Source를 Internal Clock으로 바꾼다 그리고 그 Configuration밑에 가면 NVIC Settings에 가면 
TIM1 Break interrupt and TIM9 global interrupt에 에 체크를 하고 Sub Priority에 1로 설정 그리고 그밑에 있는

TIM1 update interrupt and TIM10 global interrupt에 있는 Sub Priority를 2로 설정을 해준다
```
```
그리고 Parameter Settings가서 밑에 Counter Settings를 보면 
● Prescaler (PSC - 16 bits value)를 값을 36000 - 1로 해준다
● 그리고 Counter Period (AutoReload Register - 16 bits value)를 값을 1000 - 1로 해준다.
```
- 이걸 해주는 이유가 일단 첫번째 36000을 해주는 이유가 보면 아까 우리가 Timer2에서 36을 해줬는데 이게 그냥 36Hz가 아니라 36MHz이기 때문에 36000000을 36000으로 나눠야하기 때문이다.
- 나누면 1000이 나오는데 1초에 1Hz로 그니까 1초에 LED를 한번 깜빡이고 싶으면 1Hz로 만들어야하는데 나누면 1000이 나오니까 999를 나눠서 1Hz가 나오게 할려고 이다.

- 어?? 근데 그러면 -1은 왜 해? 그냥 999로 하면 안돼?? 라고 할 수 있는데 보통 프로그래밍에서는 1이 아니라 0부터 시작을 하기 때문에 0에서 999까지 가는데 이게 사람이 보기에는 999보다는 1000이 편하기 때문에 이렇게 하는거다 근데 사실상 999로 하는거나 다름이 없다. 


```
그리고 1000 -1 해주고 나서 그 밑에 보면 auto-reload preload가 있는데 이거 Enable해주면 된다.
```



--------------------------------------------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------------------------------------------

📢 PWM 설명
- 펄스 폭 변조라고도 하는데 우리가 아날로그를 사용하려고 하면 보통은 파형 주파수로 되어있다 보니까 값을 측정하거나 올라가는 값 혹은 내려가는 값 이런걸로 사용을 할 수가 있는데 디지털은 0과 1로 되어있다 보니까 값을 측정 못한다 그래서 나온게 PWM이다 PWM은 이제 디지털을 아날로그화 한거라고 생각하면 된다 그렇다고 진짜 아날로그로 변했다가 아니라 HIGH 되는 시간을 측정해 그걸 %로 나타내서 사용을 한다.


## PWM 기본설정부터 해보자 ^^
1. 일단 맨위에 보면 Pinout & Configuration이 있다 그 오른쪽에 바로 Clock Configuration이 있는데 그걸 클릭!
2. 그리고 가운데를 지점으로 왼쪽부분에 잘보면 PLL Source Mux라는게 조금만한 글씨로 써져 있다 거기 밑에 사다리꼴 모양에 HSI랑 HSE라는게 있는 아마 지금 HSI가 지정이 되어있을것이다 그걸 밑에있는 HSE로 지정을 하기 바란다.
3. 그리고 나서 가운데 지점을 보면 System Clock Mux가 있는데 이것도 아마 HSI로 설정되어있을건데 이걸 맨 밑에있는 PLLCLK라는걸 지정한다 그리고
4. 오른쪽에 보면 ARB2 timer clocks (MHz)가 있는데 이걸 36으로 쓰고 엔터!
5. 그리고 다시 Pinout & Configuration 여기로 돌아와서 왼쪽에 Timers에 TIM1에 들어가서 Clock Source에 있는걸 Internal Clock으로 바꾼다 그리고 그밑에있는 Channel1을 PWM Generation CH1으로 설정한다 그 밑에있는 Channel2도 PWM Generation CH2 로 설정을 해준다 왜냐면 LED를 2개 사용할거기 때문이다.
6. 그리고 Prescaler (PSC - 16 bits value) 이걸 360 - 1로 설정을 해준다.
7. 그리고 Counter Mode를 UP으로 설정하고
8. Counter Period (AutoReload Register - 16 bits value )를 이걸 1000 - 1로 설정한다.
9. 그리고  auto-reload Preload를 Enable로 설정을 하고 나서 밑으로 내리면
10. PWM Generation Channel1이 보이는데 그 밑에 Mode가 있는데 이걸 PWM mode1로 설정을 하고 
11. Pulse(16 bits value)가 있는데 이걸 250으로 바꿔준다 왜냐면 LED가 깜빡이는데 250이 기준이 될거기 때문이다.
12. Output commpare Proload를 Enable로 설정을 해준다 나머지는 알아서 되는거라 이렇게 하고 PWM Heneration Channel2도 Mode를 PWM mode1로 맞춰주고 Pulse(16 bits value)를 750으로 하고 그밑에 Output commpare preload를 Enable로 설정을 해주면 끝이 난다.

- 저 Pulse는 뭘 나타내는거냐면 250 그리고 750은 하나의 기준을 나타낸다 좀 있다가 코드 설명을 할 생각인데 HIGH > 250 < LOW, HIGH> 750 < LOW 되도록 LED1과 LED2가 깜빡일것이다 이제 코드로 넘어가자!

```c
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);   // 이건 PWM을 시작하겠다! 라는 의미 구조체 변수와 해당 타이머 채널이다.
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);   // 이것 역시 마찬가지
  printf("\n TIM1 PWM test start!");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  for (int duty = 0; duty < 1000; duty++)     // 0 ~ 999까지 계속 반복을 할거다.
	  {
		  TIM1->CCR1 = duty;      // 해당 값을 TIM1에 구조체에있는 CCR1에 값을 계속 넣겠다.
		  TIM1->CCR2 = duty;
		  HAL_Delay(10);    // 0.01초
		  printf("\n\r Duty Value = %d", duty); 
	  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
```
- 우리는 이코드를 통해 값이 계속 0 ~ 999까지 반복을 하는데 그 안에서 LED1이 250이하이면 HIGH 이상이면 LOW로 가겠다 그리고 LED2는 750 이하면 HIGH 이상이면 LOW로 하겠다는 코드를 작성한거다.


## 다음은 PWM으로 서보모터를 돌릴 생각이다.
1. 일단 맨위에 보면 Pinout & Configuration이 있다 그 오른쪽에 바로 Clock Configuration이 있는데 그걸 클릭!
2. 그리고 가운데를 지점으로 왼쪽부분에 잘보면 PLL Source Mux라는게 조금만한 글씨로 써져 있다 거기 밑에 사다리꼴 모양에 HSI랑 HSE라는게 있는 아마 지금 HSI가 지정이 되어있을것이다 그걸 밑에있는 HSE로 지정을 하기 바란다.
3. 그리고 나서 가운데 지점을 보면 System Clock Mux가 있는데 이것도 아마 HSI로 설정되어있을건데 이걸 맨 밑에있는 PLLCLK라는걸 지정한다 그리고
4. 오른쪽에 보면 ARB2 timer clocks (MHz)가 있는데 이걸 84으로 쓰고 엔터!
5. 그리고 다시 Pinout & Configuration 여기로 돌아와서 왼쪽에 Timers에 TIM1에 들어가서 Clock Source에 있는걸 Internal Clock으로 바꾼다 그리고 그밑에있는 Channel1을 PWM Generation CH1으로 설정한다.
6. 그리고 Prescaler (PSC - 16 bits value) 이걸 84 - 1로 설정을 해준다.
7. 그리고 Counter Mode를 UP으로 설정하고
8. Counter Period (AutoReload Register - 16 bits value )를 이걸 20000 - 1로 설정한다.
9. 그리고  auto-reload Preload를 Enable로 설정을 하고 나서 밑으로 내리면
10. PWM Generation Channel1이 보이는데 그 밑에 Mode가 있는데 이걸 PWM mode1로 설정을 하고 
11. Pulse(16 bits value)가 있는데 이걸 1500으로 바꿔준다 왜냐면 지금 내가 사용하고 있는 이 서보모터가 90º 기준이 1500이기 때문이다.
12. Output commpare prolad를 Enable로 설정하면 끝!

- 아! 그리고 이거 계산하는거 말인데 계산하면 총 1/50 Hz가 나오는데 이거 역순하면 20ms가 나온다.

```c
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);     // PWM을 시작하겠다라는 의미 
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  for(int i = -60; i <= 60; i += 10)
	  {
		  TIM2->CCR1 = 1500 + (i * 1000) / 60;
		  HAL_Delay(1000);    // 1초
	  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
```
- 1초에 10도씩 가는 코드이다.
- 1500 + (-60 * 1000) / 60 -> 500이 된다 이건 서보모터가 0도일 떄 상황이다.
