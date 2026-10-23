---
类型: 知识
状态: 进行中
标签: [stm32, uart, dma, 中断, 串口, 空闲中断]
创建: 2026-09-15
---

# UART 空闲中断接收不定长数据（DMA 方式）

> 一句话：用 **UART 空闲中断（IDLE）+ DMA** 接收不定长串口数据，CPU 只在"一包发完"这一刻介入一次，其余字节由 DMA 后台搬运。

## 1. 原理

串口总线空闲时接收线保持高电平。当一帧数据（最后一位停止位）结束后，接收线回到空闲状态，且空闲时间超过一个字节，USART 硬件会置位 `USART_SR` 的 `IDLE` 标志；若使能了 `IDLEIE` 并开启 USART 中断，就触发一次中断。

`HAL_UARTEx_ReceiveToIdle_DMA()` 内部同时做了三件事：

1. 让 DMA 从 USART 数据寄存器持续搬到内存缓冲；
2. 使能 USART 的 **IDLE 中断**，用来感知"一包发完后的停顿"；
3. 检测到空闲后停止 DMA，算出本次实际收到的字节数，回调 `HAL_UARTEx_RxEventCallback(huart, Size)`。

`Size` 就是本次实际收到的字节数 —— 这是**不定长接收**的关键，它不依赖固定长度，而是靠线上空闲来判断一帧结束。

## 2. 硬件与配置

- **芯片**：STM32F103C8T6（HAL 库，CubeMX 生成）
- **串口**：USART1，115200-8N1（8 数据位 / 1 停止位 / 无校验，过采样 16）

| 引脚 | 功能 | 模式 |
|---|---|---|
| PA9 | USART1_TX | 复用推挽输出 `AF_PP` |
| PA10 | USART1_RX | 浮空输入 `INPUT` |

- **DMA 通道（F1 固定映射，不可乱改）**：

| 通道 | 方向 | 用途 |
|---|---|---|
| DMA1_Channel5 | 外设→内存 | USART1 接收 |
| DMA1_Channel4 | 内存→外设 | USART1 发送 |

- **NVIC（优先级均为 0）**：`USART1_IRQn`、`DMA1_Channel4_IRQn`、`DMA1_Channel5_IRQn`。

三者处理函数（`stm32f1xx_it.c`，CubeMX 生成）分别由 `HAL_UART_IRQHandler` / `HAL_DMA_IRQHandler` 接管。

## 3. 核心代码

```c
/* main.c */
#define LENGTH 64            // 接收缓冲区长度
uint8_t rxData[LENGTH];      // 接收缓冲区

/* main() 初始化完成后只调用一次（USER CODE 2 区） */
HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rxData, LENGTH); // 启动 DMA+空闲中断接收

/* printf 重定向到 USART1（阻塞发送，经新lib write() 系统桩） */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 0xffff);
    return len;
}

/* 接收完成回调：HAL 在空闲中断里算好 Size 后调过来（运行在中断上下文） */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART1) {
        printf("数据已接收,数据如下:\r\n");
        HAL_UART_Transmit_DMA(&huart1, rxData, Size);          // DMA 回显
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rxData, LENGTH); // 重新开启接收
    }
}
```

DMA 初始化要点（`usart.c` 的 `HAL_UART_MspInit`）：接收通道 `PERIPH_TO_MEMORY` + 外设不自增 + 内存自增 + 字节对齐 + `DMA_NORMAL` 模式；`__HAL_LINKDMA(uartHandle, hdmarx, hdma_usart1_rx)` 把 DMA 句柄挂到串口上。

## 4. 优缺点

- **优点**：字节搬运全部由 DMA 完成，CPU 只在每包结束时进一次回调，效率高，适合大数据量 / 高频收发。
- **缺点**：需要额外配置 DMA 通道；DMA NORMAL 模式下收满后必须重装（重新调用接收函数），否则第二次收不到。

## 5. 排坑 / 注意

1. **回调必须在中断上下文里，别用阻塞发送**：`printf`（`_write` 是阻塞 `HAL_UART_Transmit`）和 `HAL_UART_Transmit_DMA` 同抢 USART1 发送资源，可能发生 HAL 锁冲突。生产代码建议回调里只用非阻塞回显，去掉阻塞 printf。
2. **每次回调末尾必须重新 `HAL_UARTEx_ReceiveToIdle_DMA`**，不然接收停在结束后。
3. **数据超过 `LENGTH(64)`**：DMA NORMAL 收满即停、不覆盖，超长帧会被拆成多段，需在应用层自行拼包。
4. **DMA 通道别改**：F1 上 USART1 收发固定用 DMA1_Ch5 / Ch4，结错通道会搬错源。
5. **回调算出的 `Size` 直接可用**，不要用 `strlen(rxData)` 定长判断，收发二进制数据时尤甚。

## 相关

- [[UART空闲中断接收不定长数据_IT方式]] — 同一需求的纯中断实现与对比
- [[UART自定义帧协议_接收状态机与防误识别]] — 不定长数据到手后的帧解析/防误识别
- [[OLED显示_软件I2C驱动]] — 本工程附属 OLED 显示（PB8/PB9）
- 原始工程：`08_归档/DMA中断空闲中断接收不定长数据/`