#include "UART_DMA.h"

/*
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rxData, LENGTH);
    main函数的接收DMA回调函数
*/

/*
	DMA的Mode配置
	Normal:	单次搬运, 结束后需重新调用回调函数
	Circular:	循环搬运, 搬运到缓冲区尾部回到头部继续搬运
*/
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) 
{
    if (huart->Instance == USART1) {
        printf("数据已接收,数据如下:\r\n");
        memcpy(txData, rxData, Size);
        txData[Size] = '\0';
        HAL_UART_Transmit_IT(&huart1, txData, Size);        // 中断方式回显
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rxData, LENGTH); // 重新开启接收
    }
}

/* printf 重定向到 USART1（单字符，阻塞发送） */
int _write(int file, char *ptr, int len)
{
  (void)file;
  HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 0xffff);
  return len;
}