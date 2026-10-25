---
类型: 知识
状态: 进行中
标签: [stm32, hal, 定时器, 中断, msp, nvic]
创建: 2026-10-25
---

# STM32 定时器中断_Msp与中断链路

> **定位**：裸机 HAL 知识（CubeMX 工程通用，不是 FreeRTOS 专属）。中断与 RTOS 调度器的交互（BASEPRI / 临界区 / PendSV）在 [[FreeRTOS_中断管理与临界区]]。
> **平台**：STM32F103C8T6 / HAL / CubeMX 生成 + VS Code + EIDE
> **来源**：UART_Task 工程 2026-10-24~25 实测蒸馏（过程见 [[FreeRTOS_学习进度_2026-10-24]]）；对应排坑见 [[FreeRTOS_阶段一故障合集]] 第 7~8 坑。
> **能力档位：半懂** —— 三真相源不一致是自己排查修的；"为什么 Init 不够"、"为什么高级定时器不要 MOE"是被告知的，未做对照实验。

## 1. TIM 中断链路：从配置到触发（缺的那一步）

`MX_TIM1_Init()` 只写寄存器配置（PSC / ARR / NVIC），**不置 CR1.CEN，也不置 DIER.UIE**。

中断真正生效需要三件事同时成立：

| 条件 | 谁来置 | CubeMX 代码里有吗 |
| --- | --- | --- |
| NVIC 通道使能 | `HAL_NVIC_EnableIRQ()` | ✅ `HAL_TIM_Base_MspInit` 里有 |
| 外设中断使能位 DIER | `__HAL_TIM_ENABLE_IT()` | ❌ 没有 |
| 计数器启动 CR1.CEN | `__HAL_TIM_ENABLE()` | ❌ 没有 |

所以必须补一行 `HAL_TIM_Base_Start_IT(&htim1);`。缺这一步的表现是：ISR 写对了、`extern` 写对了、回调写对了，但**中断永远不触发**。

补充两条：

- TIM1 是高级定时器，做**更新中断**不需要 MOE（`__HAL_TIM_ENABLE_MOE`），只有输出比较 / PWM 才要。
- 周期公式：`T = (PSC+1) × (ARR+1) / 72MHz`。PSC=7199、ARR=9999 → 1s。ARR=65535 时是 6.55s，看着像"没反应"。

## 2. `HAL_TIM_Base_MspInit` / `MspDeInit` 与 `MX_TIM1_Init` 的分工

```c
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_base_handle)
{
  if(tim_base_handle->Instance==TIM1)
  {
    __HAL_RCC_TIM1_CLK_ENABLE();              // 外设时钟
    HAL_NVIC_SetPriority(TIM1_UP_IRQn, 5, 0); // 抢占优先级 5，子优先级 0
    HAL_NVIC_EnableIRQ(TIM1_UP_IRQn);         // 打开 NVIC 通道
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_base_handle)
{
  if(tim_base_handle->Instance==TIM1)
  {
    __HAL_RCC_TIM1_CLK_DISABLE();             // 反初始化时关时钟（省电）
    HAL_NVIC_DisableIRQ(TIM1_UP_IRQn);        // 关 NVIC 通道
  }
}
```

| 函数 | 管什么 | 谁调用 |
| --- | --- | --- |
| `MX_TIM1_Init()` | PSC / ARR / 计数模式 / 触发源（**定时器本体**） | `main()` 显式调用 |
| `HAL_TIM_Base_MspInit()` | 时钟使能、NVIC 优先级与使能、DMA（**板级资源**） | HAL 在 `HAL_TIM_Base_Init()` 内部回调，看不到显式调用 |
| `HAL_TIM_Base_MspDeInit()` | 关时钟、关 NVIC | `HAL_TIM_Base_DeInit()` 时回调 |

三个要点：

1. **`Msp` = MCU Support Package**。CubeMX 把"芯片无关的配置"和"芯片相关/板级相关"分开，Msp 是后者。
2. **NVIC 使能在这里，不在 `MX_TIM1_Init` 里**。所谓"中断配置好了"，其实是 MspInit 干的。
3. **按实例分流**（`if(tim_base_handle->Instance==TIM1)`），所以一个文件能同时服务 TIM1/TIM2/TIM3，加定时器不会互相覆盖。

⚠️ 实践提醒：如果在 `USER CODE` 区自己重写一份 `HAL_TIM_Base_MspInit`，会和 CubeMX 生成的那份冲突（重复定义或只留自己的那份），**时钟和 NVIC 会全部失效**——表现和"忘了 Start_IT"一模一样。Msp 区一般不要碰。

## 3. 正确的中断链路只有一条（不要自己加第三层）

```
启动：   HAL_TIM_Base_Start_IT(&htim1)
           └→ __HAL_TIM_ENABLE_IT(IT_UPDATE)  +  __HAL_TIM_ENABLE(CEN=1)

中断：   TIM1_UP_IRQHandler()                 [stm32f1xx_it.c，CubeMX 管的]
           └→ HAL_TIM_IRQHandler(&htim1)      [HAL 库，清标志 + 分发]
                └→ HAL_TIM_PeriodElapsedCallback(htim)   [main.c，你写业务]
```

判断标准就一条：**业务代码只应出现在 `HAL_TIM_PeriodElapsedCallback` 里**。ISR 入口和分发都在库和 CubeMX 手里。

教程陷阱（已踩过）：正点原子视频里的 `HAL_TIM_INT_IRQHandle()` 封装，在他的纯手写工程里成立（`stm32f1xx_it.c` 由他自己维护，调的是这个自造函数）；**在 CubeMX 工程里是废码**——`it.c` 已经 `TIM1_UP_IRQHandler() → HAL_TIM_IRQHandler(&htim1)` 直连，再补一层没有调用者。`HAL_TIM_INT_IRQHandle` 也不是 HAL 官方名字（官方只有带参数的 `HAL_TIM_IRQHandler`）。**教训：照抄教程代码前，先确认"教程的 `it.c` 是谁维护的"。**

## 相关链接

- [[FreeRTOS_中断管理与临界区]] — 中断与 FreeRTOS 调度器的交互（BASEPRI / 临界区 / PendSV）
- [[FreeRTOS_阶段一故障合集]] — 第 7~8 坑（本篇原理对应的真实故障）
- [[FreeRTOS_学习进度_2026-10-24]] — 本轮过程记录与能力判定
- [[STM32F103C8T6-芯片资源与PinMux速查]] — 芯片资源与引脚
