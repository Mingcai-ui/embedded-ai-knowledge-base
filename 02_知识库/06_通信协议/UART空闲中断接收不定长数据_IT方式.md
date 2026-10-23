---
类型: 知识
状态: 进行中
标签: [stm32, uart, 中断, 串口, 空闲中断]
创建: 2026-09-15
---

# UART 空闲中断接收不定长数据（IT 方式）

> 一句话：用 **UART 空闲中断（IDLE）+ 逐字节中断（RXNE）** 接收不定长串口数据，无需 DMA，代码最简，低速率/小数据量完全够用。

## 1. 原理

串口总线空闲时接收线保持高电平。一帧数据（最后一位停止位）结束后，接收线空闲超过一个字节，USART 硬件置位 `USART_SR` 的 `IDLE` 标志，触发一次中断。

`HAL_UARTEx_ReceiveToIdle_IT()` 内部同时做两件事：

1. 使能 USART1 的 **RXNE 中断**：每到一个字节就进一次 `USART1_IRQHandler`，由 HAL 把字节存入 `rxData` 缓冲；
2. 使能 **IDLE 中断**：检测到线上空闲即判定"一包结束"，回调 `HAL_UARTEx_RxEventCallback(huart, Size)`。

和 DMA 方式共用同一个回调，`Size` 仍是本次实际收到的字节数，是**不定长接收**的关键。

## 2. 硬件与配置

- **芯片**：STM32F103C8T6（HAL 库，CubeMX 生成）
- **串口**：USART1，115200-8N1（8 数据位 / 1 停止位 / 无校验，过采样 16）
- **引脚**：PA9 = USART1_TX（复用推挽）、PA10 = USART1_RX（浮空输入）
- **NVIC**：只使能 `USART1_IRQn`（优先级 0），无需任何 DMA 通道。

`stm32f1xx_it.c` 里只有 `USART1_IRQHandler → HAL_UART_IRQHandler(&huart1)` 一个入口。

## 3. 核心代码

```c
/* main.c */
#define LENGTH 64            // 接收缓冲区长度
uint8_t rxData[LENGTH];      // 接收缓冲区

/* main() 初始化完成后只调用一次（USER CODE 2 区） */
HAL_UARTEx_ReceiveToIdle_IT(&huart1, rxData, LENGTH); // 启动 IT+空闲中断接收

/* printf 重定向到 USART1（单字符，阻塞发送） */
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* 接收完成回调：HAL 在空闲中断里算好 Size 后调过来（运行在中断上下文） */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART1) {
        printf("数据已接收,数据如下:\r\n");
        HAL_UART_Transmit_IT(&huart1, rxData, Size);        // 中断方式回显
        HAL_UARTEx_ReceiveToIdle_IT(&huart1, rxData, LENGTH); // 重新开启接收
    }
}
```

## 4. 优缺点

- **优点**：不用配 DMA，代码最简；所有初始化都在 `MX_USART1_UART_Init` 里完成，开箱即用。
- **缺点**：**每收一个字节就进一次 RXNE 中断**，CPU 频繁被打断；高速率 / 大数据量场景下开销和实时性不如 DMA 方式。

## 5. 排坑 / 注意

1. **回调里别用阻塞发送**：`__io_putchar` 是阻塞 `HAL_UART_Transmit`，本身又在中断上下文里回调，阻塞太久会卡住中断处理，还可能与 `HAL_UART_Transmit_IT` 回显抢串口发送资源（HAL 锁）。建议回调里只保留非阻塞回显。
2. **每次回调末尾必须重新 `HAL_UARTEx_ReceiveToIdle_IT`**，否则接收停在结束后。
3. **数据超过 `LENGTH(64)`** 会"满包"断开，超长帧被拆成多段，需应用层自行拼包。
4. **字节数用回调的 `Size`，不要 `strlen` 定长**，二进制数据里 0x00 会截断字符串判断。
5. **高波特率 + 主循环繁忙**时要留意中断优先级，避免漏字节（一般 F1 上 115200 无压力，更高波特率建议转 DMA 方式）。

## 相关

- [[UART空闲中断接收不定长数据_DMA方式]] — 同一需求的 DMA 实现与对比
- [[UART自定义帧协议_接收状态机与防误识别]] — 不定长数据到手后的帧解析/防误识别
- [[OLED显示_软件I2C驱动]] — 本工程附属 OLED 显示（PB8/PB9）
- 原始工程：`08_归档/IT中断空闲接收不定长数据/`