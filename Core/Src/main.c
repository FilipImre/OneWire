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
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "OneWire.h"
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
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  OneWireSetup(GPIOB, GPIO_PIN_10, 10);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  char msg[256];
  // ================================================[ TEST READ_ROM]===================================================================================
  /*
   * This is the fastest and most efficient method to retrieve 8 byte device UID for one device
   * This variant can be used only when one device is present on the One-Wire bus
   * ----[ USAGE ]----
   * Just call 'OneWire_FindAllDevices();'
   * The device UID is stored in the 0-th device UID descriptor OneWireUIDs[0] by accessing its
   * 8 bytes: OneWireUIDs[0].address[0], ... , OneWireUIDs[0].address[7]
   * */

  OneWire_Read_UID();
  sprintf(msg, "Address = %02X %02X %02X %02X %02X %02X %02X %02X\n", OneWireUIDs[0].address[0], OneWireUIDs[0].address[1], OneWireUIDs[0].address[2], OneWireUIDs[0].address[3], OneWireUIDs[0].address[4], OneWireUIDs[0].address[5], OneWireUIDs[0].address[6], OneWireUIDs[0].address[7]);
  HAL_UART_Transmit(&huart1, msg, strlen(msg), 3000);

  // ================================================[ TEST SEARCH_ROM]===================================================================================
  /*
   * When >1 One-Wire devices are present on the bus the READ-ROM command causes communication conflicts on the bus
   * In this case it must the SEARCH-ROM be used to discover devices bit-by-bit using Dallas Semiconductor's original
   * algorithm adapted to the functions of the library. This function is limited to find 64 devices at maximum
   * ----[ USAGE ]----
   * Call 'OneWire_FindAllDevices();'
   * The number of devices present on the bus can be found in 'uint8_t OneWireDevsNo'
   * The UIDs are stored in 'OneWireUIDs[OneWireDevsNo]'
   * */

  OneWire_FindAllDevices();  // Use this when there are >1 devices on the bus (this function is significantly slower but can find all devices)

  sprintf(msg, "#devices %d\n", OneWireDevsNo);
  HAL_UART_Transmit(&huart1, msg, strlen(msg), 3000);

  for(uint8_t i=0; i<OneWireDevsNo; i++)
  {
	  sprintf(msg, "Address = %02X %02X %02X %02X %02X %02X %02X %02X\n", OneWireUIDs[i].address[0], OneWireUIDs[i].address[1], OneWireUIDs[i].address[2], OneWireUIDs[i].address[3], OneWireUIDs[i].address[4], OneWireUIDs[i].address[5], OneWireUIDs[i].address[6], OneWireUIDs[i].address[7]);
	  HAL_UART_Transmit(&huart1, msg, strlen(msg), 3000);
  }
  // ================================================[ EXAMPLES IN WHILE LOOP]===================================================================================

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	// =======================================[ TEST SKIP-ROM DS18B20 MEASURE TEMPERATURE ]=================================================
	  /*
	   * This is the most commonly used functionality of DS18B20
	   * !!! NOTE !!!
	   * Not required all 5 bytes to be read, the first 2 bytes are sufficient:
	   * first byte: LSB
	   * second byte: MSB
	   * calculation formula: Tmperature = ((MSB << 8) | LSB) / 16
	   * */
	  // Request measure temperature
	  OneWire_Init();
	  OneWire_WriteByte(ROM_SKIP);  // Skip ROM         (ROM-CMD)
	  OneWire_WriteByte(0x44);  // Measure Temp

	  /* Begin of super important atomic code, interrupt or whatever that can't wait low priority OneWire tasks to complete.
	   * DISABLE_ONEWIRE() is the safest mode of pausing OneWire read/write operations without interfering bit timings.
	   * This mechanism is based on regrouping the OneWire code into atomic operation blocks that are pausing interrupt callbacks while
	   * the respective atomic block operations (writing voltage levels) are not finished. For every event that has been occurred during
	   * the execution of the atomic block is flagged and will be fired it's callback function right after atomic block execution is finished.
	   * All that DISABLE_ONEWIRE() does is signaling for each atomic block that the execution of the next atomic block is not allowed 
	   */
	  DISABLE_ONEWIRE();
	  // Your_Super_Important_Instruction_1;
	  // Your_Super_Important_Instruction_2;
	  // ...
	  // // Your_Super_Important_Last_Instruction();
	  ENABLE_ONEWIRE();
	  /* ENABLE_ONEWIRE() signals each atmoic OneWire execution block that the execution of the next block is allowed.
	   * By using this mechanism you don't have to worry about stochastic OneWire execution timings, nor corrupted bytes
	   * transmitted over the bus, even when large number of OneWire operations are required, for example calling 'OneWire_FindAllDevices()' 
	   * */

	  // Wait for temperature measurement
	  HAL_Delay(700);

	  // Request read bytes
	  OneWire_Init();
	  OneWire_WriteByte(ROM_SKIP);  // Skip ROM         (ROM-CMD)
	  OneWire_WriteByte(0xBE);  // Read Scratchpad  (F-CMD)

	  // Reading bytes from DS18B20
	  uint8_t Recvd[5] = {0, 0, 0, 0, 0};
	  for(uint8_t i=0; i<5; i++)
	  {
		  Recvd[i] = OneWire_ReadByte();
	  }

	  // Calculate Temperature
	  volatile uint16_t Temperature = (Recvd[1] << 8) | Recvd[0];
	  Temperature = Temperature / 16;

	  sprintf(msg, "byte0: %u\nbyte1: %u\nbyte2: %u\nbyte3: %u\nbyte4: %u\nTemperature: %d\n\n", Recvd[0], Recvd[1], Recvd[2], Recvd[3], Recvd[4], Temperature);
	  HAL_UART_Transmit(&huart1, msg, strlen(msg), 3000);

	// =======================================[ TEST MATCH-ROM WITH DS18B20 ]==========================================================
	  /*
	   * -------USAGE-------
	   * Initialize the BUS and request MATCH-ROM
	   * Write all 8 bytes, starting from the family-code byte
	   * The rest is the same as in case of SKIP-ROM example
	   * */
	  HAL_Delay(100);
	  OneWire_Init();
	  OneWire_WriteByte(ROM_MATCH); // Request MATCH_ROM

	  const uint8_t Adr[8] = {0x28, 0x0C, 0xBD, 0x01, 0x00, 0x00, 0x00, 0x4F}; // Address to be matched
	  for(uint8_t i=0; i<8; i++) // Send address bytes byte-by-byte
	  {
		  OneWire_WriteByte(Adr[i]);
	  }
	  OneWire_WriteByte(0xBE); // Read scratchpad, the TH byte_2 cnnot be >0x80
	  OneWire_ReadByte(); OneWire_ReadByte(); // Ignore the first 2 bytes

	  if(OneWire_ReadByte() < 0x80) // Valid response => MATCH-ROM succeeded
		  sprintf(msg, "Device %02X %02X %02X %02X %02X %02X %02X %02X matched successful\n", Adr[0],  Adr[1], Adr[2], Adr[3], Adr[4], Adr[5], Adr[6], Adr[7]);
	  else
		  sprintf(msg, "Address %02X %02X %02X %02X %02X %02X %02X %02X failed to match with the device\n", Adr[0],  Adr[1], Adr[2], Adr[3], Adr[4], Adr[5], Adr[6], Adr[7]);

	  HAL_UART_Transmit(&huart1, msg, strlen(msg), 3000);
	// =====================================================================================================================================

	  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // Heart-beat signal of the stm32
	  HAL_Delay(500);
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
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV2;
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
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1|GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
