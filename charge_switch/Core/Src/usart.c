/* Includes ------------------------------------------------------------------*/
#include "usart.h"
#include "stdio.h"
#include <string.h>
/* USER CODE BEGIN 0 */
extern DMA_HandleTypeDef hdma_tim2_ch2;
/* USER CODE END 0 */
/* 如果使用os,则包括下面的头文件即可 */
#if SYS_SUPPORT_OS
#include "os.h"                               /* os 使用 */
#endif

/******************************************************************************************/
/* 加入以下代码, 支持printf函数, 而不需要选择use MicroLIB */
#if 1
#if (__ARMCC_VERSION >= 6010050)                    /* 使用AC6编译器时 */
__asm(".global __use_no_semihosting\n\t");          /* 声明不使用半主机模式 */
__asm(".global __ARM_use_no_argv \n\t");            /* AC6下需要声明main函数为无参数格式，否则部分例程可能出现半主机模式 */
#else
/* 使用AC5编译器时, 要在这里定义__FILE 和 不使用半主机模式 */
#pragma import(__use_no_semihosting)
struct __FILE
{
    int handle;
};
#endif

/* 不使用半主机模式，至少需要重定义_ttywrch\_sys_exit\_sys_command_string函数,以同时兼容AC6和AC5模式 */
int _ttywrch(int ch)
{
    ch = ch;
    return ch;
}

/* 定义_sys_exit()以避免使用半主机模式 */
void _sys_exit(int x)
{
    x = x;
}

char *_sys_command_string(char *cmd, int len)
{
    return NULL;
}

/* FILE 在 stdio.h里面定义. */
FILE __stdout;

/* 重定义fputc函数, printf函数最终会通过调用fputc输出字符串到串口 */
int fputc(int ch, FILE *f)
{
    while ((USART1->ISR & 0X40) == 0);               /* 等待上一个字符发送完成 */
    USART1->TDR = (uint8_t)ch;                       /* 将要发送的字符 ch 写入到DR寄存器 */
    return ch;
}

/* GCC/newlib 使用 _write -> __io_putchar 路径，需要定义这两个函数 */
int __io_putchar(int ch)
{
    while ((USART1->ISR & 0X40) == 0);
    USART1->TDR = (uint8_t)ch;
    return ch;
}

int __io_getchar(void)
{
    while ((USART1->ISR & 0X20) == 0);
    return (int)(USART1->RDR & 0xFF);
}
#endif

// 正点原子风格 - 串口1中断接收定义
#define RXBUFFERSIZE    1                       /* HAL库单字节接收缓冲 */

/* 串口1接收缓冲, 最大USART1_REC_LEN个字节 */
uint8_t g_usart1_rx_buf[USART1_REC_LEN];
/* 串口1接收状态
 * bit15：接收完成标志
 * bit14：接收到0x0d
 * bit13~0：接收到的有效字节数目
*/
uint16_t g_usart1_rx_sta = 0;
uint8_t g_usart1_rx_buffer[RXBUFFERSIZE];       /* HAL库串口1接收缓冲（单字节） */

// 原有全局变量（兼容上层逻辑）
#ifndef USARTLEN_2
#define USARTLEN_2  255      //串口2 DMA缓冲最大接收长度
#endif

char        UsartRxData2[USARTLEN_2];  			// USART2接收缓冲区（DMA方式）
uint16_t    UsartRxLen2 = 0;    			// USART2接收数据长度
uint8_t     UsartTxflag1 = 0;    			// USART1_TX DMA完成标志
uint8_t     UsartTxflag2 = 0;    			// USART2_TX DMA完成标志

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_tx;  // 仅保留USART1_TX DMA
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE END 0 */

/* USART1 init function - 对齐正点原子逻辑 */
void MX_USART1_UART_Init(uint32_t baudrate)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = baudrate;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }

  /* 正点原子逻辑：开启单字节接收中断（UART_IT_RXNE） */
  HAL_UART_Receive_IT(&huart1, (uint8_t *)g_usart1_rx_buffer, RXBUFFERSIZE);
}

