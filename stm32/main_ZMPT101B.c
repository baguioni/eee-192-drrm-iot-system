/* USER CODE BEGIN Header */
/**
  ************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "usart.h"
#include "message_queue.h"
#include <stdint.h>
#include <stdlib.h>
#include "json.h"


/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
#define SENSORTYPE VOLTAGE
#define SENSORDELAY 500

UART_HandleTypeDef huart2;


uint16_t volt=0;
char msg[20];
int ADCVal[100]; //array to store ADC values
unsigned int max_v=0;
int VmaxD;
int VeffD;
int Veff;
int Vout;
int Vrms;
double Output[20];
double Overvoltage =242; //+20% of RMS voltage
uint16_t dequeued_value;

void SystemClock_Config(void);
int main(void)
{
	usart2_init();
  HAL_Init();
  SystemClock_Config();
  MX_ADC1_Init();



  //Initialize a queue
	Queue* queue = createQueue();

  while (1)
  {
    /* USER CODE END WHILE */
	 for (int j =0; j<50; j++){
	 		  HAL_ADC_Start(&hadc1);
	 		  	  HAL_ADC_PollForConversion(&hadc1, 20);

	 		  	  //Based from Electropeak Arduino Code!
	 		  	  volt= HAL_ADC_GetValue(&hadc1);
	 		  		for (int i = 0; i < 100; i++){
	 		  			if (volt > 1200){
	 		  				ADCVal[i]=volt;
	 		  			}
	 		  			else{
	 		  				ADCVal[i]=0;
	 		  			}
	 		  			//put delay here for stability
	 		  			HAL_Delay(1);
	 		  		}
	 		  		//Find max value in the array
	 		  		max_v = 0;
	 		  		for (int i = 0; i< 100; i++){
	 		  			if (ADCVal[i]>max_v){
	 		  				max_v = ADCVal[i];
	 		  			}
	 		  			ADCVal[i]=0;

	 		  		}
	 		  		//Calculate actual voltage
	 		  		if (max_v !=0){
	 		  			VmaxD = max_v; //Set VmaxD to maximum sensor value
	 		  			VeffD = VmaxD/sqrt(2); //RMS voltage
	 		  			//Veff = (((VeffD - 420.76) / -90.24) * -210.2) + 210.2;  // Apply calibration and scaling to Veff (adc-actual)
	 		  			Veff = (0.2342*VeffD)-231.36;
	 		  		}
	 		  		else{
	 		  			Veff = 0;
	 		  		}
	 		  		Output[j] = Veff;
	 	  }
	 		Vrms = 0;
	 		for (int i = 0; i<50 ; i++){
	 			if (Output[i]>Vrms){
	 				Vrms = Output[i];
	 			}
	 			Output[i]=0;
	 		}

	 		dequeued_value = Vrms;

      bool wifiModuleConnected = getWifiStatus();

      if (wifiModuleConnected && !isEmpty(queue)) {
        // Remove element in queue since it was already sent
        dequeue(queue, &dequeued_value);
      }

	 		// Message Queue related functionality
	 		enqueue(queue, Vrms);

	 		// Send to serial monitor
	 	    if (huart2.gState != HAL_UART_STATE_BUSY_TX && !isEmpty(queue)) {
	 	    	  int result = peek(queue);
	 		    	char* jsonDatapoint = createJSONString(result, VOLTAGE);
	 		    	//send to monitor
	 		    	usart6_tx_send(jsonDatapoint, strlen(jsonDatapoint));
	 		    	//send to esp32
			    	//HAL_UART_Transmit (&huart2, jsonDatapoint, strlen(jsonDatapoint), HAL_MAX_DELAY);
	 				  usart2_tx_send(jsonDatapoint, strlen(jsonDatapoint));
	 			    // Free dynamically allocated memory
	 			    free(jsonDatapoint);
	 	    }
	 	    HAL_Delay(SENSORDELAY);
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2; //DIV2 USART CLOCK is 8MHz USART2
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2; //DIV2 USART CLOCK is 16Mhz USART6

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */


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
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


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
