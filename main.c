/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "W25Qxx.h"
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include <stdbool.h>
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
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;

osThreadId UART_ThreadHandle;
osThreadId FlashStore_ThreadHandle;
osThreadId endTransmission_ThreadHandle;

osSemaphoreId UART_semaphoreHandle;
osSemaphoreId FlashStore_semaphoreHandle;
osSemaphoreId StoreCplt_semaphoreHandle;
osSemaphoreId endTransmission_semaphoreHandle;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
void UART_Task(void const * argument);
void FlashStore_Task(void const * argument);
void endTransmission_Task(void const * argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define Buffer_size 100
#define MAX_TX 100
#define accuracy MAX_TX
#define Flash_Page_Base 1400
#define MAX_WRITE_DATA 10000
#define Delimiter "**********\r\n**********"

uint8_t FS1[Buffer_size+1] = {0};
uint8_t FS2[Buffer_size+1] = {0};

uint8_t *Header = FS1;
uint8_t *Secondary_Header = FS2;

bool isSizeRxed = 0;
uint32_t size = 0;

uint32_t start_time = 0, end_time = 0, time_diff = 0;
float speed[accuracy] = {0};
int speed_idx = 0;

int flash_page_index = 0;

uint8_t size_TX[MAX_TX] = {0};
uint8_t size_TX_indx = 0;

void switch_header()
{
	if (Header == FS1){
		Header = FS2;
		Secondary_Header = FS1;
	}else {
		Header = FS1;
		Secondary_Header = FS2;
	}
}

void calculate_baud()		//for each transmit or receive
{
	end_time = HAL_GetTick();
	time_diff = end_time - start_time;
	if (time_diff > 0 && speed_idx < accuracy)
	{
		speed[speed_idx] = (float) (size * 10 * 1000) / time_diff;
		speed_idx++;
	}
	else if (speed_idx >= accuracy)
		speed_idx = 0;
}

float average_baud()
{
	float Final_baud_rate = 0;
	int terms = 0;
	for(int i=0; i < accuracy; i++)
	{
		if (speed[i] == 0)
			continue;
		Final_baud_rate = Final_baud_rate + speed[i];
		terms++;
	}

	Final_baud_rate = Final_baud_rate / terms;
	return Final_baud_rate;
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	//wake uart_thread
	osSemaphoreRelease(UART_semaphoreHandle);
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
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  W25Q_Reset();
  W25Q_Erase_Sectors(Flash_Page_Base, 0, MAX_WRITE_DATA);
  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of UART_semaphore */
  osSemaphoreDef(UART_semaphore);
  UART_semaphoreHandle = osSemaphoreCreate(osSemaphore(UART_semaphore), 1);

  osSemaphoreDef(FlashStore_semaphore);
  FlashStore_semaphoreHandle = osSemaphoreCreate(osSemaphore(FlashStore_semaphore), 1);

  osSemaphoreDef(StoreCplt_semaphore);
  StoreCplt_semaphoreHandle = osSemaphoreCreate(osSemaphore(StoreCplt_semaphore), 1);

  osSemaphoreDef(endTransmission_semaphore);
  endTransmission_semaphoreHandle = osSemaphoreCreate(osSemaphore(endTransmission_semaphore), 1);


  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of UART_Thread */
  osThreadDef(UART_Thread, UART_Task, osPriorityNormal, 0, 128);
  UART_ThreadHandle = osThreadCreate(osThread(UART_Thread), NULL);

  osThreadDef(FlashStore_Thread, FlashStore_Task, osPriorityNormal, 0,128);
  FlashStore_ThreadHandle = osThreadCreate(osThread(FlashStore_Thread), NULL);

  osThreadDef(endTransmission_Thread, endTransmission_Task, osPriorityNormal, 0, 128);
  endTransmission_ThreadHandle = osThreadCreate(osThread(endTransmission_Thread), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osSemaphoreRelease(StoreCplt_semaphoreHandle);
  osSemaphoreWait(FlashStore_semaphoreHandle, osWaitForever);
  osSemaphoreWait(UART_semaphoreHandle, osWaitForever);
  osSemaphoreWait(endTransmission_semaphoreHandle, osWaitForever);
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  huart2.Init.BaudRate = 2400;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
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
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

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
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /**/
  __HAL_SYSCFG_FASTMODEPLUS_ENABLE(SYSCFG_FASTMODEPLUS_PB6);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_UART_Task */
/**
  * @brief  Function implementing the UART_Thread thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_UART_Task */
void UART_Task(void const * argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  HAL_UART_Receive_DMA(&huart2, Header, 4);
  for(;;)
  {

	if(osSemaphoreWait(UART_semaphoreHandle, osWaitForever) == osOK)
	{
		if(isSizeRxed == 0)
		{
			osSemaphoreWait(StoreCplt_semaphoreHandle, osWaitForever);
			uint8_t temp_size[] = {Header[0], Header[1], Header[2], Header[3]};
			char *endptr = 0;
			size = strtol((char *)temp_size, &endptr, 10);//((Header[0]-48)*1000) + ((Header[1]-48)*100) + ((Header[2]-48)*10) + ((Header[3]-48));
			if (size == 0 || *endptr != '\0')
			{
				//osSemaphoreRelease(FlashStore_semaphoreHandle);
				//osSemaphoreWait(StoreCplt_semaphoreHandle, osWaitForever);

				//osThreadTerminate(FlashStore_ThreadHandle);
				osSemaphoreRelease(endTransmission_semaphoreHandle);
				//osThreadTerminate(UART_ThreadHandle);
			}
			isSizeRxed = 1;
			HAL_UART_Receive_DMA(&huart2, Header, size);
			start_time = HAL_GetTick();
		}
		else if(isSizeRxed == 1)
		{
			calculate_baud();
			size_TX[size_TX_indx] = size;
			size_TX_indx++;
			isSizeRxed = 0;
			size = 0;
			HAL_UART_Receive_DMA(&huart2, Secondary_Header, 4);
			osSemaphoreRelease(FlashStore_semaphoreHandle);
		}
	}
  }
  /* USER CODE END 5 */
}

void FlashStore_Task(void const * argument)
{
	for(;;)
	{
		if(osSemaphoreWait(FlashStore_semaphoreHandle, osWaitForever) == osOK)
		{
			uint32_t ID = W25Q_ReadID();
			if (ID != 0xef6017)		return ;		//flash not recognized
			if(strlen((char*)Header) > 0)
			{
				W25Q_Write_Page(Flash_Page_Base + flash_page_index, 0, strlen((char*)Header), Header);
				flash_page_index ++;
				memset(Header, 0, strlen((char *)Header));
			}
			switch_header();
			osSemaphoreRelease(StoreCplt_semaphoreHandle);
		}
	}
}

void endTransmission_Task(void const * argument)
{
	for(;;)
	{
		if(osSemaphoreWait(endTransmission_semaphoreHandle, osWaitForever) == osOK)
		{
			float rxBaud = average_baud();
			uint8_t rData[50] = {0};
			sprintf((char *)rData, "MCU : Receive Baud rate is %f\n",rxBaud);
			HAL_UART_Transmit(&huart2, rData, strlen((char *)rData), 5000);
			for(int i = 0; i < flash_page_index; i++)
			{
				memset(Header, 0, strlen((char *)Header));
				W25Q_Read(Flash_Page_Base + i, 0, Buffer_size, Header);
				HAL_UART_Transmit(&huart2, Header, strlen((char *)Header), 5000);
			}
			memset(rData, 0, strlen((char *)rData));
			sprintf((char *)rData, Delimiter);
			HAL_UART_Transmit(&huart2, rData, strlen((char *)rData), 5000);
			//osThreadTerminate(endTransmission_ThreadHandle);
			vTaskDelay(5000);
			NVIC_SystemReset();
		}
	}
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
