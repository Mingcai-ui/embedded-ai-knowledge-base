---
类型: 知识
状态: 进行中
标签: [freertos, stm32, 中断管理, 临界区, basepri, pendsv, nvic]
创建: 2026-10-25
---

# FreeRTOS 中断管理与临界区

> **平台**：STM32F103C8T6 / HAL / FreeRTOS 10.3.1（CMSIS-OS2）/ TIM4 作 HAL 时基 / VS Code + EIDE
> **来源**：[[FreeRTOS_学习进度_2026-10-24]] 本轮（UART_Task 工程，2026-10-24~25 实测 + 十六题答辩校准）蒸馏；对应排坑见 [[FreeRTOS_阶段一故障合集]] 第 7~10 坑。
> **能力档位：半懂** —— 机理已成文，部分结论经 AI 纠正 / 答辩校准；对照实验（实验 C~F）尚未做，边界未实测。

## 1. TIM 中断链路（摘要，全文已迁出）

> 全文（三要素 / MspInit 分工 / 唯一正确链路）已迁到裸机 HAL 笔记 [[STM32_定时器中断_Msp与中断链路]]——那部分属裸机 HAL 范畴，不是 FreeRTOS 专属。此处只留与本篇最相关的两条结论：

- 中断真正生效要三件事同时成立：NVIC 通道使能（MspInit 干的）+ DIER.UIE + CR1.CEN —— 后两者靠 `HAL_TIM_Base_Start_IT(&htim1)`，缺它**中断永远不触发**（第 7 坑）。
- CubeMX 工程的中断入口已直连 `TIM1_UP_IRQHandler() → HAL_TIM_IRQHandler() → HAL_TIM_PeriodElapsedCallback()`，不要自己加第三层（`HAL_TIM_INT_IRQHandle` 式封装在 CubeMX 工程里是废码）。

## 2. "关中断"有四个层级，作用范围完全不同

| 层级 | 函数 | 影响范围 | 可嵌套 |
| --- | --- | --- | --- |
| 全局 PRIMASK | `__disable_irq()` / `__enable_irq()` | 全部可屏蔽中断，含 prio 0 | ❌ |
| NVIC 通道 | `HAL_NVIC_DisableIRQ()` / `EnableIRQ()` | 单条中断线（标志位仍会置） | — |
| 外设使能位 | `__HAL_TIM_DISABLE_IT()` / `HAL_TIM_Base_Stop_IT()` | 单个外设的某类事件 | — |
| RTOS 临界区 | `taskENTER_CRITICAL()` / `EXIT` | prio ≥ configMAX_SYSCALL(5) | ✅ |

注意 `osKernelLock()` **不是关中断**，它只锁调度器。

## 3. `portDISABLE_INTERRUPTS()` 的真实语义（BASEPRI）

本工程实际展开（`portmacro.h:101`）：

```c
#define portDISABLE_INTERRUPTS()   vPortRaiseBASEPRI()   // msr basepri, 0x50
#define portENABLE_INTERRUPTS()    vPortSetBASEPRI(0)    // msr basepri, 0
```

`0x50 = 5 << 4`，来自 `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`。

三条关键认知：

1. **不是"全关"，是按优先级屏蔽**：prio 5~15 被挡，prio 0~4 照跑。
2. **`portENABLE_INTERRUPTS()` 无条件写 0**，不保存旧值、无嵌套计数。嵌套两次 disable 只 enable 一次，会把中断全开，破坏内核自己的临界区。准确说法：嵌套时第二次关闭基本等于没运行，第一次恢复就完全恢复——"只能调用一次"的实质是"第二次不起作用，且会让第一次失效"。
3. **`taskENTER_CRITICAL()` = 同一动作 + `uxCriticalNesting` 计数**，所以应用代码一律用它。

## 4. `HAL_Delay` 在关中断下会永久死锁

错误写法：

```c
portDISABLE_INTERRUPTS();   // basepri = 0x50
HAL_Delay(5000);            // ← 死在这里
portENABLE_INTERRUPTS();    // ← 永远执行不到
```

因果链：

```
HAL_Delay 等 uwTick 变量
  → uwTick 由 HAL_IncTick() 在 TIM4_IRQHandler 里递增
  → TIM4 优先级 15，被 BASEPRI=0x50 屏蔽
  → uwTick 永久冻结，while 差值恒为 0
  → 死循环，ENABLE 永不执行
```

**为什么正点原子同样的代码能跑**：他的 `delay_ms` → `delay_us` 是**轮询 `SysTick->VAL` 寄存器**。

一句话区分（本篇记忆锚点）：

> **"等变量"依赖中断，"查硬件计数器"不依赖。** `HAL_Delay` 等的是**变量**（`uwTick`，靠中断喂），`delay_us` 查的是**硬件计数器**（`SysTick->VAL`，关中断照跑）。SysTick 的计数器在关中断时依然递减，只是它的中断响应被屏蔽。

修复方案：自写 `delay_us_noint()` 轮询 `SysTick->VAL`（含递减回绕补偿），已在 UART_Task 工程实现并编译烧录实测。

