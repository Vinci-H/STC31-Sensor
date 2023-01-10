/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>  // printf

#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"
#include "stc3x_i2c.h"
#include "sht4x.h"

#include <inttypes.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
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
  MX_USART2_UART_Init();
  //MX_I2C2_Init();
  /* USER CODE BEGIN 2 */

    printf("STC31 & SHT40 test\n");
    HAL_Delay(100);
    printf("STC31 & SHT40 test\n");
    HAL_Delay(100);
    printf("STC31 & SHT40 test\n");
    HAL_Delay(100);


    int16_t error = 0;

    sensirion_i2c_hal_init(); //I2C2

    /*************************************************************************/
    while (sht4x_probe() != STATUS_OK) {
        printf("SHT40 sensor probing failed\n");
        sensirion_i2c_hal_sleep_usec(1000000); /* sleep 1s */
    }
    printf("SHT40 sensor probing successful\n");

    /*************************************************************************/
    error = stc3x_prepare_product_identifier();
    if (error) {
        printf("Error executing stc3x_prepare_product_identifier(): %i\n",
               error);
    }

    uint32_t product_number;
    uint8_t serial[8];
    error = stc3x_read_product_identifier(&product_number, serial, 8);
    if (error) {
        printf("Error executing stc3x_read_product_identifier(): %i\n", error);
    } else {
        printf("STC31 Product Number: 0x%08x\n", product_number);
        /***
         * old STC31 1261823:
         *      Product Number: 0x08010301
         * new STC31 1302025:
         *      Product Number: 0x08010302
         *
         */
    }

    /*************************************************************************/


    uint16_t self_test_output;
    error = stc3x_self_test(&self_test_output);
    if (error) {
        printf("Error executing stc3x_self_test(): %i\n", error);
    } else {
        printf("STC31 Self Test: 0x%04x (OK = 0x0000)\n", self_test_output);
    }

    error = stc3x_set_binary_gas(0x0013); //  here !!!!!
    if (error) {
        printf("Error executing stc3x_set_binary_gas(): %i\n", error);
    } else {
        printf("STC31 Set binary gas to 0x0013\n");
    }

    stc3x_enable_automatic_self_calibration(); //Turn on ASC

    /*************************************************************************/
    uint16_t gas_ticks;
    uint16_t temperature_ticks;

    float gas;
    float temperature;

    int32_t SHT40_temperature;
    int32_t SHT40_humidity;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

      // RH & Temperature Read Measurement
      error = sht4x_measure_blocking_read(&SHT40_temperature, &SHT40_humidity);
      if (error) {
          printf("Error RH & Temperature reading measurement\n");
      } else {
          printf("Temperature: %0.2f ^C, "
                 "Humidity: %0.2f %%RH, ",
                 SHT40_temperature / 1000.0f, SHT40_humidity / 1000.0f);
      }

      // Calibration RH and Temperature
      stc3x_set_relative_humidity((uint16_t) (SHT40_humidity / 1000.0f * 65535.0f / 100.0f));
      stc3x_set_temperature((uint16_t)(SHT40_temperature / 1000.0f * 200.0f));
      stc3x_set_pressure((uint16_t)1026);


      // CO2 Read Measurement
      error = stc3x_measure_gas_concentration(&gas_ticks, &temperature_ticks);
      if (error) {
          printf("Error executing stc3x_measure_gas_concentration(): %i\n",
                 error);
      } else {
          gas = 100 * ((float)gas_ticks - 16384.0) / 32768.0;
          //temperature = (float)temperature_ticks / 200.0;
          //printf("STC31 ==> Gas: %f - Temperature: %f\n", gas, temperature);
          printf("Gas: %f \n", gas);
      }

      //printf("*********************************************************************\n");
      //sensirion_i2c_hal_sleep_usec(900000); //900ms + 102ms = 1s
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
