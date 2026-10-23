---
类型: 代码
状态: 归档
标签: [stm32, 蜂鸣器, gpio, 江协科技, 旧版]
模块: Buzzer
硬件平台: STM32F103C8T6
版本: v1.0
创建: 2026-08-20
---

> ⚠️ 本文件为 2026-08-20 旧版。同主题现行笔记为 [[STM32_GPIO输出_LED与蜂鸣器]]（知识库，已合并 LED + 蜂鸣器）与 [[Buzzer_介绍]]（可移植模块代码配套文档）。此处仅作历史留档，**不要在此更新**。

# 代码介绍：Buzzer 模块

> **模块类型**：Buzzer（蜂鸣器，数字输出设备）
> **所属节号**：1-2 按键与LED闪烁与蜂鸣器
> **版本**：v1.0
> **硬件平台**：STM32F103C8T6
> **最后更新**：2026-08-20

## 1. 模块概述

**一句话功能**：控制蜂鸣器的开关、翻转和非阻塞间歇鸣响。

**设计模式**：非阻塞延时（与 LED 模块结构完全一致）

**复杂度**：★★☆☆☆

## 2. 硬件信息

**引脚连接**：

| 设备 | 端口 | 引脚 | 模式 | 上下拉 | 默认电平 | 有效电平 |
|---|---|---|---|---|---|---|
| Buzzer | GPIOA | PA3 | 推挽输出 | 无 | 高（静音） | 低（鸣响） |

**电路接法**：
```
3.3V ── Buzzer+ ──[三极管驱动]── GPIO    （灌电流，低电平鸣响）
                              ↑
              蜂鸣器通常需要三极管放大电流，STM32 引脚直接驱动能力不足
```

**电气特性**：
- 驱动方式：灌电流（通过三极管）
- 最大电流：蜂鸣器工作电流约 30~80mA，**需三极管放大**
- 工作电压：3.3V（有源蜂鸣器）或 5V（需电平转换）

## 3. 文件清单

| 文件 | 路径 | 行数 | 大小 |
|---|---|---|---|
| 头文件 | [[Buzzer.h]] | 21 | 579 B |
| 源文件 | [[Buzzer.c]] | 22 | 741 B |

## 4. 接口函数

### 4.1 `Buzzer_write` （调用频率：事件触发）
```c
void Buzzer_write(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState state);
```
- **作用**：设置蜂鸣器鸣响/静音状态
- **参数**：
  - `GPIOx`：端口
  - `GPIO_Pin`：引脚号
  - `state`：`Buzzer_ON`（鸣响）或 `Buzzer_OFF`（静音）
- **返回值**：无
- **调用频率**：需要改变蜂鸣器状态时调用
- **使用示例**：
```c
Buzzer_write(Buzzer_GPIO_Port, Buzzer_Pin, Buzzer_ON);  // 鸣响
Buzzer_write(Buzzer_GPIO_Port, Buzzer_Pin, Buzzer_OFF); // 静音
```

### 4.2 `Buzzer_Blink_Handler` （调用频率：主循环每次，当前未使用）
```c
void Buzzer_Blink_Handler(Buzzer_Blink_t *buzzer);
```
- **作用**：非阻塞间歇鸣响处理（到时间自动翻转）
- **参数**：`buzzer`：蜂鸣器闪烁句柄指针
- **返回值**：无
- **调用频率**：主循环每次（如需间歇鸣响功能）
- **使用示例**：
```c
Buzzer_Blink_t buzzer = {Buzzer_GPIO_Port, Buzzer_Pin, 200, 0, 0};  // 200ms 间歇
while (1) {
    Buzzer_Blink_Handler(&buzzer);  // 持续调用
}
```
- **注意事项**：当前版本未使用此函数，实际使用的是 `Buzzer_Toggle_Pin`

### 4.3 `Buzzer_Toggle_Pin` （调用频率：事件触发）
```c
void Buzzer_Toggle_Pin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
```
- **作用**：翻转蜂鸣器引脚（响/停切换）
- **参数**：
  - `GPIOx`：端口
  - `GPIO_Pin`：引脚号
- **返回值**：无
- **调用频率**：按键事件触发时调用
- **使用示例**：
```c
if (Key2.press_flag == 1) {
    Key2.press_flag = 0;
    Buzzer_Toggle_Pin(Buzzer_GPIO_Port, Buzzer_Pin);  // 响/停切换
}
```

## 5. 数据结构

### `Buzzer_Blink_t` （16 字节）
```c
typedef struct {
    GPIO_TypeDef* port;        // 端口                        ← 占 4 字节
    uint16_t      pin;         // 引脚                        ← 占 2 字节
    uint32_t      interval_ms; // 间歇间隔（ms）              ← 占 4 字节
    uint32_t      last_tick;   // 上次翻转时间戳               ← 占 4 字节
    uint8_t       state;       // 当前状态（0/1）              ← 占 1 字节
} Buzzer_Blink_t;
```
- **内存占用**：约 16 字节（含对齐填充）
- **生命周期**：全局静态（当前未使用，保留待用）

