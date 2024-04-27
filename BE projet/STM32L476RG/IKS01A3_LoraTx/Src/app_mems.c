/**
  ******************************************************************************
  * File Name          : app_mems.c
  * Description        : This file provides code for the configuration
  *                      of the STMicroelectronics.X-CUBE-MEMS1.10.0.0 instances.
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

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "app_mems.h"
#include "main.h"
#include <stdio.h>

#include "iks01a3_motion_sensors.h"
#include "iks01a3_env_sensors.h"
#include "stm32l4xx_nucleo.h"
#include "math.h"
#include <stdlib.h>
/* Private typedef -----------------------------------------------------------*/
typedef struct displayFloatToInt_s {
  int8_t sign; /* 0 means positive, 1 means negative*/
  uint32_t  out_int;
  uint32_t  out_dec;
} displayFloatToInt_t;

typedef struct {
	int8_t acc_x, acc_y, acc_z;
	uint32_t press;
	uint16_t hum,temp;
}data_send;
extern UART_HandleTypeDef huart1;
/* Private define ------------------------------------------------------------*/
#define MAX_BUF_SIZE 256

/* Private variables ---------------------------------------------------------*/
static volatile uint8_t PushButtonDetected = 0;
static IKS01A3_MOTION_SENSOR_Capabilities_t MotionCapabilities[IKS01A3_MOTION_INSTANCES_NBR];
static IKS01A3_ENV_SENSOR_Capabilities_t EnvCapabilities[IKS01A3_ENV_INSTANCES_NBR];
static char dataOut[MAX_BUF_SIZE];
static int32_t PushButtonState = GPIO_PIN_RESET;

/* Private function prototypes -----------------------------------------------*/
static void floatToInt(float in, displayFloatToInt_t *out_value, int32_t dec_prec);
static void MX_IKS01A3_DataLogTerminal_Init_G21(void);
static void MX_IKS01A3_DataLogTerminal_Process_G21(void);
static void Accelero_Sensor_Handler_G21(data_send *dataSend);
static void Temp_Sensor_Handler_G21(data_send *dataSend);
static void Press_Sensor_Handler_G21(data_send *dataSend);
static void Hum_Sensor_Handler_G21(data_send *dataSend);
static void lora_init(void);
static void lora_send(char *dataSend);
void MX_MEMS_Init(void)
{
  /* USER CODE BEGIN SV */

  /* USER CODE END SV */

  /* USER CODE BEGIN MEMS_Init_PreTreatment */

  /* USER CODE END MEMS_Init_PreTreatment */

  /* Initialize the peripherals and the MEMS components */
	MX_IKS01A3_DataLogTerminal_Init_G21();
	lora_init();
  /* USER CODE BEGIN MEMS_Init_PostTreatment */

  /* USER CODE END MEMS_Init_PostTreatment */
}

/*
 * LM background task
 */
void MX_MEMS_Process(void)
{
  /* USER CODE BEGIN MEMS_Process_PreTreatment */

  /* USER CODE END MEMS_Process_PreTreatment */

	MX_IKS01A3_DataLogTerminal_Process_G21();

  /* USER CODE BEGIN MEMS_Process_PostTreatment */

  /* USER CODE END MEMS_Process_PostTreatment */
}

/**
  * @brief  Initialize the DataLogTerminal application
  * @retval None
  */
