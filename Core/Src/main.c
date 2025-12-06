/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>

#include "io_matrix.h"
#include "midi_device.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef struct
{
	bool tgl[2];
} Key_State;

typedef struct
{
	bool pressed;
	bool pendingRelease;
	uint32_t firstTouchTick; // 10kHz ticks
	uint8_t velocity;
} Note_State;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define DEFAULT_ROOT_NOTE 36

// Intervals measured in 10 kHz ticks
#define MIN_VELOCITY_INTERVAL 30
#define MAX_VELOCITY_INTERVAL 1200
#define VELOCITY_INTERVAL_RANGE (MAX_VELOCITY_INTERVAL-MIN_VELOCITY_INTERVAL)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim11;

/* USER CODE BEGIN PV */

Key_State group[INPUT_PIN_COUNT];
Note_State keys[KEY_COUNT];

uint16_t rootNote = DEFAULT_ROOT_NOTE;

uint8_t pressedNotesCount = 0;
bool sustainFlag;

bool octaveUpState, octaveDownState;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM11_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void resetKey(uint8_t k)
{
	keys[k].pressed = false;
	keys[k].firstTouchTick = 0;
}

void releaseKey(uint8_t k)
{
	uint16_t note = rootNote + k;
	if (note >= 120)
		note -= 120;

	MIDI_addEventToBuffer(MIDI_MESSAGE_NOTE_OFF, note, 0);
	keys[k].pendingRelease = false;
	keys[k].firstTouchTick = 0;
}

void releaseAllNotes()
{
	for (uint8_t k = 0; k < KEY_COUNT; ++k)
	{
		if (keys[k].pendingRelease)
		{
			releaseKey(k);
		}
	}
}

void pressKey(uint8_t k)
{
	if (sustainFlag && pressedNotesCount == 0)
		releaseAllNotes();

	if(! keys[k].pendingRelease)
	{
		uint16_t note = rootNote + k;
		if (note >= 120)
			note -= 120;

		MIDI_addEventToBuffer(MIDI_MESSAGE_NOTE_ON, note, keys[k].velocity);
		keys[k].pressed = true;
		keys[k].pendingRelease = true;
	}
	++pressedNotesCount;
}

