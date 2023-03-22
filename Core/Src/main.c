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
uint32_t BMP388_atmosPressure;
uint32_t BMP388_atmosTemp;
float BMP388_fAtmosPressure;
float BMP388_fAtmosTemp;

extern struct bmp3_calib_data  calib_data;  //calibrate the data
extern struct bmp3_uncomp_data uncomp_data;
extern struct bmp3_data        comp_data;

void BMP388_Data_Calculate(void);

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
void sht4x_self_test();
void stc31_self_test();
void bmp388_self_test();

float one_order_low_pass_filter(float data_in, float fc);
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

    /******************************   Init  *********************************/
    printf("\n*************************\n");
    printf("STC31 & SHT40 & BMP388 test\n");
    HAL_Delay(100);
    printf("*************************\n");

    sensirion_i2c_hal_init(); //I2C2
    sht4x_self_test();
    stc31_self_test();
    bmp388_self_test();


    /*****************************  Parameters  ******************************/
    int16_t error = 0;

    uint16_t STC31_gas_ticks;
    uint16_t STC31_temperature_ticks;
    float STC31_gas;
    float STC31_temperature;
    float STC31_realCO2;

    int32_t SHT40_temperature;
    int32_t SHT40_humidity;


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */

      // SHT40 Measurement
      error = sht4x_measure_blocking_read(&SHT40_temperature, &SHT40_humidity);  //10ms
      if (error) { printf("Error RH & Temperature reading measurement\n");}
      else { printf("%0.2f, ""%0.2f, ", SHT40_temperature / 1000.0f, SHT40_humidity / 1000.0f);}

      // BMP388 Measurement
      //BMP388_I2C_WriteByte(BMP388_PWR_CTRL, 0x33);//press_on,temp on, normal mode.
      //BMP388_Data_Calculate();
      //printf("%0.2f, ", BMP388_fAtmosPressure);  //hpa

      // Calibration of RH/Temp/Press
      stc3x_set_relative_humidity((uint16_t) (SHT40_humidity / 1000.0f * 65535.0f / 100.0f));  //1ms
      stc3x_set_temperature((uint16_t)(SHT40_temperature / 1000.0f * 200.0f));  //1ms
      stc3x_set_pressure((uint16_t)1013);  //1ms



      // STC31 Measurement
      error = stc3x_measure_gas_concentration(&STC31_gas_ticks, &STC31_temperature_ticks);   //100ms
      if (error) { printf("Error executing stc3x_measure_gas_concentration(): %i\n", error);}
      else { STC31_gas = 100 * ((float)STC31_gas_ticks - 16384.0) / 32768.0;
          //STC31_temperature = (float)STC31_temperature_ticks / 200.0;
          //printf("%f \n", one_order_low_pass_filter(STC31_gas, 0.1));
          //y = 0.9326 x - 0.4448
          STC31_realCO2 = (STC31_gas + 0.3822) / 0.9284;  //100sccm
          //y=0.9284x-0.3822

          
          printf("%f \n", STC31_realCO2); // no filter
      }


      sensirion_i2c_hal_sleep_usec(30000); //30ms
      /*****************************  142ms  ->  7 Hz  ******************************/


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
void sht4x_self_test(){
    while (sht4x_probe() != STATUS_OK) {
    printf("SHT40 sensor probing failed\n");
    sensirion_i2c_hal_sleep_usec(1000000); /* sleep 1s */
    }
    printf("SHT40 sensor probing successful\n");
}

void stc31_self_test(){

    int16_t error = 0;
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

}

void bmp388_self_test(){

    uint8_t data;
    BMP388_I2C_ReadBuffer(BMP388_CHIP_ID,&data,1); // must be 0x50
    if (data != 0x50){
         printf("BMP388 sensor probing failed\n");
    } else{
         printf("BMP388 sensor probing successful\n");
    }

	BMP388_Get_CalibData();

    BMP388_I2C_WriteByte(BMP388_CMD,0xb6);
    sensirion_i2c_hal_sleep_usec(5000);

    BMP388_I2C_WriteByte(BMP388_OSR,0x00);    			//设置设置温度过采样*1 气压过采样*1  00 000 000
    BMP388_I2C_WriteByte(BMP388_CONFIG,0x04);    		//设置滤波系数2  0000 010 0
	BMP388_I2C_WriteByte(BMP388_ODR,0x01);				//设置输出分频系数，请按照相关公式计算得出要写的值
    BMP388_I2C_WriteByte(BMP388_PWR_CTRL,0x03);    	//使能气压和温度采样但是先不启动采样 00 00 00 1 1

}
/***
 *
 */
void BMP388_Data_Calculate(void){

    BMP388_atmosTemp = BMP388_GetData('T');
    BMP388_atmosPressure = BMP388_GetData('P');

    BMP388_Compensate_temperature();
	BMP388_Compensate_pressure();

    BMP388_fAtmosTemp = comp_data.temperature/100.0; //'C
    BMP388_fAtmosPressure = comp_data.pressure/10000.0; //HPa
}


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