void MX_IKS01A3_DataLogTerminal_Init_G21(void)
{

  /* Initialize LED */
  BSP_LED_Init(LED2);

  /* Initialize button */
  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);

  /* Check what is the Push Button State when the button is not pressed. It can change across families */
  PushButtonState = (BSP_PB_GetState(BUTTON_KEY)) ?  0 : 1;

  /* Initialize Virtual COM Port */
  BSP_COM_Init(COM1);

  IKS01A3_MOTION_SENSOR_Init(IKS01A3_LSM6DSO_0, MOTION_ACCELERO | MOTION_GYRO);

  IKS01A3_MOTION_SENSOR_GetCapabilities(0, &MotionCapabilities[0]);
  snprintf(dataOut, MAX_BUF_SIZE,
             "\r\nMotion Sensor Instance %d capabilities: \r\n ACCELEROMETER: %d\r\n GYROSCOPE: %d\r\n MAGNETOMETER: %d\r\n LOW POWER: %d\r\n",
             0, MotionCapabilities[0].Acc, MotionCapabilities[0].Gyro, MotionCapabilities[0].Magneto, MotionCapabilities[0].LowPower);
  printf("%s", dataOut);
  printf("hi");

  IKS01A3_ENV_SENSOR_Init(IKS01A3_HTS221_0, ENV_TEMPERATURE | ENV_HUMIDITY);

  IKS01A3_ENV_SENSOR_Init(IKS01A3_LPS22HH_0, ENV_TEMPERATURE | ENV_PRESSURE);


  IKS01A3_ENV_SENSOR_GetCapabilities(0, &EnvCapabilities[0]);
  snprintf(dataOut, MAX_BUF_SIZE,
			 "\r\nEnvironmental Sensor Instance %d capabilities: \r\n TEMPERATURE: %d\r\n PRESSURE: %d\r\n HUMIDITY: %d\r\n LOW POWER: %d\r\n",
			 0, EnvCapabilities[0].Temperature, EnvCapabilities[0].Pressure, EnvCapabilities[0].Humidity, EnvCapabilities[0].LowPower);
  printf("%s", dataOut);
  IKS01A3_ENV_SENSOR_GetCapabilities(1, &EnvCapabilities[1]);
    snprintf(dataOut, MAX_BUF_SIZE,
  			 "\r\nEnvironmental Sensor Instance %d capabilities: \r\n TEMPERATURE: %d\r\n PRESSURE: %d\r\n HUMIDITY: %d\r\n LOW POWER: %d\r\n",
  			 1, EnvCapabilities[1].Temperature, EnvCapabilities[1].Pressure, EnvCapabilities[1].Humidity, EnvCapabilities[1].LowPower);
    printf("%s", dataOut);

}
/**
  * @brief  BSP Push Button callback
  * @param  Button Specifies the pin connected EXTI line
  * @retval None.
  */
void BSP_PB_Callback(Button_TypeDef Button)
{
  PushButtonDetected = 1;
}

/**
  * @brief  Process of the DataLogTerminal application
  * @retval None
  */
void MX_IKS01A3_DataLogTerminal_Process_G21(void)
{
  data_send data;
  if (PushButtonDetected != 0U)
  {
    /* Debouncing */
    HAL_Delay(50);

    /* Wait until the button is released */
    while ((BSP_PB_GetState( BUTTON_KEY ) == PushButtonState));

    /* Debouncing */
    HAL_Delay(50);

    /* Reset Interrupt flag */
    PushButtonDetected = 0;

    /* Do nothing */
  }

  Accelero_Sensor_Handler_G21(&data);
  Temp_Sensor_Handler_G21(&data);
  Hum_Sensor_Handler_G21(&data);
  Press_Sensor_Handler_G21(&data);
  snprintf(dataOut, MAX_BUF_SIZE, "%x%d%d%d%d%d%d%d%d%ld\n",
      				(data.acc_x >= 0) ? 2: 3, abs(data.acc_x)
      				,(data.acc_y >= 0) ? 2: 3,abs(data.acc_y)
      				,(data.acc_z >= 0) ? 2: 3, abs(data.acc_z)
      				,(data.temp >= 0)? 2 : 3 ,data.temp,
					data.hum,
					data.press);
  lora_send(dataOut);
  HAL_Delay( 1000 );
}
/**
  * @brief  Splits a float into two integer values.
  * @param  in the float value as input
  * @param  out_value the pointer to the output integer structure
  * @param  dec_prec the decimal precision to be used
  * @retval None
  */
static void floatToInt(float in, displayFloatToInt_t *out_value, int32_t dec_prec)
{
  if(in >= 0.0f)
  {
    out_value->sign = 0;
  }else
  {
    out_value->sign = 1;
    in = -in;
  }

  in = in + (0.5f / pow(10, dec_prec));
  out_value->out_int = (int32_t)in;
  in = in - (float)(out_value->out_int);
  out_value->out_dec = (int32_t)trunc(in * pow(10, dec_prec));
}

/**
  * @brief  Handles the accelerometer axes data getting/sending
  * @param  None
  * @retval None
  */