/* USART2 init function - 保留原有DMA逻辑 */
void MX_USART2_UART_Init(uint32_t baudrate)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = baudrate;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  // USART2保留DMA接收，使能IDLE中断
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
  // 初始化DMA接收
  HAL_UART_Receive_DMA(&huart2, (uint8_t *)UsartRxData2, USARTLEN_2);
}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;  // 上拉避免电平噪声
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 DMA Init - 仅保留TX DMA */
    hdma_usart1_tx.Instance = DMA1_Channel2;
    hdma_usart1_tx.Init.Request = DMA_REQUEST_3;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }
    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

    /* USART1中断配置（正点原子风格：抢占3，子优先级3） */
    HAL_NVIC_SetPriority(USART1_IRQn, 3, 3);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  }
  else if(uartHandle->Instance==USART2)
  {
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 DMA Init */
    hdma_usart2_rx.Instance = DMA1_Channel6;
    hdma_usart2_rx.Init.Request = DMA_REQUEST_4;
    hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx.Init.Mode = DMA_NORMAL;
    hdma_usart2_rx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
    {
      Error_Handler();
    }
    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart2_rx);

    hdma_usart2_tx.Instance = DMA1_Channel4;
    hdma_usart2_tx.Init.Request = DMA_REQUEST_4;
    hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode = DMA_NORMAL;
    hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
    {
      Error_Handler();
    }
    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart2_tx);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 3, 4);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  }
}

// 正点原子风格 - 串口1接收完成回调函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)             /* 如果是串口1 */
    {
        if((g_usart1_rx_sta & 0x8000) == 0)  /* 接收未完成 */
        {
            if(g_usart1_rx_sta & 0x4000)     /* 接收到了0x0d */
            {
                if(g_usart1_rx_buffer[0] != 0x0a)
                {
                    g_usart1_rx_sta = 0;     /* 接收错误,重新开始 */
                }
                else
                {
                    g_usart1_rx_sta |= 0x8000;   /* 接收完成标记 */
                }
            }
            else                                /* 还没收到0X0D */
            {
                if(g_usart1_rx_buffer[0] == 0x0d)
                {
                    g_usart1_rx_sta |= 0x4000;
                }
                else
                {
                    g_usart1_rx_buf[g_usart1_rx_sta & 0X3FFF] = g_usart1_rx_buffer[0] ;
                    g_usart1_rx_sta++;
                    if(g_usart1_rx_sta > (USART1_REC_LEN - 1))
                    {
                        g_usart1_rx_sta = 0;     /* 接收数据错误,重新开始接收 */
                    }
                }
            }
        }

        /* 重新开启单字节接收中断（正点原子核心逻辑） */
        HAL_UART_Receive_IT(&huart1, (uint8_t *)g_usart1_rx_buffer, RXBUFFERSIZE);
    }
}

// DMA发送完成回调函数
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        UsartTxflag1 = 1;
    }
    if (huart->Instance == USART2)
    {
        UsartTxflag2 = 1;
    }
}

/*
*  功能：复制串口缓冲区数据到目标数组（兼容原有接口，适配正点原子接收逻辑）
*  参数：
*   usart：串口索引 {1 2}
*   dest： 目标数组 {字符串}
*   maxLen 拷贝的限制长度
*/
void CopySerialData(uint8_t usart, char *dest, int maxLen)
{
    if (dest == NULL || maxLen <= 0) return;

    // 临界区保护：禁止中断，避免拷贝时数据被修改
    __disable_irq();
    switch(usart)
    {
        case 1:
            {
                // 串口1：读取正点原子风格的接收缓冲区
                uint16_t rx_len = g_usart1_rx_sta & 0x3FFF;
                int lenToCopy = (rx_len <= maxLen) ? rx_len : (maxLen-1);
                memcpy(dest, g_usart1_rx_buf, lenToCopy);
                dest[lenToCopy] = '\0'; // 字符串终止符
            }
            break;
        case 2:
            {
                int lenToCopy = (UsartRxLen2 <= maxLen) ? UsartRxLen2 : (maxLen-1);
                memcpy(dest, UsartRxData2, lenToCopy);
                dest[lenToCopy] = '\0';
            }
            break;
        default:break;
    }
    __enable_irq();
}

