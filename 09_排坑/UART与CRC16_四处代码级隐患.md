---
类型: 排坑
状态: 进行中
标签: [stm32, uart, crc16, 头文件, 类型溢出, 中断, 可移植]
创建: 2026-11-06
---

# UART 与 CRC16 模块归档时查出的四处代码级隐患

> 来源：2026-11-06 归档 `08_归档/可移植模块代码/UART/` 与 `CRC16/` 时对代码的静态审查。
> **四处都还没在实机上触发**——属于"当前能跑，但换个用法就炸"的类型，先记下来当检查清单。

## 环境
- 芯片 / 板卡：STM32F103C8T6
- 工具链 / 版本：CubeMX + HAL 库，ARM GCC（EIDE / VS Code）

---

## 隐患 1：头文件里定义变量，同时包含必重复定义

### 现象
编译或链接报 `multiple definition of 'rxData'` / `redefinition of 'LENGTH'`。
典型触发场景：某个 `.c` 同时 `#include "UART.h"` 和 `#include "UART_DMA.h"`（比如以后想把 IT 版和 DMA 版放进同一个工程做对比演示）。

### 原因
两个头文件都这样写：

```c
/* UART.h */
#define LENGTH 64
uint8_t rxData[LENGTH];      // ← 这是「定义」，不是「声明」

/* UART_DMA.h */
#define LENGTH 64            // ← 与 UART.h 同名同值的宏
uint8_t rxData[LENGTH];      // ← 同名同类型的数组定义
uint8_t txData[LENGTH];
```

三条问题叠在一起：

1. **头文件里放数组定义** = 每个包含它的 `.c` 各生成一份实体（`uint8_t rxData[64]` 是 tentative definition），多文件包含后链接器看到多个同名符号 → 报 `multiple definition`。
2. **`LENGTH` 这个宏名太泛**，两个模块都用，同时包含时虽然值相同不报错，但一旦哪天想把接收缓冲改成 128（DMA 版）两个宏就会打架，报 `macro redefined`。
3. **`rxData` 只是"恰好现在不冲突"**：两个头文件都叫 `rxData`，如果哪天要给 DMA 版单独开缓冲，rename 时极易漏改。

### 解决
头文件只放**声明**，定义挪到 `.c`：

```c
/* UART.h —— 只声明 */
#define UART_RX_BUF_SIZE 64
extern uint8_t rxData[UART_RX_BUF_SIZE];

/* UART.c —— 唯一定义点 */
uint8_t rxData[UART_RX_BUF_SIZE];
```

要点：
- 数组/变量在头文件里一律加 `extern`，`static` 也不行（`static` 会变成"每个 .c 一份私有副本"，各改各的，比重复定义更难查）。
- 宏名加模块前缀（`UART_RX_BUF_SIZE`），别用裸 `LENGTH`。
- 两个模块要用同一块缓冲就共享一个 `extern`；要各自独立就分名（`uartRx` / `dmaRx`）。

---

## 隐患 2：CRC16 里 `uint8_t` 循环变量配 `uint16_t` 长度 → 超 255 字节死循环

### 现象
传入 `len > 255` 时函数**永不返回**，程序卡死在 `CRC16()` 里（表现为主循环停摆、看门狗复位）。`len ≤ 255` 时一切正常，所以自测很容易漏掉。

### 原因
```c
uint16_t CRC16(uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xffff;
    uint8_t i, j;          // ← i 只有 8 位，上限 255
    for (i = 0; i < len; i++)   // len 是 16 位
    {
        ...
    }
}
```

逐层拆开看：

| 步骤 | 发生什么 |
|---|---|
| `i < len` 比较 | `i` 被整型提升为 `int`，值仍是 0~255，比较正确 |
| `i` 递增到 255 | 再 `i++` → **无符号回绕成 0** |
| 回到判断 `i < len` | `0 < 300` 成立 → 继续循环 |
| 结果 | `i` 永远跑不到 300，**无限循环** |

C 标准的整数提升（integer promotion）只提升「比 int 窄」的类型参与运算时的类型，**不会阻止赋值回 `uint8_t` 时的截断**。这是 `uint8_t` 循环变量最典型的坑。

### 解决
循环变量类型必须 **≥ 长度参数类型**：

```c
uint16_t i;      // 与 len 同宽
uint8_t  j;      // j 只跑 0~7，用 uint8_t 完全安全
```

顺手做两件事，防这类问题复发：
- **调用方约束**：Modbus RTU 单帧上限 256 字节，接收层应在调用前就拒绝超长帧，而不是指望 CRC 函数兜底。
- **编译器帮忙**：开 `-Wconversion` / `-Wsign-compare`，GCC 对 `uint8_t` 与 `uint16_t` 比较会给出提示（默认 `-Wall` 不报这条）。

> 这条在 Modbus 场景里几乎不会踩到（帧最长 256），但它属于**"参数放宽一点就炸"**的 latent bug——今天传 6 字节没事，明天拿这个函数去校验 W25Q64 的一大块缓存就卡死。函数既然收 `uint16_t len`，就得对 16 位负责。

---

## 隐患 3：中断回调里调 `printf` / `_write`，阻塞发送抢 USART 锁

### 现象
- 长时间连续收包后，偶发**收不到数据**、或回显内容错乱/粘连；
- 主循环明显变慢，高波特率下更明显；
- 极端情况下串口发送彻底卡住不再恢复。

### 原因
`HAL_UARTEx_RxEventCallback()` **运行在中断上下文**，而回调第一句就是 `printf`：

