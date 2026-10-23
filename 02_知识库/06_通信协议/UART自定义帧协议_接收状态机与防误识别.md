---
类型: 知识
状态: 已完成
标签: [UART, 通信协议, 状态机, STM32, K230]
创建: 2026-08-22
---

# UART 自定义帧协议：接收状态机与防误识别

## 简介

两个芯片（如 K230 视觉模块 → STM32 主控）之间用 UART 传"一条结果"时，不能直接裸发数据——要定义**帧格式**：帧头 + 数据 + 校验 + 帧尾。接收端在单字节中断里跑**状态机**逐字节拼帧，配合**校验和**过滤误码，配合**连续帧确认**过滤误识别。

本项目实例：K230 识别扑克牌 → 发给 STM32 记录并显示（2026 校赛视觉小车）。

## 帧格式定义（7 字节）

| 字节 | 0 | 1 | 2 | 3 | 4 | 5 | 6 |
|---|---|---|---|---|---|---|---|
| 含义 | 帧头1 | 帧头2 | 花色 | 点数 | 校验和 | 帧尾1 | 帧尾2 |
| 取值 | 0xAA | 0x55 | 1~5 | 1~15 | 和校验 | 0x0D | 0x0A |

- 校验和 = `(帧头1 + 帧头2 + 数据1 + 数据2) & 0xFF`（累加和取低 8 位）
- 双帧头 `AA 55` 抗干扰：数据域出现 0xAA 也不会误触发

## 发送端（K230 / MicroPython）

```python
SEND_INTERVAL_MS = 200   # 两次发送最小间隔，防止淹没接收端
_last_send_time = 0

def send_card_info(label):
    global _last_send_time
    suit, rank = parse_card_label(label)
    if suit == 0 and rank == 0:
        return  # 无有效结果不发送
    # 发送限流：识别是逐帧跑的，但 UART 只需 200ms 一条
    now_ms = time.ticks_ms()
    if time.ticks_diff(now_ms, _last_send_time) < SEND_INTERVAL_MS:
        return
    _last_send_time = now_ms
    frame = bytearray([0xAA, 0x55, suit, rank, 0, 0x0D, 0x0A])
    frame[4] = (0xAA + 0x55 + suit + rank) & 0xFF   # 填校验和
    uart.write(frame)
```

**要点**：视觉循环 30fps 每帧都想发，但接收端处理能力有限——发送端主动限流比接收端丢弃更干净。

## 接收端（STM32 / HAL 库）

三件套：**单字节中断接收** → **状态机拼帧** → **校验通过才采纳**。

```c
#define UART_FRAME_LEN  7
uint8_t uart_rx_frame[UART_FRAME_LEN];
uint8_t uart_rx_fsm = 0;   // 0=等帧头, 1=收帧体
uint8_t uart_rx_cnt = 0;

// main() 初始化区：开启单字节中断接收
HAL_UART_Receive_IT(&huart3, &uart3_rx_byte, 1);

// 中断回调：每收到 1 字节进一次
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        uint8_t b = uart3_rx_byte;
        if (uart_rx_fsm == 0) {              // 状态0：找帧头 0xAA
            if (b == 0xAA) {
                uart_rx_frame[0] = b;
                uart_rx_cnt = 1;
                uart_rx_fsm = 1;
            }
        } else {                             // 状态1：收满 7 字节
            uart_rx_frame[uart_rx_cnt++] = b;
            if (uart_rx_cnt >= UART_FRAME_LEN) {
                if (uart_rx_frame[0]==0xAA && uart_rx_frame[1]==0x55) {
                    uint8_t sum = (0xAA+0x55+uart_rx_frame[2]+uart_rx_frame[3]) & 0xFF;
                    if (sum == uart_rx_frame[4]) {
                        // ===== 校验通过，处理数据 =====
                        OnFrameReceived(uart_rx_frame[2], uart_rx_frame[3]);
                    }
                }
                uart_rx_fsm = 0;             // 无论成败都回到找帧头
                uart_rx_cnt = 0;
            }
        }
        HAL_UART_Receive_IT(&huart3, &uart3_rx_byte, 1);  // 重新挂接收中断
    }
}
```

**要点**：
- 回调里**只做拼帧和校验**，业务处理尽量短（本例只写几个 volatile 变量），重活留给主循环。
- 中断回调末尾必须重新调用 `HAL_UART_Receive_IT`，否则只收到一个字节就停了。
- 状态机在"非帧头字节"上不会推进，天然跳过总线上杂散数据。

## 防误识别：连续帧确认 + 去重

校验和只能过滤**传输误码**，过滤不了**模型误识别**（模型把牌看错，帧本身是合法的）。两层过滤：

```c
#define CONFIRM_NEED 2          // 同一结果连续出现 N 帧才采纳
volatile uint8_t confirm_suit, confirm_rank, confirm_count;

void OnFrameReceived(uint8_t ns, uint8_t nr)
{
    if (confirm_suit == ns && confirm_rank == nr) confirm_count++;
    else { confirm_suit = ns; confirm_rank = nr; confirm_count = 1; }

    if (confirm_count >= CONFIRM_NEED) {
        // 去重：和上一张相同则跳过（同一张牌一直在视野内，只记一次）
        if (card_history_count == 0 ||
            card_history_suit[card_history_count-1] != ns ||
            card_history_rank[card_history_count-1] != nr) {
            card_history_suit[card_history_count] = ns;   // 记入历史
            card_history_rank[card_history_count] = nr;
            card_history_count++;
        }
        confirm_count = 0;    // 记录后清零，避免重复添加
    }
}
```

## 设计要点与已知坑

1. **帧头别用数据域可能出现的单字节**；双字节帧头 + 固定长度是最简单的方案（复杂场景用长度域 + CRC）。
2. **接收状态机没有超时复位**：若收到 `0xAA` 后剩余字节丢失，状态机会停在"收帧体"状态，直到凑满 7 字节才恢复。改进：加帧间超时（如 50ms 无后续字节则复位 fsm=0）。
3. **两侧参数一致**：波特率、帧格式、校验算法必须两端同步修改；本项目 K230 115200 ↔ STM32 USART3 115200。
4. **volatile**：中断写、主循环读的变量全部加 `volatile`。
5. 调试手段：两侧各加计数器（总接收字节数 / 有效帧数），快速判断"没收到"还是"收到了但校验失败"。

## 相关链接

- [[K230_nncase图像分类部署]]（发送端完整实现）
- [[2026校赛_视觉小车]]（项目实例）
- [[STM32_按键消抖_状态机]]（同款状态机思路）
