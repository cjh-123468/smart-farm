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
#include "stdio.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DHT11_1_PORT GPIOA
#define DHT11_1_PIN  GPIO_PIN_1   // A1

#define DHT11_2_PORT GPIOA
#define DHT11_2_PIN  GPIO_PIN_4   // A2

#define DHT11_3_PORT GPIOB
#define DHT11_3_PIN  GPIO_PIN_0   // A3

#define DHT11_4_PORT GPIOC
#define DHT11_4_PIN  GPIO_PIN_1   // A4
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
uint8_t Rh_byte1, Rh_byte2, Temp_byte1, Temp_byte2;
uint16_t SUM;
uint8_t Presence = 0;
uint8_t TEMP = 0;
uint8_t RH = 0;
float Temperature = 0;
float Humidity = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;

    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

void DHT11_Start(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    Set_Pin_Output(GPIOx, GPIO_Pin);

    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, 0);
    delay_us(18000);

    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, 1);
    delay_us(20);

    Set_Pin_Input(GPIOx, GPIO_Pin);
}

uint8_t DHT11_Check_Response(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    uint8_t Response = 0;

    delay_us(40);

    if (!(HAL_GPIO_ReadPin(GPIOx, GPIO_Pin)))
    {
        delay_us(80);

        if ((HAL_GPIO_ReadPin(GPIOx, GPIO_Pin)))
            Response = 1;
        else
            Response = 0;
    }

    while ((HAL_GPIO_ReadPin(GPIOx, GPIO_Pin)));

    return Response;
}

uint8_t DHT11_Read(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    uint8_t i = 0, j;

    for (j = 0; j < 8; j++)
    {
        while (!(HAL_GPIO_ReadPin(GPIOx, GPIO_Pin)));

        delay_us(40);

        if (!(HAL_GPIO_ReadPin(GPIOx, GPIO_Pin)))
        {
            i &= ~(1 << (7 - j));
        }
        else
        {
            i |= (1 << (7 - j));
        }

        while ((HAL_GPIO_ReadPin(GPIOx, GPIO_Pin)));
    }

    return i;
}

uint8_t Read_DHT11(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t *temp, uint8_t *humid)
{
    uint8_t rh1, rh2, temp1, temp2, sum;
    uint8_t presence;

    DHT11_Start(GPIOx, GPIO_Pin);

    presence = DHT11_Check_Response(GPIOx, GPIO_Pin);

    if (presence == 1)
    {
        rh1 = DHT11_Read(GPIOx, GPIO_Pin);
        rh2 = DHT11_Read(GPIOx, GPIO_Pin);
        temp1 = DHT11_Read(GPIOx, GPIO_Pin);
        temp2 = DHT11_Read(GPIOx, GPIO_Pin);
        sum = DHT11_Read(GPIOx, GPIO_Pin);

        if (sum == (uint8_t)(rh1 + rh2 + temp1 + temp2))
        {
            *humid = rh1;
            *temp = temp1;
            return 1;
        }
    }

    return 0;
}

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
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim2);
  HAL_Delay(2000);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */


    /* USER CODE BEGIN 3 */

    uint8_t temp1 = 0, humid1 = 0;
    uint8_t temp2 = 0, humid2 = 0;
    uint8_t temp3 = 0, humid3 = 0;
    uint8_t temp4 = 0, humid4 = 0;

    uint8_t ok1 = 0, ok2 = 0, ok3 = 0, ok4 = 0;

    uint8_t sensorstr[200];

    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

    ok1 = Read_DHT11(DHT11_1_PORT, DHT11_1_PIN, &temp1, &humid1);
    HAL_Delay(1000);

    ok2 = Read_DHT11(DHT11_2_PORT, DHT11_2_PIN, &temp2, &humid2);
    HAL_Delay(1000);

    ok3 = Read_DHT11(DHT11_3_PORT, DHT11_3_PIN, &temp3, &humid3);
    HAL_Delay(1000);

    ok4 = Read_DHT11(DHT11_4_PORT, DHT11_4_PIN, &temp4, &humid4);
    HAL_Delay(1000);

    // ===== CM1107N CO2 센서 읽기 =====
    /* USER CODE BEGIN 3 */
    uint8_t co2_cmd[4] = {0x11, 0x01, 0x01, 0xED};
    uint8_t co2_rx[8] = {0};
    uint16_t co2_value = 9999;

    HAL_UART_Transmit(&huart3, co2_cmd, 4, 100);

    if (HAL_UART_Receive(&huart3, co2_rx, 8, 1000) == HAL_OK)
    {
        co2_value = ((uint16_t)co2_rx[3] << 8) | co2_rx[4];
    }
    /* USER CODE END 3 */

    sprintf((char*)sensorstr,
            "%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
            ok1 ? temp1 : 255,
            ok1 ? humid1 : 255,
            ok2 ? temp2 : 255,
            ok2 ? humid2 : 255,
            ok3 ? temp3 : 255,
            ok3 ? humid3 : 255,
            ok4 ? temp4 : 255,
            ok4 ? humid4 : 255,
            co2_value);

    HAL_UART_Transmit(&huart2,
                      sensorstr,
                      strlen((char*)sensorstr),
                      100);

    // 센서 4개 딜레이(4x1000ms) + 읽기/CO2 처리 시간(~0.1~1s)을 합쳐
    // 한 사이클이 총 약 15초가 되도록 마지막 딜레이를 늘림
    // (1학기 때 학습한 LSTM 모델이 15초 간격 데이터를 기준으로 만들어졌기 때문)
    HAL_Delay(10000);
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
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

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 64-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
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
  huart2.Init.BaudRate = 9600;
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
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DHT11_4_GPIO_Port, DHT11_4_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, DHT11_1_Pin|DHT11_2_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DHT11_3_GPIO_Port, DHT11_3_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DHT11_4_Pin */
  GPIO_InitStruct.Pin = DHT11_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_4_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DHT11_1_Pin DHT11_2_Pin LD2_Pin */
  GPIO_InitStruct.Pin = DHT11_1_Pin|DHT11_2_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : DHT11_3_Pin */
  GPIO_InitStruct.Pin = DHT11_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB11 */
  /*Configure GPIO pin : PB11 */
  /*Configure GPIO pin : PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/


  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
