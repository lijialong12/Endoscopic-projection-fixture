/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 主程序文件
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
#include "main.h"
#include "dma.h"
#include "stm32l0xx_hal.h"
#include "stm32l0xx_hal_tim.h"
#include "usart.h"
#include "gpio.h"
#include <stdint.h>
#include <stdio.h>
#include "adc.h"
#include "tim.h"
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
IWDG_HandleTypeDef hiwdg;

/**
  * @brief  独立看门狗初始化
  * @retval 无
  */
void MX_IWDG_Init(void)
{
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
  hiwdg.Init.Window = 4095;
  hiwdg.Init.Reload = 1000;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  喂狗函数
  * @retval 无
  */
void iwdg_feed(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}

/* 充电状态消抖参数 */
#define DEBOUNCE_THRESHOLD  50      // 消抖次数，循环周期 1ms → 50ms 确认

static uint32_t debounce_cnt = 0;   // 消抖计数器
static uint8_t  pending_state = 0;  // 临时状态（0:放电, 1:充电中, 2:充满）
static uint8_t  stable_state  = 0;  // 稳定状态（用于程序逻辑）

/**
  * @brief  充电状态消抖（每1ms调用一次）
  * @retval 无
  */
void ChargeState_Debounce(void)
{
    uint8_t current_state;

    if (!CHRG) {
        current_state = 1;          // 充电中
    } else if (!STDBY) {
        current_state = 2;          // 充满
    } else {
        current_state = 0;          // 放电
    }

    if (current_state != pending_state) {
        debounce_cnt = 0;
        pending_state = current_state;
    } else {
        if (debounce_cnt < DEBOUNCE_THRESHOLD) {
            debounce_cnt++;
        }
        if (debounce_cnt >= DEBOUNCE_THRESHOLD) {
            stable_state = current_state;   // 确认状态
        }
    }
}

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern ADC_HandleTypeDef hadc;
extern float ADC_ConvertedValue;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* 按键与时间相关宏定义 */
#define KEY_PRESSED()       (HAL_GPIO_ReadPin(key_GPIO_Port, key_Pin) == GPIO_PIN_RESET)  // 按键按下为低电平
#define LONG_PRESS_MS       2000            // 长按阈值：2秒
#define SCAN_INTERVAL_MS    10              // 按键扫描间隔：10ms
#define CHARGE              1               // 充电状态
#define DISCHARGE           0               // 放电状态

/* 全局变量 */
volatile uint32_t sys_tick_ms = 0;          // 系统毫秒计数器
volatile uint8_t  tick_10ms = 0;            // 10ms 标志，供主循环使用
float vol_v = 0;
uint8_t chargeflag = 0;                     // 当前是否处于充电状态

/* 按键状态机枚举 */
typedef enum {
    KS_IDLE = 0,
    KS_DEBOUNCE_DOWN,
    KS_PRESSED,
    KS_DEBOUNCE_UP
} KeyState_t;

/* 按键信息结构体 */
typedef struct {
    KeyState_t state;           // 当前状态
    uint32_t   press_start_ms;  // 按键按下的起始时间（毫秒）
    uint8_t    short_flag;      // 短按事件标志
    uint8_t    long_flag;       // 长按事件标志
    uint8_t    long_triggered;  // 长按是否已触发（防止重复触发）
} KeyInfo_t;

KeyInfo_t key;

/* 档位与 PWM 控制变量 */
static uint8_t gearkey = 1;     // 当前档位 (1/2/3)，对应 10%/20%/30%
static uint8_t pwm_is_on = 0;   // PWM 当前输出状态 (0:停止, 1:输出)

/* 函数声明 */
void KeyScan(void);             // 按键扫描状态机
void ShortPressAction(void);    // 短按动作
void LongPressAction(void);     // 长按动作

/**
  * @brief  按键扫描状态机（每10ms调用一次）
  * @retval 无
  */
void KeyScan(void)
{
    uint8_t is_pressed = KEY_PRESSED();

    switch (key.state) {
        case KS_IDLE:
            if (is_pressed) {
                key.state = KS_DEBOUNCE_DOWN;
                key.press_start_ms = sys_tick_ms;
            }
            break;

        case KS_DEBOUNCE_DOWN:
            if (is_pressed) {
                key.state = KS_PRESSED;
                key.long_triggered = 0;     // 新一次按键开始，重置长按触发标志
            } else {
                key.state = KS_IDLE;        // 抖动，视为无效
            }
            break;

        case KS_PRESSED:
            if (!is_pressed) {
                key.state = KS_DEBOUNCE_UP; // 按键释放，进入释放消抖
            } else {
                // 仍在按下，检查是否达到长按时间
                if (!key.long_triggered && (sys_tick_ms - key.press_start_ms >= LONG_PRESS_MS)) {
                    key.long_flag = 1;      // 置位长按事件标志
                    key.long_triggered = 1; // 避免重复触发
                }
            }
            break;

        case KS_DEBOUNCE_UP:
            if (!is_pressed) {
                // 确认已释放
                if (!key.long_triggered) {  // 没有触发过长按，则是短按
                    key.short_flag = 1;
                }
                key.state = KS_IDLE;
            } else {
                key.state = KS_PRESSED;     // 释放时的抖动，回到按下状态
            }
            break;
    }
}

/**
  * @brief  短按动作：切换 PWM 输出开关
  * @retval 无
  */
void ShortPressAction(void)
{
    switch (gearkey) {
        case 1: pwm_set(20); break;   // 20/200=10%
        case 2: pwm_set(40); break;   // 40/200=20%
        case 3: pwm_set(200); break;   // 60/200=30%
        default: pwm_set(0); break;
    }

    printf("短按：切换至占空比 %d%%\r\n", gearkey * 10);

    gearkey++;
    if (gearkey > 3) gearkey = 1;   // 档位循环：1 -> 2 -> 3 -> 1
}

/**
  * @brief  长按动作：循环切换占空比并强制打开 PWM
  * @retval 无
  */
void LongPressAction(void)
{
    if (pwm_is_on) {
        HAL_TIM_PWM_Stop(&htim22, TIM_CHANNEL_1);
        pwm_is_on = 0;
        POWEROFF;  // 取POWER  
        printf("长按：PWM 停止\r\n");
    } else {
        HAL_TIM_PWM_Start(&htim22, TIM_CHANNEL_1);
        pwm_is_on = 1;
        POWERON;  // 取POWER  
        printf("长按：PWM 启动\r\n");
    }
}

/* USER CODE END PFP */

/**
  * @brief  主函数
  * @retval int
  */
int main(void)
{
    /* USER CODE BEGIN 1 */
    uint32_t adc_val = 0;

    /* USER CODE END 1 */

    HAL_Init();
    SystemClock_Config();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART1_UART_Init(115200);
    MX_ADC_Init();
    MX_TIM22_Init(200, 16 - 1);             // PWM频率约10KHz
    MX_TIM6_Init(100 - 1, 3200 - 1);        // 32MHz/(3200*100)=100Hz，即10ms中断
    MX_IWDG_Init();

    /* USER CODE BEGIN 2 */
    // 初始化按键结构体
    memset(&key, 0, sizeof(key));

    // 初始 PWM 状态：占空比20%，输出关闭
    pwm_set(20);
    HAL_TIM_PWM_Stop(&htim22, TIM_CHANNEL_1);
    pwm_is_on = 0;

    // 启动定时器6中断
    Tim6_Start();

    /* 充电状态初始消抖（简单快速确认） */
    {
        uint8_t init_state1, init_state2;
        init_state1 = (!CHRG) ? 1 : ((!STDBY) ? 2 : 0);
        HAL_Delay(20);
        init_state2 = (!CHRG) ? 1 : ((!STDBY) ? 2 : 0);
        if (init_state1 == init_state2) {
            stable_state = pending_state = init_state1;
            debounce_cnt = DEBOUNCE_THRESHOLD;
        } else {
            stable_state = pending_state = 0;  // 默认放电
            debounce_cnt = 0;
        }
    }

    // 根据初始状态设置 chargeflag（若后续未使用可忽略）
    if (stable_state == 1) {
        chargeflag = CHARGE;
    } else {
        chargeflag = DISCHARGE;

        HAL_ADC_Start(&hadc);
        if (HAL_ADC_PollForConversion(&hadc, 100) == HAL_OK)
        {
            adc_val = HAL_ADC_GetValue(&hadc);
            vol_v = ((adc_val * 3.3f) / 4096.0f) * 2.0f;

            if (vol_v > 4.0f) {
                LED_0_ON; LED_1_ON; LED_2_ON; LED_3_ON;
            }
            else if (vol_v > 3.0f && vol_v <= 4.0f) {
                LED_0_ON; LED_1_ON; LED_2_ON; LED_3_OFF;
            }
            else if (vol_v > 2.0f && vol_v <= 3.0f) {
                LED_0_ON; LED_1_ON; LED_2_OFF; LED_3_OFF;
            }
            else {
                LED_0_ON; LED_1_OFF; LED_2_OFF; LED_3_OFF;
            }
        }
        HAL_ADC_Stop(&hadc);
    }

    /* USER CODE END 2 */

    // 流水灯与放电刷新相关变量（静态，保持整个生命周期）
    static uint32_t last_time = 0;
    static uint8_t  led_count = 0;    // 0:全灭, 1~3:已亮个数, 4:全亮待熄灭
    static uint8_t  reveflag = 1;     // 放电状态ADC刷新标志，0:需要刷新, 1:已刷新

    while (1)
    {
        /* 每1ms调用充电状态消抖 */
        ChargeState_Debounce();

        /* ---- ADC 采集与打印（每5分钟一次） ---- */
        static uint32_t last_adc_time = 0;
        if (sys_tick_ms - last_adc_time >= 300000)   // 300000ms = 5分钟
        {
            last_adc_time = sys_tick_ms;

            // 仅当消抖确认为放电状态，且上次退出充电/充满后已刷新过（reveflag==1）时更新LED
            if (stable_state == 0 && reveflag == 1)
            {
                HAL_ADC_Start(&hadc);
                if (HAL_ADC_PollForConversion(&hadc, 100) == HAL_OK)
                {
                    adc_val = HAL_ADC_GetValue(&hadc);
                    vol_v = ((adc_val * 3.3f) / 4096.0f) * 2.0f;

                    if (vol_v > 4.0f) {
                        LED_0_ON; LED_1_ON; LED_2_ON; LED_3_ON;
                    }
                    else if (vol_v > 3.0f && vol_v <= 4.0f) {
                        LED_0_ON; LED_1_ON; LED_2_ON; LED_3_OFF;
                    }
                    else if (vol_v > 2.0f && vol_v <= 3.0f) {
                        LED_0_ON; LED_1_ON; LED_2_OFF; LED_3_OFF;
                    }
                    else {
                        LED_0_ON; LED_1_OFF; LED_2_OFF; LED_3_OFF;
                    }
                }
                HAL_ADC_Stop(&hadc);
            }
        }

        /* ---- 按键扫描（每10ms一次） ---- */
        if (tick_10ms) {
            tick_10ms = 0;
            KeyScan();
            iwdg_feed();
        }

        /* ---- 处理按键事件 ---- */
        if (key.short_flag) {
            key.short_flag = 0;
            ShortPressAction();
        }
        if (key.long_flag) {
            key.long_flag = 0;
            LongPressAction();
        }

        /* ---- 充电/放电 LED 显示状态机（基于消抖后的 stable_state） ---- */
        if (stable_state == 1)   // 充电中
        {
            if (HAL_GetTick() - last_time >= 500)
            {
                last_time = HAL_GetTick();

                // 标记当前处于充电/充满状态，以便退出时刷新放电LED
                reveflag = 0;

                if (led_count == 4)
                {
                    led_count = 0;
                }
                else
                {
                    // 依次点亮 LED0 ~ LED3（累积点亮，不熄灭已点亮的）
                    switch (led_count)
                    {
                        case 0: LED_0_ON;  LED_1_OFF; LED_2_OFF; LED_3_OFF; break;
                        case 1: LED_0_ON;  LED_1_ON;  LED_2_OFF; LED_3_OFF; break;
                        case 2: LED_0_ON;  LED_1_ON;  LED_2_ON;  LED_3_OFF; break;
                        case 3: LED_0_ON;  LED_1_ON;  LED_2_ON;  LED_3_ON;  break;
                        default: break;
                    }
                    led_count++;
                }
            }
        }
        else if (stable_state == 2)   // 充满
        {
            // 充满状态 LED 全亮
            LED_0_ON;
            LED_1_ON;
            LED_2_ON;
            LED_3_ON;

            // 重置流水灯变量，以便下次充电重新开始
            reveflag = 0;
            led_count = 0;
            last_time = HAL_GetTick();
        }
        else   // 放电状态 (stable_state == 0)
        {
            // 刚从充电/充满退出时，立即用 ADC 刷新一次 LED 显示
            if (reveflag == 0)
            {
                HAL_ADC_Start(&hadc);
                if (HAL_ADC_PollForConversion(&hadc, 100) == HAL_OK)
                {
                    adc_val = HAL_ADC_GetValue(&hadc);
                    vol_v = ((adc_val * 3.3f) / 4096.0f) * 2.0f;

                    if (vol_v > 4.0f) {
                        LED_0_ON; LED_1_ON; LED_2_ON; LED_3_ON;
                    }
                    else if (vol_v > 3.0f && vol_v <= 4.0f) {
                        LED_0_ON; LED_1_ON; LED_2_ON; LED_3_OFF;
                    }
                    else if (vol_v > 2.0f && vol_v <= 3.0f) {
                        LED_0_ON; LED_1_ON; LED_2_OFF; LED_3_OFF;
                    }
                    else {
                        LED_0_ON; LED_1_OFF; LED_2_OFF; LED_3_OFF;
                    }
                }
                HAL_ADC_Stop(&hadc);
                reveflag = 1;   // 标记已刷新，后续由5分钟定时更新
            }
        }

        HAL_Delay(1);   // 保持1ms循环周期（与消抖函数调用频率一致）
    }
}

/**
  * @brief  System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */