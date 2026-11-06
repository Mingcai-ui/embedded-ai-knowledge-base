#ifndef __UART_DMA_H__
#define __UART_DMA_H__

#include "main.h"
#include "usart.h"
#include "stdio.h"      //提供printf函数原型
#include "string.h"     //提供memcpy函数原型

#define LENGTH 64            // 接收缓冲区长度
uint8_t rxData[LENGTH];      // 接收缓冲区
uint8_t txData[LENGTH];      // 发送缓冲区

#endif
