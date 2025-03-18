/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
//Буферы, в ктр помещаем данные для приёма/передачи
uint8_t i2c_data[8]={0};//Обнуляем массив
uint8_t uart_data[4]={0};//Буфер для UART


//uint8_t state[20] = "off";//Переменная состояний имитатора
//Переменные состояний 0 - нет, 1 - да
uint8_t blocked = 0;//Экран заблокирован
uint8_t enabled = 0;//Телефон включен

//Переменные для выполнения касаний
uint8_t Event_flag;// 
uint8_t	XH, XL;
uint8_t Touch_ID ;
uint8_t YH, YL;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void TouchScreenInit(void);//Ф-я для ответов процессору телефона при инициализации тача
/*Ф-я для посылки телефону касания в одной точке. Посылает 4 пакета на 4 запроса
процессора телефона. Вид такой:

00 Ответ 00 00 01 xh xl yh yl 00 
08 Ответ 00 FF FF FF FF FF FF FF
10 Ответ FF FF FF FF FF FF FF FF
A6 Ответ 01

В xh содержится Event Flag (биты 7,6) и XH (биты 3-0).
В yh содержится Touch ID (биты 7-4) и YH (биты 3-0).

*/
void SendPacketOnePoint(uint8_t, uint8_t, uint8_t, uint8_t);
void SendPacketEmpty(void);/*Ф-я для посылки пустых пачек вида
			00 Ответ 00 00 00 FF FF FF FF FF
			08 Ответ FF FF FF FF FF FF FF FF
			10 Ответ FF FF FF FF FF FF FF FF
			A6 Ответ 01
*/

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
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	
	
			
	
  while (1)
  {
		//Ждём команд от компьютера (ждём 4 байта)
		while ( HAL_UART_Receive(&huart1, uart_data, 4, 30) != HAL_OK );
		uint8_t str[] = "Command received\0";
		HAL_UART_Transmit(&huart1, str, 16, 30);
		if (uart_data[0]==0x10)//Если пришла команда сделать касание
		{
			uint8_t str[] = "Making touch\0";
			HAL_UART_Transmit(&huart1, str, 12, 30);
			
			/*Формат такой:Команда, XHYH, XL, YL.
											[0x10, 0x12,0x1A,0xE1])
			*/
//Пачка №1 - касание. Event Flag 00 -  Press Down	
			//Извлекаем из юарта координаты и пакуем  в нужном формате.
			//Event_flag = 0;// Press Down
			XH = (uart_data[1]& 0xF0)>>4;
			XH = XH & 0x3F;//Пишем 00 в 7й и 6й биты ("и"   с 0b0011 1111)
			XL = uart_data[2];
			//Обнуляем биты 7-4 - Пишем 	Touch_ID = 0b0000.
			//Заодно избавляемся от XH
			YH = uart_data[1] & 0x0F;
			YL = uart_data[3];
			
			/*Компилятор почему-то ругается на двочиные числа, что очень странно. 
			Как только их убрал, ошибки пропали. Возможно дело в кодировке.
			uint8_t Event_flag = 0b00;// Press Down	Можно сделать #define Press Down 0b00
			uint8_t	XH = ((uart_data[1]& 0b11110000)>>4) | (Event_flag <<6);
			uint8_t Touch_ID = 0b0000;
			uint8_t YH = (uart_data[1]& 0b00001111) | (Touch_ID <<4);
			*/
			
			SendPacketOnePoint(XH, XL, YH, YL);
			HAL_Delay(6);//Между пачками 6,5 мс. Решил сделать 6 мс. Работает.
//Пачка №2 - удержание Event Flag =  Contact				
			//Event_flag = 2;// 10 - Contact
			//Помещаем новое значение Event_flag в XH
			XH = XH | 0x80;//Пишем 1 в 7й бит ("или" с 0b1000 0000) 
			XH = XH & 0xBF;//Пишем 0 в 6й бит ("и"   с 0b1011 1111)
			
			SendPacketOnePoint(XH, XL, YH, YL);
			HAL_Delay(6);//Задержка между пачками 6,5 мс. Решил сделать 6 мс.
			
//Пачка №3 - отпускание. Event Flag =  Lift Up	
			//Event_flag = 1;// 01 - Lift Up
			//Помещаем новое значение Event_flag в XH
			XH = XH | 0x40;//Пишем 1 в 6й бит ("или" с 0b0100 0000) 
			XH = XH & 0x7F;//Пишем 0 в 7й бит ("и"   с 0b0111 1111)
			
			SendPacketOnePoint(XH, XL, YH, YL);
			HAL_Delay(6);//Задержка между пачками 6,5 мс. Решил сделать 6 мс.

/*Пачки 4-8. 5 пустых пачек вида
			00 Ответ 00 00 00 FF FF FF FF FF
			08 Ответ FF FF FF FF FF FF FF FF
			10 Ответ FF FF FF FF FF FF FF FF
			A6 Ответ 01

*/
			for(uint8_t i = 1; i < 6; i++)
			{
				SendPacketEmpty();	
				HAL_Delay(6);//Задержка между пачками 6,5 мс. Решил сделать 6 мс.
			}

/*Проверить, может удобно будет записать условие и посылать пустую пачку
той же ф-ей SendPacketOnePoint(). Можно использовать значение Event Flag =
No Event, добавить в ф-ю условие. Посмотреть, удобно ли так.
*/

			//В дальнейшем можно отправить переменную состояния с флагами, а пока строка
			uint8_t str2[] = "OK\0";
			HAL_UART_Transmit(&huart1,str2, 2, 30);
				
		}
		else if (uart_data[0]==0x20)//Если пришла команда включить телефон
		{
			//1 Линию PWR_KEY в высокий уровень (Зажимаем кнопку)
			HAL_GPIO_WritePin(PWR_KEY_GPIO_Port, PWR_KEY_Pin, GPIO_PIN_SET);
			//2 Ждём нарастающего фронта сигнала Reset (сброса тача).
			//Так мк точно будет знать, когда телефон включится		
			//При включении Reset  у нас в низком уровне, поэтом ждём высокого			
			while( HAL_GPIO_ReadPin(RST_GPIO_Port, RST_Pin) != GPIO_PIN_SET );
			//3 Линию PWR_KEY в низкий уровень (Отпускаем кнопку)
			HAL_GPIO_WritePin(PWR_KEY_GPIO_Port, PWR_KEY_Pin, GPIO_PIN_RESET);
			//Ждём посылок инициализации тача по i2c 
			//От фронта Reset до первой посылки 110 мс
			TouchScreenInit();
			enabled = 1;//Состояние=включен
			//В дальнейшем можно отправить переменную состояния с флагами, а пока строка
			uint8_t str[] = "Enabled\0";
			HAL_UART_Transmit(&huart1, str, 7, 30);
		}
		else if (uart_data[0]==0x30)//Если пришла команда разблокировать телефон
		{
			//1 Линию PWR_KEY в высокий уровень (Зажимаем кнопку)
			HAL_GPIO_WritePin(PWR_KEY_GPIO_Port, PWR_KEY_Pin, GPIO_PIN_SET);
			// Ждём нарастающего фронта сигнала Reset (сброса тача).
			//Так мк точно будет знать, когда телефон разблокировался
			//2 При разблокировании Reset  у нас в высоком уровне, поэтом ждём низкого			
			while( HAL_GPIO_ReadPin(RST_GPIO_Port, RST_Pin) != GPIO_PIN_RESET );
			//3 Линию PWR_KEY в низкий уровень (Отпускаем кнопку)
			HAL_GPIO_WritePin(PWR_KEY_GPIO_Port, PWR_KEY_Pin, GPIO_PIN_RESET);
			//4 Затем ждём высокого уровня. Это будет нарастающий фронт
			while( HAL_GPIO_ReadPin(RST_GPIO_Port, RST_Pin) != GPIO_PIN_SET );
			
			blocked = 0;//Состояние=разблокирован
			//В дальнейшем можно отправить переменную состояния с флагами, а пока строка
			uint8_t str[] = "Unblocked\0";
			HAL_UART_Transmit(&huart1, str, 9, 30);
		}
		else if (uart_data[0]==0x40)//Если пришла команда смах заставки
		{
			/*Нужно сделать несколько касаний:
			Паузы между пачками 6,5 мс. Думаю, можно сделать 6 мс.
			1.Касание в точке XY с Event Flag 00 -  Press Down (касание). Кол-во точек = 1
			2.Каcание в той же точке XY с Event Flag 10 -  Contact (удержание).
			3.N касаний с той же Y, и изменяющимся с каким-то delta шагом X.
			Сколько минимум N и delta макс пока не ясно. Просто возьмём данные из обратной
			разработки. Там N=18 (18 пачек). delta=1...100 Чаще 10,25 60.
			4.Отпускание  Event Flag 01 -  Lift Up (отпускание). Кол-во точек = 0
			5. 4 пустых пачки вида
			00 Ответ 00 00 00 FF FF FF FF FF
			08 Ответ FF FF FF FF FF FF FF FF
			10 Ответ FF FF FF FF FF FF FF FF
			A6 Ответ 01
			*/

//Пачка 1. Касание в точке XY с Event Flag 00 -  Press Down (касание). Кол-во точек = 1		
			/*Координаты начальной точки приходят по UART
				Формат такой:Команда, XHYH, XL, YL.
											[0x10, 0x12,0x1A,0xE1]
			*/

			//Извлечём из посылки координаты и упакуем их в нужном формате
			//Event_flag = 0;// Press Down
			XH = (uart_data[1]& 0xF0)>>4;
			XH = XH & 0x3F;//Пишем 00 в 7й и 6й биты ("и"   с 0b0011 1111)
			XL = uart_data[2];
			//Обнуляем биты 7-4 - Пишем 	Touch_ID = 0b0000.
			//Заодно избавляемся от XH
			YH = (uart_data[1]& 0x0F);
			YL = uart_data[3];
			
			SendPacketOnePoint(XH, XL, YH, YL);
			HAL_Delay(6);//Между пачками 6,5 мс. Решил сделать 6 мс. Работает.
				
/*Пачка №2 - каcание в той же точке XY с Event Flag 10 -  Contact (удержание). Далее
			пачки 3...3+N. N касаний с той же Y, и изменяющимся с каким-то шагом n.
			Сколько минимум N и delta макс пока не ясно. Просто возьмём данные из обратной
			разработки. Там N=18 (18 пачек). delta=1...100 Чаще 10,25 60. Примем шаг n=25, а
			N будет зависеть от шага.*/
			
			uint16_t X = (XH << 8) | XL;
			
			/*Значения YH, YL записали перед отправкой прошлой пачки.
				YH = (uart_data[1]& 0x0F) | (Touch_ID <<4);
				YL = uart_data[3];
			*/

			while(X <= 0x1E6)//Конечная координата
			{
				//1 Формируем новые координаты в нужном формате
				XH = (uint8_t)(X >> 8) ;//Выделяем старшие 8 бит
				
				//Помещаем значение Event_flag = 0b10 (Contact) в XH
			  XH = XH | 0x80;//Пишем 1 в 7й бит ("или" с 0b1000 0000)
				//В пачке 1 записали Event_flag 00, поэтому в 6м бите и так 0
				//Поэтому в 6й не пишем. XH = XH & 0xBF;//Пишем 0 в 6й бит ("и"   с 0b1011 1111)
				
				XL = (uint8_t)X;//Выделяем младшие 8 бит
				
				SendPacketOnePoint(XH, XL, YH, YL);
				HAL_Delay(6);//Между пачками 6,5 мс. Решил сделать 6 мс. Работает.		
				//Смещаем х вправо (для смаха заставки телефон так и ждёт)
				X = X + 25;
			}

//4. Пачка 3+N+1. Отпускание  Event Flag 01 -  Lift Up (отпускание). Кол-во точек = 0
			
			//1 Формируем координаты в нужном формате.
			//Выделяем байты из последней координаты цикла.
			XH = ((uint8_t)(X >> 8));//Выделяем старшие 8 бит 
			XL = (uint8_t)X;//Выделяем младшие 8 бит
						
			//Помещаем новое значение Event_flag = 0b01 -  Lift Up (отпускание) в XH
			XH = XH | 0x40;//Пишем 1 в 6й бит ("или" с 0b0100 0000) 
			XH = XH & 0x7F;//Пишем 0 в 7й бит ("и"   с 0b0111 1111)
			//YH, YL не менялись, поэтому оставляем те же.
			SendPacketOnePoint(XH, XL, YH, YL);
			HAL_Delay(6);//Задержка между пачками 6,5 мс. Решил сделать 6 мс.
			
/*
5. Пачки 3+N+2...3+N+5. 4 пустых пачки вида
			00 Ответ 00 00 00 FF FF FF FF FF
			08 Ответ FF FF FF FF FF FF FF FF
			10 Ответ FF FF FF FF FF FF FF FF
			A6 Ответ 01
		*/ 				
			for(uint8_t i = 1; i < 5; i++)
			{
				SendPacketEmpty();	
				HAL_Delay(6);//Задержка между пачками 6,5 мс. Решил сделать 6 мс.
			}
	
			//В дальнейшем можно отправить переменную состояния с флагами, а пока строка
			uint8_t str2[] = "OK\0";
			HAL_UART_Transmit(&huart1,str2, 2, 30);
		}
		
		
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  hi2c1.Init.OwnAddress1 = 112;
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
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(INT_GPIO_Port, INT_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(PWR_KEY_GPIO_Port, PWR_KEY_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : RST_Pin */
  GPIO_InitStruct.Pin = RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : VIBRO_Pin */
  GPIO_InitStruct.Pin = VIBRO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(VIBRO_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : INT_Pin */
  GPIO_InitStruct.Pin = INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PWR_KEY_Pin */
  GPIO_InitStruct.Pin = PWR_KEY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PWR_KEY_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void TouchScreenInit(void)//Ф-я для ответов процессору телефона при инициализации тача
{
	
			//Сначала процессор телефона присылает 00.
			//while(HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,300) != HAL_OK);
			//Здесь лучше без цикла?
			HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,300);
			if (i2c_data[0]==0x00)
				{
					i2c_data[0]=0x00;
					HAL_I2C_Slave_Transmit(&hi2c1,&i2c_data[0],1,30);//таймаут 30 мс
				}
			//Через 420 мс процессор телефона присылает A8. Ждём посылки 450 мс
			while(HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,450) != HAL_OK);			
			if (i2c_data[0]==0xA8)
				{
					i2c_data[0]=0x11;
					HAL_I2C_Slave_Transmit(&hi2c1,&i2c_data[0],1,30);//таймаут 30 мс
				}
			//Через 0.5 мс процессор телефона присылает A6
			while(HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,2) != HAL_OK);
			if (i2c_data[0]==0xA6)
				{
					i2c_data[0]=0x01;
					HAL_I2C_Slave_Transmit(&hi2c1,&i2c_data[0],1,30);//таймаут 30 мс
				}
			//Далее через 17,8 с процессор телефона присылает B0 00.
			while(HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,18000) != HAL_OK);
			if (i2c_data[0]==0xB0)
				{
					HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,30);//таймаут 30 мс
				}
				
				
				/*Что делать с этим, пока не ясно. Напрашиваются прерывания. 
				Т к нужно либо ждать команду от компа по Юарт, либо от телефона по i2c
				Пока оставляю так.*/
			//Далее через 46,7 с процессор телефона присылает А5 03.
			while(HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,47000) != HAL_OK);
			if (i2c_data[0]==0xA5)
				{
					HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,30);//таймаут 30 мс
				}
				//де-нибудь потом проверять, если пришла команда перевода в сон, то
				//переменная состояния blocked = 1; Т е заблокирован.
		
}