/*
*  功能：串口发送函数（DMA方式）
*  参数：
*   usart：串口索引 {1 2}
*   dest：发送目标字符串
*   len：发送长度
*/
void Usart_Transmit(uint8_t usart, char *dest, uint16_t len)
{
    if (dest == NULL || len == 0) return;

    switch(usart)
    {
		case 1:
			UsartTxflag1 = 0;
			HAL_UART_Transmit_DMA(&huart1, (uint8_t *)dest, len);
			break;
        case 2:
            UsartTxflag2 = 0;
            HAL_UART_Transmit_DMA(&huart2, (uint8_t *)dest, len);
            break;
        default:break;
    }
}

/*
*  功能：清除串口缓冲区数据（兼容原有接口，适配正点原子接收逻辑）
*  参数：
*   usart：串口索引 1 2
*/
void ClearSerialBuffer(uint8_t usart)
{
    __disable_irq();
    switch(usart)
    {
        case 1:
            memset(g_usart1_rx_buf, 0, USART1_REC_LEN);
            g_usart1_rx_sta = 0;
            __enable_irq();
            HAL_UART_Receive_IT(&huart1, g_usart1_rx_buffer, 1); // ★ 只加这一行
            return;
        case 2:
            memset(UsartRxData2, 0, USARTLEN_2);
            UsartRxLen2 = 0;
            HAL_UART_Receive_DMA(&huart2, (uint8_t *)UsartRxData2, USARTLEN_2);
            break;
        default:break;
    }
    __enable_irq();
}




/**
  * @brief This function handles DMA1 channel 2 and 3 interrupts.
  */
void DMA1_Channel2_3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel2_3_IRQn 0 */

  /* USER CODE END DMA1_Channel2_3_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart1_tx);
  /* USER CODE BEGIN DMA1_Channel2_3_IRQn 1 */

  /* USER CODE END DMA1_Channel2_3_IRQn 1 */
}



/* USER CODE BEGIN 1 */
/**
  * @brief USART1中断服务函数（正点原子风格）
  */
void USART1_IRQHandler(void)
{
#if SYS_SUPPORT_OS                              /* 使用OS */
    OSIntEnter();
#endif

    HAL_UART_IRQHandler(&huart1);       /* 调用HAL库中断处理公用函数 */

#if SYS_SUPPORT_OS                              /* 使用OS */
    OSIntExit();
#endif
}





/**
  * @brief USART2中断服务函数（DMA接收）
  */
void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */
    if(__HAL_UART_GET_FLAG(&huart2, UART_FLAG_IDLE) != RESET)
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart2); // 清除IDLE标志
        HAL_UART_DMAStop(&huart2);          // 停止DMA
        UsartRxLen2 = USARTLEN_2 - __HAL_DMA_GET_COUNTER(&hdma_usart2_rx);
        HAL_UART_Receive_DMA(&huart2, (uint8_t *)UsartRxData2, USARTLEN_2); // 重启DMA接收
    }
  /* USER CODE END USART2_IRQn 0 */

  HAL_UART_IRQHandler(&huart2);
}

/**
  * @brief DMA1_Channel4_5_6_7中断服务函数（USART2_TX/RX）
  */
void DMA1_Channel4_5_6_7_IRQHandler(void)
{
  // 处理USART2_RX(Channel6)
  if(__HAL_DMA_GET_FLAG(&hdma_usart2_rx, DMA_FLAG_TC6 | DMA_FLAG_TE6))
    HAL_DMA_IRQHandler(&hdma_usart2_rx);
  // 处理USART2_TX(Channel4)
  if(__HAL_DMA_GET_FLAG(&hdma_usart2_tx, DMA_FLAG_TC4 | DMA_FLAG_TE4))
    HAL_DMA_IRQHandler(&hdma_usart2_tx);
}
/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
