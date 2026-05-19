/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.h
  * @brief   This file contains all the function prototypes for
  *          the gpio.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */
#define LED3_Pin GPIO_PIN_3
#define LED2_Pin GPIO_PIN_2
#define LED1_Pin GPIO_PIN_1
#define LED0_Pin GPIO_PIN_0
#define LED_GPIO_Port GPIOB

#define LED_0_ON          HAL_GPIO_WritePin(LED_GPIO_Port, LED0_Pin, GPIO_PIN_RESET)     /* ȡPOWER */
#define LED_0_OFF         HAL_GPIO_WritePin(LED_GPIO_Port, LED0_Pin, GPIO_PIN_SET)     /* ȡPOWER */
#define LED_1_ON          HAL_GPIO_WritePin(LED_GPIO_Port, LED1_Pin, GPIO_PIN_RESET)     /* ȡPOWER */
#define LED_1_OFF         HAL_GPIO_WritePin(LED_GPIO_Port, LED1_Pin, GPIO_PIN_SET)     /* ȡPOWER */
#define LED_2_ON          HAL_GPIO_WritePin(LED_GPIO_Port, LED2_Pin, GPIO_PIN_RESET)     /* ȡPOWER */
#define LED_2_OFF         HAL_GPIO_WritePin(LED_GPIO_Port, LED2_Pin, GPIO_PIN_SET)     /* ȡPOWER */
#define LED_3_ON          HAL_GPIO_WritePin(LED_GPIO_Port, LED3_Pin, GPIO_PIN_RESET)     /* ȡPOWER */
#define LED_3_OFF         HAL_GPIO_WritePin(LED_GPIO_Port, LED3_Pin, GPIO_PIN_SET)     /* ȡPOWER */

/* USER CODE BEGIN Private defines */

#define POWER_Pin GPIO_PIN_2
#define POWER_GPIO_Port GPIOA
#define POWERON           HAL_GPIO_WritePin(POWER_GPIO_Port, POWER_Pin, GPIO_PIN_SET)     /* ȡPOWER */
#define POWEROFF          HAL_GPIO_WritePin(POWER_GPIO_Port, POWER_Pin, GPIO_PIN_RESET)     /* ȡPOWER */

#define key_Pin GPIO_PIN_1
#define CHRG_Pin  GPIO_PIN_3
#define STDBY_Pin GPIO_PIN_4
#define key_GPIO_Port GPIOA
#define KEY             HAL_GPIO_ReadPin(key_GPIO_Port, key_Pin)     /* ȡKEY */
#define CHRG            HAL_GPIO_ReadPin(key_GPIO_Port, CHRG_Pin)     /* ȡKEY */
#define STDBY           HAL_GPIO_ReadPin(key_GPIO_Port, STDBY_Pin)     /* ȡKEY */
/* USER CODE END Private defines */

void MX_GPIO_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */

