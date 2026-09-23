/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.c
  * @brief   This file provides code for the configuration
  *          of the TIM instances.
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
#include "tim.h"



/* USER CODE BEGIN 0 */
TIM_HandleTypeDef htim22;
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim2;

/* ȫ�ֱ��� -------------------------------------------------------*/
extern volatile uint32_t sys_tick_ms;          // ϵͳʱ�䣨���룩
extern volatile uint8_t  tick_10ms;            // 10ms ��־������ѭ��ʹ��
/**
 * @brief       PWM��ʼ��, ʵ���Ͼ��ǳ�ʼ����ʱ��
 * @note
 *              ��ʱ����ʱ����ԴAPB1 / APB2, ��APB1 / APB2 ��Ƶʱ, ��ʱ��Ƶ�ʻ��Զ�����
 *              ���, һ�������, ����Ҫ�ٸ���ʱ����Ƶ, ֱ��32Mhz ����ϵͳʱ��Ƶ��
 *              ��ʱ�����ʱ����㷽��: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft = ��ʱ������Ƶ��, ��λ: Mhz
 *
 * @param       arr: �Զ���װֵ
 * @param       psc: ʱ��Ԥ��Ƶ��
 * @retval      ��
 */
/* USER CODE END 0 */
/* TIM22 init function */
void MX_TIM22_Init(uint16_t arr, uint16_t psc)
{
    __HAL_RCC_TIM22_CLK_DISABLE();
    htim22.Instance = TIM22;
    htim22.Init.Prescaler = psc;
    htim22.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim22.Init.Period = arr;
    htim22.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim22.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim22) != HAL_OK)      // ��ʼ��Ϊ PWM Init
    {
        Error_Handler();
    }

    TIM_MasterConfigTypeDef sMasterConfig = {0};
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim22, &sMasterConfig);

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;           // ��ʼ��Ϊ PWM1 ģʽ
    sConfigOC.Pulse = 0;                          // ��ʼռ�ձ� 0
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim22, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) // ��ʼ��Ϊ PWM ConfigChannel
    {
        Error_Handler();
    }

    //HAL_TIM_PWM_Start(&htim22, TIM_CHANNEL_1);    // ���� PWM ���
}



/* TIM2 init function */
void MX_TIM2_Init(uint16_t arr, uint16_t psc)
{
    __HAL_RCC_TIM2_CLK_DISABLE();
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = psc;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = arr;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    TIM_MasterConfigTypeDef sMasterConfig = {0};
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig);

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
}


/* TIM6 init function */
void MX_TIM6_Init(uint16_t arr, uint16_t psc)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = psc;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = arr;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}


void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* tim_pwmHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(tim_pwmHandle->Instance==TIM22)
  {
  /* USER CODE BEGIN TIM22_MspInit 0 */

  /* USER CODE END TIM22_MspInit 0 */
    /* TIM22 clock enable */
    __HAL_RCC_TIM22_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /**TIM22 GPIO Configuration
    PA6     ------> TIM22_CH1
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_TIM22;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM22_MspInit 1 */

  /* USER CODE END TIM22_MspInit 1 */
  }
  else if(tim_pwmHandle->Instance==TIM2)
  {
    /* TIM2 clock enable */
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /**TIM2 GPIO Configuration
    PA5     ------> TIM2_CH1
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
}
void HAL_TIM_MspPostInit(TIM_HandleTypeDef* timHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(timHandle->Instance==TIM22)
  {
  /* USER CODE BEGIN TIM22_MspPostInit 0 */

  /* USER CODE END TIM22_MspPostInit 0 */

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**TIM22 GPIO Configuration
    PA6     ------> TIM22_CH1
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_TIM22;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM22_MspPostInit 1 */

  /* USER CODE END TIM22_MspPostInit 1 */
  }
  else if(timHandle->Instance==TIM2)
  {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**TIM2 GPIO Configuration
    PA5     ------> TIM2_CH1
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
}




/* USER CODE BEGIN 1 */
/**
 * @brief       ����PWM ռ�ձȲ���
 * @param       temp : 0~100��ARR=99
 * @retval      ��
 */
void pwm_set(uint16_t temp)
{
    __HAL_TIM_SET_COMPARE(&htim22, TIM_CHANNEL_1, temp);  /* �����µ�ռ�ձ� */
}
/**
 * @brief       ����PWM2 ռ�ձȲ���
 * @param       temp : 0~ARR��PWM2 ͨ��1��
 * @retval      ��
 */
void pwm_set2(uint16_t temp)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, temp);  /* �����µ�ռ�ձ� */
}
/* USER CODE END 1 */









void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM6)
  {
  /* USER CODE BEGIN TIM6_MspInit 0 */

  /* USER CODE END TIM6_MspInit 0 */
    /* TIM6 clock enable */
    __HAL_RCC_TIM6_CLK_ENABLE();

    /* TIM6 interrupt Init */
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
  /* USER CODE BEGIN TIM6_MspInit 1 */

  /* USER CODE END TIM6_MspInit 1 */
  }
}




void Tim6_Start(void)
{
	HAL_TIM_Base_Start_IT(&htim6);                       /* ʹ�ܶ�ʱ��x�Ͷ�ʱ�������ж� */
}

void Tim6_Stop(void)
{
	HAL_TIM_Base_Stop_IT(&htim6);                       /* ʧ�ܶ�ʱ��x�Ͷ�ʱ�������ж� */
}




/* USER CODE BEGIN 1 */
void TIM6_DAC_IRQHandler(void)
{
  /* USER CODE BEGIN TIM6_DAC_IRQn 0 */

  /* USER CODE END TIM6_DAC_IRQn 0 */
  HAL_TIM_IRQHandler(&htim6);
  /* USER CODE BEGIN TIM6_DAC_IRQn 1 */

  /* USER CODE END TIM6_DAC_IRQn 1 */
}



void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	
    if (htim->Instance == TIM6)
    {
        sys_tick_ms += 10;      // 10ms ʱ��
        tick_10ms = 1;          // ֪ͨ��ѭ��ɨ��
    }
}