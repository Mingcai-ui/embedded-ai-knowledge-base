---
类型: 知识
状态: 已完成
标签: [stm32, 编码器, 正交解码, 定时器, 中断]
创建: 2026-08-22
---

# STM32 编码器软件正交解码（查表法 4 倍频）

## 简介

增量编码器输出 A/B 两相方波，相位差 90°。**查表法软件解码**：定时器高频中断里采样 A/B 电平，用 16 项查找表判断本次变化对应 +1/-1/0 脉冲。相比 TIM 硬件编码器模式，软件解码不占用编码器接口定时器、引脚任意选，但占用 CPU 中断时间。

## 原理：4 倍频查表

把「上次 AB 状态 + 本次 AB 状态」拼成 4bit 索引（`(last_AB<<2) | (A<<1) | B`），查表得到脉冲增量。表中同时包含非法跳变（如 00→11，抖动/漏采）计 0：

```c
static const int8_t enc_table[] = {   // 正交解码查找表（4倍频）
//  last:00  01  10  11
    0, -1,  1, 0,    // 当前:00
    1,  0,  0, -1,   // 当前:01
   -1,  0,  0, 1,    // 当前:10
    0,  1, -1, 0     // 当前:11
};
```

一个完整周期（A、B 各一个方波）计 4 个脉冲，分辨率 ×4。

## 实现（本项目实例）

### 1. 初始化：编码器引脚 + 采样定时器（Encoder_Init，放 main() 初始化区）

```c
TIM_HandleTypeDef htim3;

void Encoder_Init(void)
{
    // 1. A/B 相引脚配置为上拉输入（本例 PB14=A，PB15=B）
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 2. TIM3 定时中断：72MHz/72 = 1MHz，Period=100 → 100μs 中断（10kHz 采样）
    __HAL_RCC_TIM3_CLK_ENABLE();
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 72 - 1;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 100 - 1;
    HAL_TIM_Base_Init(&htim3);
    HAL_NVIC_SetPriority(TIM3_IRQn, 2, 0);   // 优先级低于控制周期定时器
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
    HAL_TIM_Base_Start_IT(&htim3);
}
```

### 2. 中断里采样解码（回调函数）

```c
volatile int32_t EncoderCount = 0;      // 总脉冲累计（正转+，反转-）
static uint8_t last_AB = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        uint16_t idr = GPIOB->IDR;              // 直接读寄存器，比 HAL_GPIO_ReadPin 快
        uint8_t A = (idr & GPIO_PIN_15) ? 1 : 0;
        uint8_t B = (idr & GPIO_PIN_14) ? 1 : 0;
        uint8_t idx = (last_AB << 2) | (A << 1) | B;
        EncoderCount += enc_table[idx];
        last_AB = (A << 1) | B;
        return;
    }
}
```

### 3. 控制周期里取增量算速度/里程（TIM2 40ms 中断里）

```c
static int32_t LastEncoderCount = 0;

int16_t Encoder_GetDelta(void)          // 取自上次调用以来的脉冲增量
{
    int32_t curr = EncoderCount;
    int16_t delta = (int16_t)(curr - LastEncoderCount);
    LastEncoderCount = curr;
    return delta;
}

// 40ms 控制周期中断内：
EncoderDelta = Encoder_GetDelta();          // 速度 ∝ EncoderDelta
TotalEncoderPulse += EncoderDelta;          // 里程（累计脉冲）
WheelDistance = (float)TotalEncoderPulse / ENCODER_PPR;   // 换算圈数
```

## 关键参数标定

**ENCODER_PPR（一圈脉冲数）必须实测**：让轮子手动转一整圈，读 `EncoderCount` 变化量。本项目基础版取 5500、最终版标定为 25160（换了电机/减速比后重标）。PPR 不准则圈数、里程、速度全部失真。

**采样率 vs 最高转速**（奈奎斯特约束）：4 倍频后最高脉冲频率 = 4 × PPR × 转速(rps)。以 PPR=25160、10kHz 采样为例，采样率必须 ≥ 2× 脉冲频率 → 最高约 0.2 圈/秒。**转速高或 PPR 大时必须提高采样率或改用硬件编码器模式**，否则丢脉冲、表跳变计 0，里程偏少。

## 软件解码 vs 硬件编码器模式

| | 软件查表（本笔记） | TIM 硬件编码器模式 |
|---|---|---|
| 引脚 | 任意 GPIO | 必须是定时器 CH1/CH2 对应脚 |
| CPU 占用 | 每次中断都跑（10kHz） | 零（硬件计数） |
| 最高转速 | 受采样率限制 | 高（硬件计数无漏采） |
| 配置 | 手写查表逻辑 | `HAL_TIM_Encoder_Init` 几行搞定 |

**结论**：低速小车够用且灵活；转速高、PPR 大、或编码器数量多时用硬件模式。

## 已知坑

1. 本项目每轮只用**一个**编码器（测里程/圈数），没做双轮差速测速——速度环实际没闭环（PID 只闭了循迹位置环）。
2. `EncoderCount` 中断写、主循环读，必须 `volatile`；32 位变量在 Cortex-M3 上读写原子，无需关中断保护。
3. `last_AB` 等状态变量是 `static`，多编码器时每路要独立一套。

## 相关链接

- [[灰度循迹_PID差速控制]]（里程用于停车控制）
- [[2026校赛_视觉小车]]（项目实例）
- [[闭环编码电机_回零后机械卡死]]（编码器回零相关坑）
