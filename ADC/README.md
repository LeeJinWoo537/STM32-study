# ADC에 대하여

## ADC의 정의

- 📢**Digital**
    - 디지털은 0과 1로 되어 있어서 컴퓨터가 알아 듣기가 쉬움 그래서
    - 예) 버튼이나 PIR 센서같은 0혹은 1 감지가 됐다 안됐다로 나눌 때 좋은게 디지털

- 📢**Analog**
    - 보통 주파수의 흐름으로 이루어진다.
    - 예) FM, AM, ADC다


# ADC로 가변저항
- ADC는 아날로그를 디지털로 변환해주는 거라고 할 수 있다.

- 일단 첫번째로 ADC 구조체 변수에 대해서 알려주겠다.
```c
ADC_HandleTypeDef hadc1;
```
    → 이건 보통 ADC를 생성하면 자동으로 생성되는거다.

```c
int main(void)
{
  /* USER CODE BEGIN 1 */
	float volt1 = 0.0, volt2 = 0.0;
	uint32_t value1 = 0, value2 = 0, ad_max = 4095;
  /* USER CODE END 1 */
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();       // ADC를 초기화 해주는 함수


  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  HAL_ADC_Start(&hadc1);
	  HAL_ADC_PollForConversion(&hadc1, 100);       // 채널 0번째
	  value1 = HAL_ADC_GetValue(&hadc1);
	  HAL_ADC_PollForConversion(&hadc1, 100);       // 채널 1번째
	  value2 = HAL_ADC_GetValue(&hadc1);
	  HAL_ADC_Stop(&hadc1);
	  volt1 = (float)((value1 * 3.3) / ad_max);
	  volt2 = (float)((value2 * 3.3) / ad_max);
	  printf("\n\r adc value1 = %d, volt1 = %1.2f, \t value2 = %d, volt2 = %1.2f", value1, volt1, value2, volt2);
	  HAL_Delay(10);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
```
    → 첫번째 HAL_ADC_Start(); 이거는 ADC를 시작하는 함수라고 할 수 있다.
    → 두번째 HAL_ADC_PollForConversion(&hadc1, 100); hadc1은 말 그대로 adc 구조체 변수를 사용하는거다 좀 더 정확히 이야기하면 채널0번째인 ADC를 선언하는거라고 생각하면 쉽다. 뒤에 있는 100이라는 값은 100밀리초동안값이 들어오는지 안들어오는지를 검사하는거다 그래서 없으면 다음 코드로 넘어간다.
    → HAL_ADC_GetValue(&hadc1); 이문법은 ADC값을 읽는 함수이다 그니까 정확히는 0 ~ 4095까지에 값을 읽는 함수이다.
    → 마지막 HAL_ADC_Stop(&hadc1); 이건 ADC를 실행했으니까 이제 그만 실행하겠다라는 의미이다.
    📌 그래서 STM32에서 ADC는 1개이다 그래서 그 1개 안에서 ADC를 채널을 나눠서 사용을 하는데 그 안에서 우선순위를 정해서 사용을 해야한다. 그래서 HAL_ADC_PollForConversion 이거를 2번 정의하는것도 우선순위를 정해야하기 때문에 그런것이다.



                ------------------------------------------------------------------------------------------------------
## ADC 인터럽트
- 간략하게 이야기해서 계속해서 ADC의 값을 받으면 자원낭비가 심하니까 인터럽트에 값을 받을 때만 값을 읽어서 실행을 하게하는 코드를 설명하겠다.


```c
int main(void)
{
  /* USER CODE BEGIN 1 */
	float volt = 0.0;
  /* USER CODE END 1 */

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADC_Start_IT(&hadc1);         // ADC 인터럽트
  printf("\n ADC test start!");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  volt = (float)((value * 3.3) / ad_max);       // 이거는 0 ~ 4095를 0.0 ~ 3.3으로 나누는 것이다.
	  printf("\n\r adc value = %ld, volt = %1.2f", value, volt);
	  HAL_Delay(100);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
```
    → HAL_ADC_Start_IT(&hadc1); 이거는 ADC 인터럽트를 호출하는 함수로 이게 실행이 되면

```c
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	if (hadc->Instance == ADC1)
	{
		value = HAL_ADC_GetValue(&hadc1);
	}
	HAL_ADC_Start_IT(hadc);
}
```
    → 이 함수가 실행이 된다..
    → 이건 인터럽트 콜백 함수이다 그래서 if문에 있는
    → ADC1가 값이 들어오면 그 때 0 ~ 4095까지 HAL_ADC_GetValue(&hadc1); 값을 읽겠다 라는 뜻이다. 그리고
    → 그리고 HAL_ADC_Start_IT(hadc); 이게 다시 실행이 되는 이유는 뭐냐면 인터럽트 자체가 보통 한번 이벤트가 발생하면 끊기는데 끊기지 않고 계속 인터럽트 발생을 하기 위해서 다시 실행을 하는 것이다.



## ADC DMA
- DMA를 설명을 하자면 DMA는 캐시를 지나고 램을 지나는 과정을 보통 CPU가 싸이클을 돌면서 하는데 이게 조금 CPU 입장에서 낭비가 될 수 있어서 DMA에게 이 역할 하게한다 그러면 CPU는 그동안에 다른 작업을 할 수 있게 하는게 DMA이다 자원을 아낄 수 있는 것인데 이걸 이제
- 이걸 이제 ADC에서 적용을 하면 ADC에서 아날로그 값을 읽으면 그 값을 램에게 보내주는데 CPU가 그걸 DMA에게 넘겨줘서 자원을 아끼게 할 수 있는 것이다 DMA 역시 인터럽트 기능이 있다.

```c
int main(void)
{

  /* USER CODE BEGIN 1 */
	uint16_t buffer[2], ad_max = 4095;
	float volt1, volt2;
  /* USER CODE END 1 */
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)buffer, 2);
  printf("\n ADC test start!");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  volt1 = (float)((buffer[0] * 3.3) / ad_max);
	  volt2 = (float)((buffer[1] * 3.3) / ad_max);
	  printf("\n\r adc buffer[0] = %d, volt1 = %1.2f, \t buffer[1] = %d, volt2 = %1.2f", buffer[0], volt1, buffer[1], volt2);
	  HAL_Delay(100);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
```
    → HAL_ADC_Start_DMA(&hadc1, (uint32_t*)buffer, 2); 이것만 설명을 하자면 buffer 이거는 해당 버퍼 값으로 해당 ADC 값을 읽는 값을 저장하는 곳이고 그 뒤에 있는 2는 ADC채널의 갯수라고 생각하면 된다.