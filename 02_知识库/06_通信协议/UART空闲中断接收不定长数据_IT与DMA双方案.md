---
类型: 知识
状态: 进行中
标签: [stm32, uart, 空闲中断, 中断, dma, 串口]
创建: 2026-09-15
---

# UART 空闲中断接收不定长数据（IT / DMA 双方案）

> 一句话：靠**空闲中断（IDLE）**判断"一包发完"，靠 `Size` 拿到实际长度——这就是不定长接收。字节搬运交给谁，决定了用 IT 还是 DMA。

> 本文由 `_IT方式` 与 `_DMA方式` 两篇合并（2026-10-23），两种实现并列对比。

## 1. 原理（两方案共用）

串口总线空闲时接收线保持高电平。一帧数据（最后一位停止位）结束后，接收线空闲超过一个字节，USART 硬件置位 `USART_SR` 的 `IDLE` 标志；若使能了 `IDLEIE`，就触发一次中断。

`HAL_UARTEx_ReceiveToIdle_xxx()` 系列函数内部都做了两件事：

1. 把线上字节搬进内存缓冲（**搬法不同**：IT 用 RXNE 中断逐字节搬，DMA 用 DMA 通道搬）；
2. 使能 **IDLE 中断**，检测到空闲后算出本次实际收到的字节数，回调 `HAL_UARTEx_RxEventCallback(huart, Size)`。

`Size` 就是本次实际收到的字节数 —— 这是**不定长接收**的关键，它不依赖固定长度，而是靠线上空闲判断一帧结束。两种方案**共用同一个回调函数**，上层解析代码完全不用改。

## 2. 怎么选

| 维度 | IT 方式 | DMA 方式 |
| --- | --- | --- |
| 启动函数 | `HAL_UARTEx_ReceiveToIdle_IT` | `HAL_UARTEx_ReceiveToIdle_DMA` |
| CPU 开销 | 每收 1 字节进 1 次中断 | 每收 1 包进 1 次回调 |
| CubeMX 配置 | 只开 USART1 全局中断 | 额外配 DMA 通道 |
| 适用场景 | 低速、小数据量（≤115200、偶发几条） | 高速、大数据量、连续流 |
| 代码复杂度 | 最简，开箱即用 | 多一层 DMA 配置与通道约束 |

**默认先用 IT**；发现主循环忙、丢字节、或波特率上到 115200 以上且数据连续时，换 DMA。

## 3. 硬件与配置

- **芯片**：STM32F103C8T6（HAL 库，CubeMX 生成）
- **串口**：USART1，115200-8N1（8 数据位 / 1 停止位 / 无校验，过采样 16）

| 引脚 | 功能 | 模式 |
|---|---|---|
| PA9 | USART1_TX | 复用推挽输出 `AF_PP` |
| PA10 | USART1_RX | 浮空输入 `INPUT` |

**IT 方式**：NVIC 只使能 `USART1_IRQn`（优先级 0），无需 DMA 通道。`stm32f1xx_it.c` 里只有 `USART1_IRQHandler → HAL_UART_IRQHandler(&huart1)` 一个入口。

**DMA 方式**：额外使能 `DMA1_Channel4_IRQn`、`DMA1_Channel5_IRQn`（优先级均 0）。F1 的 DMA 通道是**固定映射，不可乱改**：

| 通道 | 方向 | 用途 |
|---|---|---|
| DMA1_Channel5 | 外设→内存 | USART1 接收 |
| DMA1_Channel4 | 内存→外设 | USART1 发送 |

## 4. 方案 A：IT 方式

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

**优点**：不用配 DMA，代码最简；所有初始化都在 `MX_USART1_UART_Init` 里完成。

**缺点**：每收一个字节进一次 RXNE 中断，CPU 频繁被打断；高速率 / 大数据量下开销和实时性不如 DMA。

## 5. 方案 B：DMA 方式

```c
/* main.c */
#define LENGTH 64            // 接收缓冲区长度
uint8_t rxData[LENGTH];      // 接收缓冲区

/* main() 初始化完成后只调用一次（USER CODE 2 区） */
HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rxData, LENGTH); // 启动 DMA+空闲中断接收

/* printf 重定向到 USART1（阻塞发送，经 newlib write() 系统桩） */
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

**优点**：字节搬运全由 DMA 完成，CPU 只在每包结束进一次回调，适合大数据量 / 高频收发。

**缺点**：要额外配 DMA 通道；DMA NORMAL 模式下收满后必须重装（重新调用接收函数），否则第二次收不到。

## 6. 排坑 / 注意

### 6.1 两方案通用

1. **回调里别用阻塞发送**。`printf`（`_write` / `__io_putchar` 都是阻塞 `HAL_UART_Transmit`）本身在中断上下文里被调用，阻塞太久会卡住中断处理，还会和 `HAL_UART_Transmit_IT/DMA` 回显抢 USART1 发送资源（HAL 锁冲突）。生产代码只在回调里保留非阻塞回显。
2. **每次回调末尾必须重新调用接收函数**（`ReceiveToIdle_IT` / `ReceiveToIdle_DMA`），否则接收停在结束后，第二包永远进不来。
3. **数据超过 `LENGTH(64)`** 会断成多段：IT 是"满包"断开，DMA NORMAL 是收满即停不覆盖。超长帧都要在应用层自行拼包。
4. **字节数用回调的 `Size`，不要 `strlen()` 定长判断**——二进制数据里的 `0x00` 会被当字符串结束。

### 6.2 DMA 专属

5. **DMA 通道别改**：F1 上 USART1 收发固定 DMA1_Ch5 / Ch4，接错通道会搬错源。
6. **NORMAL 模式必须手动重装**，想要"永不停止"要改成 Circular 模式，但那样 `Size` 语义会变，需配合半传输中断判断。

### 6.3 IT 专属

7. **高波特率 + 主循环繁忙**时留意中断优先级，避免漏字节（F1 上 115200 一般无压力，更高建议直接转 DMA）。

## 相关

- [[UART自定义帧协议_接收状态机与防误识别]] — 不定长数据到手后的帧解析 / 防误识别
- [[环形缓冲区]] — 高频字节流的字节级缓冲，常与空闲中断接收配合
- [[FreeRTOS_任务管理与消息队列]] — 收包后用队列解耦，进一步降丢包
- [[OLED显示_软件I2C驱动]] — 本工程附属 OLED 显示（PB8/PB9）
- 原始工程：`08_归档/IT中断空闲接收不定长数据/`、`08_归档/DMA中断空闲中断接收不定长数据/`
