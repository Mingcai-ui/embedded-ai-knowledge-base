---
类型: 知识
状态: 进行中
标签: [stm32, 按键, 消抖, 状态机]
创建: 2026-09-16
---

# STM32 按键消抖：三态状态机（详细版）

> 来源：江科大（江协科技）STM32 教程
> 本文为「代码介绍」详细版。

## 1. 模块概述

**一句话功能**：检测按键按下，通过三态状态机实现软件消抖，产生按键事件标志。

**设计模式**：有限状态机（FSM）+ 事件标志位
**复杂度**：★★★☆☆

## 2. 硬件信息

**引脚连接**（已确认，江科大工程）：

| 设备 | 端口 | 引脚 | 模式 | 上下拉 | 默认电平 | 有效电平 |
|---|---|---|---|---|---|---|
| Key1 | GPIOA | PA3 | 上拉输入 | 上拉 | 高（释放） | 低（按下） |
| Key2 | GPIOA | PA4 | 上拉输入 | 上拉 | 高（释放） | 低（按下） |

> ⚠️ **数据冲突提示**：收件箱旧版介绍写 Key1=PB1、Key2=PB11；经核对确认版为 **Key1=PA3、Key2=PA4**（GPIOA）。本文以确认版为准。

**电路接法**：
```
3.3V ──[内部上拉]── GPIO ── Key ── GND
                           ↑
      按下拉低，释放被上拉拉高
```
**电气特性**：纯输入检测（无驱动）；工作电压 3.3V；消抖时间 30ms（软件消抖）。

## 3. 为什么用状态机

机械按键按下/释放瞬间会抖动，直接读电平会误触发多次。用「空闲→消抖→等待释放」三态状态机 + `HAL_GetTick()` 非阻塞消抖，既不卡主循环，也能防止按住重复触发。

**状态转换**：
- 空闲(0)：检测到低电平（按下）→ 进入消抖(1)，记时间戳。
- 消抖中(1)：电平恢复高 → 回空闲（抖动干扰，取消本次）；持续低电平满 `debounce_ms` → 进入等待释放(2)，置 `press_flag=1`。
- 等待释放(2)：电平恢复高（释放）→ 回空闲，准备下次。

## 4. 文件清单

| 文件 | 路径 | 备注 |
|---|---|---|
| 头文件 | [[Key.h]] | 声明 `Key1/Key2` 全局句柄 + 函数原型 |
| 源文件 | [[Key.c]] | 状态机实现 |

原始代码：`08_归档/可移植模块代码/Key/Key.c`

## 5. 接口函数

### 5.1 `Button_Init`（初始化一次）
```c
void Button_Init(Button_Handle_t *btn, GPIO_TypeDef* port, uint16_t pin);
```
- **作用**：绑定端口/引脚，复位状态机，设置消抖时间
- **位置**：`main()` 初始化阶段调用，每个按键一个句柄
- **示例**：`Button_Init(&Key1, GPIOA, GPIO_PIN_3);` `Button_Init(&Key2, GPIOA, GPIO_PIN_4);`
- **注意**：必须先调用，否则 `Button_Handler` 会读未初始化句柄

### 5.2 `Button_Handler`（主循环每次）
```c
void Button_Handler(Button_Handle_t *btn);
```
- **作用**：三态状态机轮询（消抖 + 事件检测），检测到有效按下时置 `press_flag=1`
- **位置**：**必须放 `while(1)` 主循环持续调用**，不能只放按键事件里
- **示例**：
```c
while (1) {
    Button_Handler(&Key1); Button_Handler(&Key2);
    if (Key1.press_flag) { Key1.press_flag = 0; /* 处理一次事件 */ }
}
```
- **返回值**：无，通过 `btn->press_flag` 传递事件

## 6. 数据结构

### `Button_Handle_t`（约 16 字节，含对齐填充）
```c
typedef struct {
    GPIO_TypeDef* port;        // 端口                        ← 4 字节
    uint16_t      pin;         // 引脚                        ← 2 字节
    uint8_t       state;       // 状态机：0/1/2               ← 1 字节
    uint32_t      last_tick;   // 上次状态改变时间戳           ← 4 字节
    uint32_t      debounce_ms; // 消抖时间(ms)                ← 4 字节
    uint8_t       press_flag;  // 按下触发标志                 ← 1 字节
} Button_Handle_t;
```
- **内存占用**：约 16 字节（含对齐填充）
- **生命周期**：全局静态（`Key1`、`Key2` 在 Key.h 中声明）