⚠️ 但更正确的原则是：**临界区里根本不该有任何延时** —— `osDelay`、`HAL_Delay`、自己写的轮询 Delay 一律不行。轮询 Delay 不死锁，却把关中断时间拉长到毫秒级，比"一眼看出来卡死"更隐蔽地破坏实时性。临界区的唯一任务是保护几条指令的原子性。（正点原子"临界区只能用自己写的 Delay"的说法危险，已被纠正。）

顺带区分（答辩纠正）：`HAL_Delay` 与 `osDelay` 失效**表现相似、机理相反**——前者忙等变量，关中断后 CPU 空转跑飞；后者阻塞任务等 tick，关中断后任务静默卡在阻塞态、让出了 CPU。排查时一个看"CPU 在跑什么"，一个看"任务在哪个状态"。

## 5. 任务调度同样被掐断，而且是两级

| 层级 | 异常 | 优先级 | 被屏蔽后 |
| --- | --- | --- | --- |
| ① 请求 | SysTick | 15 | `xTaskIncrementTick()` 不跑，`xTickCount` 不增 |
| ② 执行 | PendSV | 15 | 即使挂起位已置，异常不响应，不切栈 |

**PendSV 为什么存在**：可挂起（写 `ICSR.PENDSVSET` 即请求，不必立刻执行）+ 必须最低优先级，保证所有中断处理完才切栈。若在 SysTick 里直接切，遇到嵌套中断时会切在错误的栈帧上。

一条可实测的细节：临界区内 `taskYIELD()` 只置 `ICSR.PENDSVSET`，任务**不会**当场切换，要到 `portENABLE_INTERRUPTS()` 执行完的那一瞬间才切。（实验 F：给 `xPortPendSVHandler` 下断点验证，尚未做。）

由此推出两条铁律：

1. **临界区禁止一切阻塞 API**（`osDelay` / `osMessageQueueGet` / `osMutexAcquire`）。它们靠 PendSV 把任务切走，PendSV 被屏蔽 → 函数返回后"已阻塞"的任务继续跑 → 链表错乱。**这类 bug 不会当场报错，事后才崩，最难查。**
2. **`FromISR` 的 prio ≥ 5 限制和 BASEPRI 是同一套机制**。prio 0~4 里调内核 API，内核想用 BASEPRI 保护自己也保护不住。

## 6. 临界区函数组

| 函数对 | 计数 | 用途 |
| --- | --- | --- |
| `portDISABLE_INTERRUPTS()` / `portENABLE_INTERRUPTS()` | ❌ 无 | 内核内部使用 |
| `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` | ✅ `uxCriticalNesting` | **应用代码一律用这对** |

三个易错点（2026-10-24 十六题答辩中暴露并纠正）：

1. `vPortExitCritical()` 里是 `uxCriticalNesting--`（**递减**），计数归零才真正开中断。
2. `portSET_INTERRUPT_MASK_FROM_ISR()` 返回的是**旧 BASEPRI 值**（供之后恢复用），不是 0/1 开关状态。
3. `configASSERT( (portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK) == 0 )` 检查的是 `ICSR.VECTACTIVE` —— 判断当前是否在中断上下文（任务级临界区不许在 ISR 里用）。

**未整理（缺口）**：`FromISR` 那对函数（`portSET/CLEAR_INTERRUPT_MASK_FROM_ISR` 的保存-恢复用法、`xxxFromISR` API 族）完全空白，见 [[FreeRTOS_学习进度_2026-10-24]] 未验证清单。

## 7. 调度器挂起 / 恢复（任务C 前置，半懂）

- `vTaskSuspendAll()` / `xTaskResumeAll()`：**只锁调度、不关中断**，与临界区是两套机制。
- 自评（2026-10-24 看完正点原子视频后）："还不是很理解"，待任务C 时回头补齐。

## 8. CubeMX 的 `Uses FreeRTOS functions` 勾选项

- 在 CubeMX NVIC 页，**每个中断通道独立**一个复选框（不是"定时器默认开启"）。
- 勾选（默认）：该中断优先级被限制在 ≥ `configMAX_SYSCALL_INTERRUPT_PRIORITY`（5），ISR 里可调内核 API。
- 取消勾选：优先级可设 0~4（高于内核保护线），代价是**该 ISR 里绝不能调任何 FreeRTOS API**。
- 实验 E（尚未做）：取消 `TIM1_UP_IRQn` 勾选、优先级 5 → 4 重新生成，冻结期看 `BASEPRI = 0x50` 且 LED 仍在闪（prio 4 < 5 没被挡）。

## 相关链接

- [[STM32_定时器中断_Msp与中断链路]] — 裸机 HAL：TIM 中断链路与 MspInit 分工（本篇 §1 全文迁出处）
- [[FreeRTOS_索引]] — 目录入口
- [[FreeRTOS_学习进度_2026-10-24]] — 本轮能力判定、答辩记录、下一步
- [[FreeRTOS_阶段一故障合集]] — 第 7~10 坑（本轮四个真实故障）
- [[EIDE_JLink_调试环境搭建]] — 调试器环境与使用技巧
- [[FreeRTOS_任务管理与消息队列]] — 前一篇原理笔记（任务 / 队列）
- [[CMSIS-RTOS2函数速查表-STM32CubeMX版]] — API 签名与返回值