## 6. 关键宏定义

| 宏名 | 值 | 含义 | 硬件相关性 |
|---|---|---|---|
| `Buzzer_ON` | `GPIO_PIN_RESET` | 低电平鸣响 | 板子相关，换板子要改 |
| `Buzzer_OFF` | `GPIO_PIN_SET` | 高电平静音 | 板子相关，换板子要改 |

> **注意**：宏命名不符合 C 语言规范，应为 `BUZZER_ON`（全大写），下个版本修正

## 7. 调用流程

### 7.1 初始化（main 函数开头）
```c
// 1. HAL 初始化
HAL_Init();
SystemClock_Config();
MX_GPIO_Init();

// 2. 蜂鸣器初始化（静音）
Buzzer_write(Buzzer_GPIO_Port, Buzzer_Pin, Buzzer_OFF);
```

### 7.2 主循环（while(1)）
```c
while (1) {
    // 1. 按键事件处理（开关模式）
    if (Key2.press_flag == 1) {
        Key2.press_flag = 0;
        Buzzer_Toggle_Pin(Buzzer_GPIO_Port, Buzzer_Pin);
    }

    // 2. 如需间歇鸣响，用 Buzzer_Blink_Handler（当前未启用）
    // Buzzer_Blink_Handler(&buzzer);
}
```

### 7.3 中断（如有）
```c
// 当前版本未使用中断
```

## 8. 依赖关系

- **依赖的 HAL 函数**：
  - `HAL_GPIO_WritePin`（操作 BSRR 寄存器，原子写）
  - `HAL_GPIO_TogglePin`（读 ODR → 改写 → 写 BSRR）
  - `HAL_GetTick`（读取 SysTick 计数器，仅 Blink_Handler 使用）
- **依赖的其他模块**：`main.h`、`gpio.h`（获取 `Buzzer_GPIO_Port` 等宏）
- **被谁调用**：`main.c`
- **调用谁**：无（只操作 GPIO，不调用其他模块）

## 9. 底层寄存器（进阶）

| 寄存器 | 功能 | 模块如何使用 |
|---|---|---|
| `BSRR` | 位设置/复位（原子操作） | `HAL_GPIO_WritePin` 底层用，设置/复位引脚 |
| `ODR` | 输出数据寄存器 | `HAL_GPIO_TogglePin` 底层先读 ODR，异或后写回 BSRR |
| `CRL/CRH` | 端口配置（模式+速度） | `MX_GPIO_Init` 配置为推挽输出，速度 LOW |
| `SysTick->VAL` | 系统滴答定时器 | `HAL_GetTick` 底层读取，提供时间基准 |

**寄存器补充说明**：
- 蜂鸣器与 LED 共用同一套寄存器机制，只是引脚不同
- `BSRR` 的原子性确保多设备控制时不会互相干扰

## 10. 设计要点

- **低电平鸣响（灌电流）**：蜂鸣器和 LED 一样，低电平有效
- **与 LED 模块结构完全一致**：90% 代码重复，是整合为 `Dev_GPIO` 的主要候选
- **已解除对 LED.h 的依赖**：v1.0 修复了 `#include "LED.h"` 的设计问题
- **开关模式 vs 闪烁模式**：当前用 Toggle（开关），保留了 Blink（闪烁）待用

## 11. 测试方法

**验证步骤**：
1. 编译无错误无警告
2. 下载运行，确认上电时蜂鸣器静音（默认高电平）
3. 按 Key2，蜂鸣器应响/停切换
4. 长按 Key2，只切换一次（状态机防重复）

**预期现象**：
- 上电：蜂鸣器静音
- Key2 按一下：蜂鸣器响
- Key2 再按一下：蜂鸣器停

**常见故障排查**：

| 现象 | 可能原因 | 解决方案 |
|---|---|---|
| 上电就响 | gpio.c 默认电平为 RESET | 改为 `GPIO_PIN_SET` |
| 不响 | 三极管驱动电路问题 / 引脚错 | 检查硬件电路、引脚配置 |
| 按键无响应 | `press_flag` 没清零 | 消费后立即 `= 0` |
| 响声不断 | Toggle 逻辑反 / 宏定义反 | 检查 `Buzzer_ON/OFF` 值 |
| 间歇鸣响不工作 | `Blink_Handler` 没放主循环 | 在 while(1) 里调用 |

## 12. 已知问题 / 改进方向

- [ ] **宏命名不规范**：`Buzzer_ON` 应为 `BUZZER_ON`（全大写，符合 C 宏命名规范）
- [ ] `Buzzer_Blink_Handler` 和 `Buzzer_Blink_t` 定义了但未使用（死代码）
- [ ] 与 LED 模块 90% 重复，应整合为通用 `Dev_GPIO` 模块
- [ ] 缺少 `Buzzer_Init` 函数（当前在 main.c 手动调 `Buzzer_write`）
- [ ] **下一版本计划**：v2.0 与 LED 整合为 `Dev_GPIO`，统一宏命名规范