void transpose(bool up)
{
	int8_t offset;
	if (up)
		offset = +12;
	else
		offset = -12;

	for (uint8_t k = 0; k < KEY_COUNT; ++k)
	{
		if (keys[k].pendingRelease)
		{
			releaseKey(k);

			int16_t note = rootNote + k + offset;
			if (note < 0)
				note += 120;
			else if (note >= 120)
				note -= 120;

			MIDI_addEventToBuffer(MIDI_MESSAGE_NOTE_ON, note, keys[k].velocity);
			keys[k].pendingRelease = true;
		}
	}

	if (up && rootNote == 120)
		rootNote = 0;
	else if (! up && rootNote == 0)
		rootNote = 108;
	else
		rootNote += offset;
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
  MX_USB_DEVICE_Init();
  MX_TIM11_Init();
  /* USER CODE BEGIN 2 */

  uint8_t firstKey, key;
  uint32_t velocityInterval, velocityTimerTick, velocityOffset;

  sustainFlag = ! HAL_GPIO_ReadPin(sustainToggle.port, sustainToggle.pin);
  bool newSustainFlag, newOctaveUpState, newOctaveDownState;
  bool panicMessageSent;

//  for (uint8_t i = 0; i < INPUT_PIN_COUNT; ++i)
//  {
//	  group[i].tgl[0] = false;
//	  group[i].tgl[1] = false;
//  }
//
//for (uint8_t k = 0; k < KEY_COUNT; ++k)
//{
//	keys[k].pressed = false;
//	keys[k].firstTouchTick = 0;
//}

	HAL_TIM_Base_Start(&htim11);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
	  newSustainFlag = ! HAL_GPIO_ReadPin(sustainToggle.port, sustainToggle.pin);

	  if (sustainFlag && ! newSustainFlag)
	  {
		  releaseAllNotes();
	  }

	  sustainFlag = newSustainFlag;

	  newOctaveUpState = ! HAL_GPIO_ReadPin(octaveUpButton.port, octaveUpButton.pin);
	  newOctaveDownState = ! HAL_GPIO_ReadPin(octaveDownButton.port, octaveDownButton.pin);

	  if (! panicMessageSent)
	  {
		  if ((octaveUpState || octaveDownState) && (newOctaveUpState && newOctaveDownState))
		  {
			  // MIDI Panic
			  releaseAllNotes();
			  MIDI_addEventToBuffer(MIDI_MESSAGE_CONTROL_CHANGE, MIDI_MESSAGE_CONTROL_ALL_NOTES_OFF, 0);
			  MIDI_flush();
			  panicMessageSent = true;
		  }
		  else
		  {
			  if (octaveUpState && ! newOctaveUpState)
			  {
				  transpose(true);
			  }
			  else if (octaveDownState && ! newOctaveDownState)
			  {
				  transpose(false);
			  }
		  }
	  }
	  else if (! newOctaveUpState && ! newOctaveDownState)
	  {
		  panicMessageSent = false;
	  }

	  octaveUpState = newOctaveUpState;
	  octaveDownState = newOctaveDownState;

	  uint8_t keyCountForGroup = INPUT_PIN_COUNT;

	  for (uint8_t o = 0; o < OUTPUT_PIN_COUNT; o += 2)
	  {
		  if (o >= 12) // special condition for last 2 groups
		  {
			  keyCountForGroup = 1;
		  }

		  for (uint8_t t = 0; t < 2; ++t)
		  {
			  uint8_t outPin = o + t;

			  HAL_GPIO_WritePin(outputPins[outPin].port, outputPins[outPin].pin, GPIO_PIN_RESET);

			  for (uint8_t i = 0; i < keyCountForGroup; ++i)
			  {
				  group[i].tgl[t] = ! HAL_GPIO_ReadPin(inputPins[i].port, inputPins[i].pin);
			  }

			  HAL_GPIO_WritePin(outputPins[outPin].port, outputPins[outPin].pin, GPIO_PIN_SET);
		  }

		  firstKey = INPUT_PIN_COUNT * (o / 2);

		  for (uint8_t k = 0; k < keyCountForGroup; ++k)
		  {
			  key = firstKey + k;

			  if (! keys[key].pressed)
			  {
				  if ((keys[key].firstTouchTick == 0) && (group[k].tgl[0] || group[k].tgl[1]))
				  {
					  keys[key].firstTouchTick = __HAL_TIM_GET_COUNTER(&htim11);
				  }

				  if (keys[key].firstTouchTick > 0)
				  {
					  velocityTimerTick = __HAL_TIM_GET_COUNTER(&htim11);

					  if (keys[key].firstTouchTick <= velocityTimerTick)
					  {
						  velocityInterval = velocityTimerTick - keys[key].firstTouchTick;
					  }
					  else
					  {
						  // Dealing with timer period overflow
						  velocityInterval = velocityTimerTick + (htim11.Init.Period - keys[key].firstTouchTick);
					  }

					  if (velocityInterval > MAX_VELOCITY_INTERVAL)
					  {
						  keys[key].velocity = 25;
						  if (group[k].tgl[0] || group[k].tgl[1])
						  {
							  pressKey(key);
						  }
						  else
						  {
							  resetKey(key);
						  }
					  }
					  else if (group[k].tgl[0] && group[k].tgl[1])
					  {
						  if (velocityInterval < MIN_VELOCITY_INTERVAL)
						  {
							  keys[key].velocity = 0xFF;
						  }
						  else
						  {
							  velocityOffset = (float)(velocityInterval - MIN_VELOCITY_INTERVAL);
							  keys[key].velocity = 0xFF - (uint8_t)(100.f * velocityOffset / (float)VELOCITY_INTERVAL_RANGE);
						  }

						  pressKey(key);
					  }

				  }
			  }
			  else if (! group[k].tgl[0] && ! group[k].tgl[1])
			  {
				  if (! sustainFlag && keys[key].pendingRelease)
				  {
					  releaseKey(key);
				  }

				  resetKey(key);
				  --pressedNotesCount;
			  }
		  }

//		  HAL_Delay(1);
	  }
	  MIDI_flush();

  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 5;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM11 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM11_Init(void)
{

  /* USER CODE BEGIN TIM11_Init 0 */

  /* USER CODE END TIM11_Init 0 */

  /* USER CODE BEGIN TIM11_Init 1 */

  /* USER CODE END TIM11_Init 1 */
  htim11.Instance = TIM11;
  htim11.Init.Prescaler = 1600;
  htim11.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim11.Init.Period = 65535;
  htim11.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim11.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim11) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM11_Init 2 */

  /* USER CODE END TIM11_Init 2 */

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
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_3|GPIO_PIN_15, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_3|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8
                          |GPIO_PIN_9, GPIO_PIN_SET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC14 PC15 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA3 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_3|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA4 PA5 PA6 PA7
                           PA8 PA9 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB10 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB13 PB14 PB3 PB4
                           PB5 PB6 PB7 PB8
                           PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_3|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8
                          |GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
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
