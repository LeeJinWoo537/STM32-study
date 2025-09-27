# UART통신

📢 UART는 전이중 통신으로 내가 값을 보내면서 받을 수 있는 통신이라고 할 수 있다 비유적으로 이야기하자면 통화 전화를 생각하면 쉽게 생각할 수 있다.

```c
UART_HandleTypeDef huart2;
```

위에 있는게 UART 구조체 변수이다 저 변수 안에서 UART 관련된 코드들이 있다.

```c
int main(void)
{

  /* USER CODE BEGIN 1 */
	uint8_t size;
	uint8_t message[] = "Transmit Test\n\r";
	size = sizeof(message);
  /* USER CODE END 1 */
  HAL_Init();       // HAL 드라이버를 초기화 해주는 함수

  /* Configure the system clock */
  SystemClock_Config();     // 시스템 클럭을 초기화해주는 함수

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  HAL_UART_Transmit(&huart2, message, size, 100);        
	  HAL_Delay(1000);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

HAL_UART_Transmit(); 이거는 UART 통신을 할 수 있는 정확히는 값을 주고 받을 수 있는 문법이다 그래서 앞쪽에 있는 &huart2는 해당 UART를 통신할 수 있는 구조체 변수를 뜻하고 message는 해당 버퍼 그니까 문자열 값이고 size는 배열 크기  그리고 뒤에 있는 100은 블록킹을 의미한다.


// 저 100에 의미는 100밀리초 동안에 저 코드를 계속해서 오류가 없는지 계속 검사한다. 그래서 만약에 값을 보내지 않으면 그 다음 코드로 넘어간다.
// 대신 CPU가 그니까 자원을 많이 낭비를 하게된다 왜냐면 CPU를 계속 사용을 하기 때문에..
```


## ✏️다음 문법은

```c
HAL_UART_Transmit_IT(&huart2, message, size);
HAL_Delay(1000);
```

- 해당 함수는 인터럽트 함수이다 이거는 아까 HAL_UART_Transmit(); 이거랑 출력은 같을지 몰라도 실행되는 과정이 조금 다르다
- 아까 HAL_UART_Transmit(); 이거는 맨 오른쪽에 있는 꼭 100이라는 값일 필요는 없지만 100이는 밀리초를 통해 이 해당 문법을 계속 검사한다 그치만
- HAL_UART_Transmit_IT(); 이거는 검사는 하지 않고 그냥 값이 없으면 바로 다음 코드로 넘어간다.




## 다음!
📢 아까도 이야기했지만 UART는 전이중 통신이다 값을 주고 받고가 가능하다 그래서 이번에는 값을 주고 받고를 할 생각이다.

```c
/* USER CODE BEGIN PV */
uint8_t rxData;
uint8_t txBuf[TX_BUF_SIZE];
/* USER CODE END PV */

/* USER CODE BEGIN 0 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
	if (huart->Instance == USART2)
	{
		const char* msg = "recv\r\n";
		HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
		HAL_UART_Receive_IT(&huart2, &rxData, 1);
	}
}
/* USER CODE END 0 */

int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  HAL_UART_Receive_IT(&huart2, &rxData, 1);

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  const char* periodic = "send\r\n";
  while (1)
  {
	  size_t len = strlen(periodic);
	  HAL_UART_Transmit(&huart2, (uint8_t*)periodic, (uint16_t)len, 100);
	  HAL_Delay(1000);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
```
- 해당 HAL_UART_RxCpltCallback() 함수는 해당 출력창에 아무키나 입력이 됐을 때 이 함수가 호출이 된다.
- 그래서 HAL_UART_Receive_IT() 이것 역시 아까 말했던 인터럽트 IT부분과 같다. 그래서 키가 입력이 됐다고 됐을 때 이 함수가 호출이 된다.
📉 실행순서는 
```c
HAL_UART_RxCpltCallback()   // 1번
HAL_UART_Receive_IT()       // 2번
```
- 이순서대로 진행이 된다.




# DMA에 대해서
📢 DMA는 쉽게 이야기해서 DMA가 메모리에 접근할 동안에 CPU는 다른걸 처리하게 만드는 그래서 속도 그리고 메모리 관련되서 CPU에 자원을 아낄 수 있는 것!

```c
/* USER CODE BEGIN PV */
uint8_t receive_data = 0;
const char* send_msg = "send\n\r";
const char* recv_msg = "recv\n\r";
volatile uint8_t flag_received = 0;
volatile uint8_t last_receive_tick = 0;
/* USER CODE END PV */

/* USER CODE BEGIN PFP */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
	if (huart->Instance == USART2)
	{
		flag_received = 1;
		last_receive_tick = HAL_GetTick();          // HAL_GetTick()은 현재 시간을 재는 거라고 쉽게 이야기하면 그렇게 생각하면 된다.
		HAL_UART_Receive_DMA(&huart2, &receive_data, 1);
	}
}
/* USER CODE END PFP */


int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  /* Configure the system clock */
  SystemClock_Config();
    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART2_UART_Init();
    HAL_UART_Receive_DMA(&huart2, &receive_data, 1);    

    /* USER CODE BEGIN 2 */
    uint32_t last_send_tick = 0;
    /* USER CODE END 2 */

    /* USER CODE BEGIN WHILE */
    while (1)
    {
        uint32_t current_tick = HAL_GetTick();

        if (flag_received == 1)
        {
            flag_received = 0;
            HAL_UART_Transmit(&huart2, (uint8_t*)recv_msg, strlen(recv_msg), 100);
        }

        if ((current_tick - last_receive_tick > 500) && (current_tick - last_send_tick >= 1000))
        {
            HAL_UART_Transmit(&huart2, (uint8_t)send_msg, strlen(send_msg), 100);
            last_send_tick = current_tick;
        }

        if (current_tick - last_receive_tick <= 500)
        {
            last_send_tick = current_tick;
        }
    }
    /* USER CODE END 3 */
}
```
- 아까 UART랑 통신했을 때 출력은 같을지 그 안에 내부 과정 안에서가 다르기 때문이다.