static void Accelero_Sensor_Handler_G21(data_send *dataSend)
{

  IKS01A3_MOTION_SENSOR_Axes_t acceleration;

  if (IKS01A3_MOTION_SENSOR_GetAxes(0, MOTION_ACCELERO, &acceleration))
  {
    snprintf(dataOut, MAX_BUF_SIZE, "\r\nACC: Error\r\n");
  }
  else
  {

	dataSend->acc_x = (int)(acceleration.x/100);
	if(dataSend->acc_x < 0){
		dataSend->acc_x = (dataSend->acc_x < -9) ? -9 :dataSend->acc_x;
	}else{
		dataSend->acc_x = (dataSend->acc_x > 9) ? 9 :dataSend->acc_x;
	}
	dataSend->acc_y = (int)(acceleration.y/100);
	if(dataSend->acc_y < 0){
		dataSend->acc_y = (dataSend->acc_y < -9) ? -9 :dataSend->acc_y;
	}else{
		dataSend->acc_y = (dataSend->acc_y > 9) ? 9 :dataSend->acc_y;
	}
	dataSend->acc_z = (int)(acceleration.z/100);
	if(dataSend->acc_z < 0){
		dataSend->acc_z = (dataSend->acc_z < -9) ? -9 :dataSend->acc_z;
	}else{
		dataSend->acc_z = (dataSend->acc_z > 9) ? 9 :dataSend->acc_z;
	}
    snprintf(dataOut, MAX_BUF_SIZE, "\r\nACC_X : %d, ACC_Y: %d, ACC_Z: %d\r\n",
    		dataSend->acc_x, dataSend->acc_y, dataSend->acc_z);
  }
  printf("%s", dataOut);


}


/**
  * @brief  Handles the temperature data getting/sending
  * @param  None
  * @retval None
  */

static void Temp_Sensor_Handler_G21(data_send *dataSend)
{

  float temperature;
  displayFloatToInt_t out_value;

  if (IKS01A3_ENV_SENSOR_GetValue(0, ENV_TEMPERATURE, &temperature))
  {
    snprintf(dataOut, MAX_BUF_SIZE, "\r\nTemp: Error\r\n");
  }
  else
  {
    floatToInt(temperature, &out_value, 2);
    dataSend->temp = temperature*100;
    snprintf(dataOut, MAX_BUF_SIZE, "\r\nTemp: %0.2f degC\r\n",temperature);
  }

  printf("%s", dataOut);

}

/**
  * @brief  Handles the pressure sensor data getting/sending
  * @param  Instance the device instance
  * @retval None
  */
static void Press_Sensor_Handler_G21(data_send *dataSend)
{
  float pressure;
  displayFloatToInt_t out_value;

  if (IKS01A3_ENV_SENSOR_GetValue(1, ENV_PRESSURE, &pressure))
  {
    snprintf(dataOut, MAX_BUF_SIZE, "\r\nPress: Error\r\n");
  }
  else
  {

    floatToInt(pressure, &out_value, 2);
    dataSend->press = pressure*100;
    snprintf(dataOut, MAX_BUF_SIZE, "\r\nPress: %0.2f hPa\r\n",pressure);
  }

  printf("%s", dataOut);
}

/**
  * @brief  Handles the humidity data getting/sending
  * @param  Instance the device instance
  * @retval None
  */


static void Hum_Sensor_Handler_G21(data_send *dataSend)
{
  float humidity;
  displayFloatToInt_t out_value;


  if (IKS01A3_ENV_SENSOR_GetValue(0, ENV_HUMIDITY, &humidity))
  {
    snprintf(dataOut, MAX_BUF_SIZE, "\r\nHum: Error\r\n");
  }
  else
  {
    floatToInt(humidity, &out_value, 2);
    dataSend->hum = humidity * 100;
    snprintf(dataOut, MAX_BUF_SIZE, "\r\nHum : %0.2f %%\r\n",humidity);
  }

  printf("%s", dataOut);

}

static void lora_init(void){
	char command1[] = "AT+MODE=TEST\r\n" ;
	char response[1000] = "";
	HAL_UART_Transmit(&huart1, (uint8_t*)command1, strlen(command1), 1000);
	HAL_UART_Receive(&huart1, (uint8_t*)response, sizeof(response), 1000);
}

static void lora_send(char *dataSend){
	lora_init();
	char command2[] = "AT+TEST=TXLRPKT," ;
	char response[1000] = "";
	HAL_UART_Transmit(&huart1, (uint8_t*)command2, strlen(command2), 1000);
	HAL_UART_Transmit(&huart1, (uint8_t*)dataSend, strlen(dataSend), 1000);
    HAL_UART_Receive(&huart1, (uint8_t*)response, sizeof(response), 1000);
	HAL_UART_Transmit(&huart2, (uint8_t*)response, strlen(response), 1000);
}
#ifdef __cplusplus
}
#endif
