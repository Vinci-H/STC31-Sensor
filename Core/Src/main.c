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
#include "bmp388.h"

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

PRESS_EN_SENSOR_TYPY bmp388Type;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
float one_order_low_pass_filter(float data_in, float fc);
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

    printf("\n*************************\n");
    printf("STC31 & SHT40 test\n");
    HAL_Delay(100);
    printf("*************************\n");


    int16_t error = 0;

    sensirion_i2c_hal_init(); //I2C2
    pressSensorInit(&bmp388Type, hi2c2);

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

    //stc3x_enable_automatic_self_calibration(); //Turn on ASC, this ASC is developed for other use.

    /*************************************************************************/
    uint16_t gas_ticks;
    uint16_t temperature_ticks;

    float gas;
    float temperature;

    int32_t SHT40_temperature;
    int32_t SHT40_humidity;

    int32_t BMP388_Temperature;
    int32_t BMP388_Pressure;
    int32_t BMP388_Altitude;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

      // RH & Temperature Read Measurement
      error = sht4x_measure_blocking_read(&SHT40_temperature, &SHT40_humidity);  //10ms
      if (error) {
          printf("Error RH & Temperature reading measurement\n");
      } else {
          printf("%0.2f, "
                 "%0.2f, ",
                 SHT40_temperature / 1000.0f, SHT40_humidity / 1000.0f);
      }

      pressSensorDataGet(&BMP388_Temperature, &BMP388_Pressure, &BMP388_Altitude);
      printf("%0.2f, ", BMP388_Pressure);

      // Calibration RH and Temperature and Pressure!!!!!
      stc3x_set_relative_humidity((uint16_t) (SHT40_humidity / 1000.0f * 65535.0f / 100.0f));  //1ms
      stc3x_set_temperature((uint16_t)(SHT40_temperature / 1000.0f * 200.0f));  //1ms
      //stc3x_set_pressure((uint16_t)1012);  //1ms


      // CO2 Read Measurement
      error = stc3x_measure_gas_concentration(&gas_ticks, &temperature_ticks);   //100ms
      if (error) {
          printf("Error executing stc3x_measure_gas_concentration(): %i\n",
                 error);
      } else {
          gas = 100 * ((float)gas_ticks - 16384.0) / 32768.0;
          //temperature = (float)temperature_ticks / 200.0;
          //printf("STC31 ==> Gas: %f - Temperature: %f\n", gas, temperature);
          //printf("%f \n", one_order_low_pass_filter(gas, 0.1));
          printf("%f \n", gas); // no filter
      }

      //printf("*********************************************************************\n");
      sensirion_i2c_hal_sleep_usec(30000); //112ms  +  30ms   =  142ms  ->  7 Hz
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
#define LPF_Ts 0.142f
#define PI     3.1415926f

float one_order_low_pass_filter(float data_in, float fc)
{
	static float alpha, data_out_last = -999.999f;
	float delta, data_temp;
	delta = 2.0f * PI * fc * LPF_Ts;
	alpha = delta / (delta + 1.0f);

	if (data_out_last != -999.999f)
	{
		data_temp = (alpha * data_in + (1.0f - alpha) * data_out_last);
	}
	else
	{
		data_temp = data_in;
	}
	data_out_last = data_temp;
	return data_temp;
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