## 7. 关键宏定义

当前版本消抖时间 `30` **硬编码在 `Button_Init`**，建议改为宏：

```c
#define BUTTON_DEBOUNCE_MS  30
#define BUTTON_PRESSED      1
#define BUTTON_STATE_IDLE         0
#define BUTTON_STATE_DEBOUNCE     1
#define BUTTON_STATE_WAIT_RELEASE 2
```

## 8. 调用流程

```c
// 初始化（main 开头）
HAL_Init(); SystemClock_Config(); MX_GPIO_Init();
Button_Init(&Key1, GPIOA, GPIO_PIN_3);
Button_Init(&Key2, GPIOA, GPIO_PIN_4);

// 主循环（while(1)）
while (1) {
    Button_Handler(&Key1); Button_Handler(&Key2);   // 持续轮询
    if (Key1.press_flag) { Key1.press_flag = 0; LED_Toggle_Pin(GPIOA, GPIO_PIN_1); }
    if (Key2.press_flag) { Key2.press_flag = 0; Buzzer_Toggle_Pin(GPIOA, GPIO_PIN_2); }
}
```
当前版本未使用外部中断，纯轮询；未来可扩展为 EXTI 中断 + 消抖定时器。

## 9. 依赖关系

- **HAL 函数**：`HAL_GPIO_ReadPin`（读 IDR）、`HAL_GetTick`（SysTick）
- **依赖其他模块**：`main.h`、`gpio.h`（引脚宏）
- **被谁调用**：`main.c`
- **调用谁**：无（只产生 `press_flag`，不直接操作其他设备）

## 10. 底层寄存器（进阶）

| 寄存器 | 功能 | 模块如何使用 |
|---|---|---|
| `IDR` | 输入数据寄存器（16 位，只读） | `HAL_GPIO_ReadPin` 读对应位判断电平 |
| `CRL/CRH` | 端口配置（模式+速度） | `MX_GPIO_Init` 配为上拉输入 |
| `SysTick->VAL` | 系统滴答定时器 | `HAL_GetTick` 提供时间基准 |

> `IDR` 反映引脚真实电平；按键抖动本质是 IDR 短时快速变化，消抖算法即过滤 `IDR` 的短时跳变。

## 11. 设计要点

- **三状态机消抖**：比「延时消抖」更优，不阻塞主循环，可多按键并行
- **事件标志模式**：`press_flag` 实现「边沿检测」，主循环消费后清零
- **防重复触发**：状态 2（等待释放）确保按住不放只触发一次
- **非阻塞设计**：时间判断用 `HAL_GetTick()` 差值，不用 `HAL_Delay`

## 12. 测试方法

**验证步骤**：编译无错误无警告 → 下载运行 → 按 Key1 翻转 LED2 一次 → 按 Key2 切换蜂鸣器 → 快速抖动不误触发（30ms 消抖）。
**预期现象**：Key1 按一下 LED2 翻转一次；Key2 按一下蜂鸣器开关切换；按住不放不重复触发。

**常见故障排查**：

| 现象 | 可能原因 | 解决方案 |
|---|---|---|
| 按键无响应 | 没调 `Button_Handler` | 检查主循环调用 |
| 触发多次 | `press_flag` 没清零 / 删了状态 2 | 消费后清零；恢复三状态机 |
| 时好时坏 | 消抖时间太短 | 增大 `debounce_ms` 到 30~50ms |
| 两键互相干扰 | 共用句柄 | 每个按键独立句柄 |

## 13. 已知问题 / 改进方向

- [ ] 状态号用魔法数字（0/1/2），应改为枚举 `Button_State_t`
- [ ] `Key1/Key2` 在头文件全局声明，应移到 .c 或由用户 main.c 定义
- [ ] 消抖时间硬编码 30ms，应改为宏/参数
- [ ] 缺少长按 / 双击检测（未来扩展状态 3/4）
- [ ] **v2.0 计划**：增加枚举状态 + 长按支持

## 相关

- [[STM32_GPIO输出_LED与蜂鸣器]]
- [[OLED显示_软件I2C驱动]]
- 原始代码：`08_归档/可移植模块代码/Key/Key.c`