/*Ф-я для посылки телефону касания в одной точке. Посылает 4 пакета на 4 запроса
процессора телефона. Вид такой:

00 Ответ 00 00 01 xh xl yh yl 00 
08 Ответ 00 FF FF FF FF FF FF FF
10 Ответ FF FF FF FF FF FF FF FF
A6 Ответ 01

В xh содержится Event Flag (биты 7,6) и XH (биты 3-0).
В yh содержится Touch ID (биты 7-4) и YH (биты 3-0).

*/
void SendPacketOnePoint(uint8_t xh,uint8_t xl,uint8_t yh, uint8_t yl)
{
	//uint8_t event_flag = xh & 0b11000000 На двоичные числа компилятор почему-то ругается
	//Извлекаем флаг (биты 7,6) из xh. Обнуляем биты 5-0 и сдвигаем. 
	uint8_t event_flag = (xh & 0xC0)>>6;
	//1 Линию INT в низкий уровень
	HAL_GPIO_WritePin(INT_GPIO_Port, INT_Pin, GPIO_PIN_RESET);
	
	//2 На запрос 00 нужно отправить ответ 8 байт по i2c
	//(регистры 00-07, параметры 1й точки).
	while ( HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,3) != HAL_OK );
	
	if (i2c_data[0] == 0x00)
		{	
			i2c_data[0] = 0x00;//DevMode
			i2c_data[1] = 0x00;//Gest_ID
			/*Event Flag = 0b00 -  Press Down (касание)
				Event Flag = 0b01 -  Lift Up (отпускание)
				Event Flag = 0b10 -  Contact (удержание)
			*/
			if(event_flag == 1)//Event Flag = 0b01 -  Lift Up (отпускание)
			{
				i2c_data[2] = 0x00;//Кол-во точек касания
			}
			else//Event Flag = 0b00 Press Down (касание) или 0b10 Contact (удержание)
			{
				i2c_data[2] = 0x01;//Кол-во точек касания
			}
			
			i2c_data[3] = xh;//Event Flag и XH
			i2c_data[4] = xl;//XL
			i2c_data[5] = yh;//Touch ID и YH
			i2c_data[6] = yl;//YL
			i2c_data[7] = 0x00;//1st Touch Weight
			
			HAL_I2C_Slave_Transmit(&hi2c1,&i2c_data[0],8,2);//таймаут 2 мс
		}
	//3 На запрос 08 нужно отправить ответ 8 байт по i2c
	//(регистры 08-0D, параметры 2й точки).
	while ( HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,3) != HAL_OK );	
	if (i2c_data[0]==0x08)
		{	
			i2c_data[0] = 0x00;//1st Touch area
			i2c_data[1] = 0xFF;//Event Flag и XH
			i2c_data[2] = 0xFF;//XL
			i2c_data[3] = 0xFF;//2nd Touch ID и YH
			i2c_data[4] = 0xFF;//YL
			i2c_data[5] = 0xFF;//2nd Touch Weight
			i2c_data[6] = 0xFF;//2nd Touch area
			i2c_data[7] = 0xFF;//
								
			HAL_I2C_Slave_Transmit(&hi2c1,&i2c_data[0],8,2);//таймаут 2 мс
		}	
	//4 На запрос 10 нужно отправить ответ 8 байт по i2c (регистры 10-17).
	while ( HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,3) != HAL_OK );
	if (i2c_data[0]==0x10)
		{	
			i2c_data[0] = 0xFF;//
			/*В прошлом пакете в эти элементы массива уже записали FF
			i2c_data[1] = 0xFF;//
			i2c_data[2] = 0xFF;//
			i2c_data[3] = 0xFF;//
			i2c_data[4] = 0xFF;//
			i2c_data[5] = 0xFF;//
			i2c_data[6] = 0xFF;//
			i2c_data[7] = 0xFF;//
			*/
			HAL_I2C_Slave_Transmit(&hi2c1,&i2c_data[0],8,2);//таймаут 2 мс
		}		
	//5 На запрос А6 нужно отправить ответ 01 - 1 байт по i2c.
	while ( HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,3) != HAL_OK );	
	if (i2c_data[0]==0xA6)
		{	
			i2c_data[0] = 0x01;//Firmware Version 					
			HAL_I2C_Slave_Transmit(&hi2c1,&i2c_data[0],1,1);//таймаут 1 мс
		}		
		
	//6 Возвращаем линию INT в высокий уровень
	HAL_GPIO_WritePin(INT_GPIO_Port, INT_Pin, GPIO_PIN_SET);	
}
/*Ф-я для посылки пустых пачек вида
			00 Ответ 00 00 00 FF FF FF FF FF
			08 Ответ FF FF FF FF FF FF FF FF
			10 Ответ FF FF FF FF FF FF FF FF
			A6 Ответ 01
*/
void SendPacketEmpty(void)
{
	//1 Линию INT в низкий уровень
	HAL_GPIO_WritePin(INT_GPIO_Port, INT_Pin, GPIO_PIN_RESET);

	// Делаем касание
	//2 На запрос 00 нужно отправить ответ 8 байт по i2c (регистры 00-07, параметры 1й точки).	
	while ( HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,3) != HAL_OK );
	if (i2c_data[0]==0x00)
		{	
			i2c_data[0] = 0x00;//DevMode
			i2c_data[1] = 0x00;//Gest_ID
			i2c_data[2] = 0x00;//Кол-во точек касания
			i2c_data[3] = 0xFF;//Event Flag и XH
			i2c_data[4] = 0xFF;//XL
			i2c_data[5] = 0xFF;//Touch ID и YH
			i2c_data[6] = 0xFF;//YL
			i2c_data[7] = 0xFF;//1st Touch Weight
			
			HAL_I2C_Slave_Transmit(&hi2c1,&i2c_data[0],8,2);//таймаут 2 мс
		}
	//3 На запрос 08 нужно отправить ответ 8 байт по i2c
	//(регистры 08-0D, параметры 2й точки).
	while ( HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,3) != HAL_OK );	
	if (i2c_data[0]==0x08)
		{	
			i2c_data[0] = 0xFF;//1st Touch area
			i2c_data[1] = 0xFF;//Event Flag и XH
			i2c_data[2] = 0xFF;//XL
			/*Эти элементы массива и так уже FF
			i2c_data[3] = 0xFF;//2nd Touch ID и YH
			i2c_data[4] = 0xFF;//YL
			i2c_data[5] = 0xFF;//2nd Touch Weight
			i2c_data[6] = 0xFF;//2nd Touch area
			i2c_data[7] = 0xFF;*/
								
			HAL_I2C_Slave_Transmit(&hi2c1,&i2c_data[0],8,2);//таймаут 2 мс
		}	
	//4 На запрос 10 нужно отправить ответ 8 байт по i2c (регистры 10-17).
	while ( HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,3) != HAL_OK );
	if (i2c_data[0]==0x10)
		{	
			i2c_data[0] = 0xFF;//
			/*Эти элементы массива и так уже FF
			i2c_data[1] = 0xFF;//
			i2c_data[2] = 0xFF;//
			i2c_data[3] = 0xFF;//
			i2c_data[4] = 0xFF;//
			i2c_data[5] = 0xFF;//
			i2c_data[6] = 0xFF;//
			i2c_data[7] = 0xFF;*/
			
			HAL_I2C_Slave_Transmit(&hi2c1,&i2c_data[0],8,2);//таймаут 2 мс
		}		
	//5 На запрос А6 нужно отправить ответ 01 - 1 байт по i2c.
	while ( HAL_I2C_Slave_Receive(&hi2c1,&i2c_data[0],1,3) != HAL_OK );	
	if (i2c_data[0]==0xA6)
		{	
			i2c_data[0] = 0x01;//Firmware Version 					
			HAL_I2C_Slave_Transmit(&hi2c1,&i2c_data[0],1,1);//таймаут 1 мс
		}			
	//6 Возвращаем линию INT в высокий уровень
	HAL_GPIO_WritePin(INT_GPIO_Port, INT_Pin, GPIO_PIN_SET);
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