```c
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1) {
        printf("数据已接收,数据如下:\r\n");        // ① 阻塞发送
        HAL_UART_Transmit_IT(&huart1, txData, Size); // ② 非阻塞发送
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rxData, LENGTH);
    }
}
```

而 `printf` 最终走到重定向的 `_write`，里面是**阻塞**的 `HAL_UART_Transmit(..., 0xffff)`：

```c
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 0xffff);  // 阻塞，最长等 65535ms
}
```

两处冲突：

1. **阻塞 vs 中断**：`HAL_UART_Transmit` 是轮询等待 `TXE` 的忙等，在中断里忙等会长时间占住中断，压低整个系统的实时性。一帧 13 个 ASCII 字符 @115200 ≈ 1.1ms，看着不长，但它发生在**每次接收回调**里，高频收包时是纯浪费。
2. **HAL 锁竞争**：HAL 串口句柄有 `gState` 状态机。`HAL_UART_Transmit`（阻塞）会把 `gState` 置为 `BUSY_TX`，此时再调 `HAL_UART_Transmit_IT` 会因为 `gState != READY` **直接返回 `HAL_BUSY`**——回显悄悄失败，没有任何报错。这就是"有时候回显没了"的根因。

### 解决
**中断里只做搬运，不做等待**。三种改法按侵入性从低到高：

1. **精简**：回调里只保留 `Transmit_IT` / `Transmit_DMA`，把 `printf` 挪到主循环（先置标志位，主循环看到再打印）。
2. **分级**：调试期保留 `printf`，但用宏 `#define UART_DEBUG 0` 关掉，别让调试代码进生产。
3. **解耦（最终形态）**：回调里只把数据和长度塞进**环形缓冲区 / FreeRTOS 消息队列**，发送完全交给主循环或独立任务。这也是 [[环形缓冲区]] + [[FreeRTOS_任务管理与消息队列]] 在这个工程里的意义。

> 附带一条判定技巧：**看 `Transmit_IT` 的返回值**。它返回 `HAL_BUSY` 而不是 `HAL_OK`，就说明有别的发送在占着句柄——回调里回显莫名丢失时，第一步先查这个返回值。

---

## 隐患 4：`txData[Size] = '\0'` 在收满缓冲区时越界一字节

### 现象
嵌在 `HAL_UARTEx_RxEventCallback` 里的这两行：

```c
memcpy(txData, rxData, Size);
txData[Size] = '\0';        // ← Size 可能等于 LENGTH
```

`Size == LENGTH`（正好收满 64 字节）时，写入的是 `txData[64]`——**数组只有 0~63**。

后果很隐蔽，不会立刻崩：
- 踩到的是紧邻 `txData` 后面的那个变量（链接顺序决定是谁），表现为某个无关全局量**莫名其妙变成 0**，或者 `LENGTH` 之类被改写；
- 每次只踩 1 字节、只写 `0x00`，用调试器单步往往看不出来，只在大包连发时偶发；
- 如果 `txData` 恰好被链接到 RAM 末尾之外，才会 hardfault——但多数情况下只是"数据偶尔不对"。

### 原因
`HAL_UARTEx_RxEventCallback(huart, Size)` 的 `Size` 取值区间是 **`0 ~ 缓冲区长度`（左闭右闭）**。

这是它的语义决定的：`Size` 表示"这一包实际装了多少字节"，收满时它就是缓冲区的满容量。所以：

| `Size` | `txData[Size]` 是否越界 |
|---|---|
| 0 ~ 63 | 否，合法写入 |
| **64（= `LENGTH`）** | **是，写到数组外 1 字节** |

### 根本问题：为什么要加 `'\0'`？

发送用的是 `HAL_UART_Transmit_IT(&huart1, txData, Size)`——**已经显式给了长度**，HAL 根本不看内容里有没有 `'\0'`。

加 `'\0'` 的唯一动机是"万一后面要当字符串 `printf`"，属于**字符串思维残留**。串口收发的是**二进制字节流**：

- 数据里本来就可能含 `0x00`（Modbus 帧到处都是），补 `'\0'` 反而让 `printf("%s")` 提前截断；
- 长度信息已经由 `Size` 携带，不需要 sentinel 终结符。

### 解决
按推荐度排序：

1. **直接删掉这一行**（推荐）。用长度发送就不需要终结符。
2. **真需要 `'\0'` 就给数组留位**：
   ```c
   uint8_t txData[UART_RX_BUF_SIZE + 1];   // 多留 1 字节给终结符
   ```
3. **加边界判断**（治标）：
   ```c
   if (Size < LENGTH) txData[Size] = '\0';
   ```
   这样写不会崩，但等于"知道有越界风险还留着"，不如直接删。

> **通用判据**：凡是「按长度处理二进制数据」的地方，`'\0'` 都是多余的；凡是「按字符串处理文本」的地方，长度里就必须把这 1 字节算进去。混着用就一定会踩线——这也是第 6.1 节"字节数用 `Size`，不要 `strlen()`"的同一条原则。

---

## 相关链接

- [[UART空闲中断接收不定长数据_IT与DMA双方案]] — 两方案的完整说明与通用注意事项
- [[CRC16_介绍]] — CRC16 模块的算法参数与字节序
- [[进度_CRC16_Modbus_完整记录]] — CRC 手算过程
- [[环形缓冲区]] — 隐患 3 的最终解法
- 代码位置：`08_归档/可移植模块代码/UART/`、`08_归档/可移植模块代码/CRC16/`
