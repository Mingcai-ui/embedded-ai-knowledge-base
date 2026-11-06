#include "UART.h"

    /*    
    // 启动 IT+空闲中断接收
    // 单字节搬运,搬运一次中断一次
    需要在main函数中进行一次调用,开启中断
    //  HAL_UARTEx_ReceiveToIdle_IT(&huart1, rxData, LENGTH); 
    */

/* 接收完成回调：HAL 在空闲中断里算好 Size 后调过来（运行在中断上下文） */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) 
{
    if (huart->Instance == USART1) {
        printf("数据已接收,数据如下:\r\n");
        HAL_UART_Transmit_IT(&huart1, rxData, Size);        // 中断方式回显
        HAL_UARTEx_ReceiveToIdle_IT(&huart1, rxData, LENGTH); // 重新开启接收
    }
}

/* printf 重定向到 USART1（单字符，阻塞发送） */
int _write(int file, char *ptr, int len)
{
  (void)file;
  HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 0xffff);
  return len;
}