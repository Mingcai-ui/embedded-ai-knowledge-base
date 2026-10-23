---
类型: 代码
状态: 归档
标签: [stm32, 按键, gpio, 江协科技, 旧版]
模块: Key
硬件平台: STM32F103C8T6
版本: v1.0
创建: 2026-08-20
---

> ⚠️ 本文件为 2026-08-20 旧版。同主题现行笔记为 [[STM32_按键消抖_状态机]]（知识库，三态状态机实现）与 [[Key_介绍]]（可移植模块代码配套文档）。此处仅作历史留档，**不要在此更新**。

# 代码介绍：Key 模块

> **模块类型**：Key（按键输入设备）
> **所属节号**：1-2 按键与LED闪烁与蜂鸣器
> **版本**：v1.0
> **硬件平台**：STM32F103C8T6
> **最后更新**：2026-08-20

## 1. 模块概述

**一句话功能**：检测按键按下，通过三状态机实现软件消抖，产生按键事件标志。

**设计模式**：有限状态机（FSM）+ 事件标志位

**复杂度**：★★★☆☆

## 2. 硬件信息

**引脚连接**：

| 设备 | 端口 | 引脚 | 模式 | 上下拉 | 默认电平 | 有效电平 |
|---|---|---|---|---|---|---|
| Key1 | GPIOB | PB1 | 上拉输入 | 上拉 | 高（释放） | 低（按下） |
| Key2 | GPIOB | PB11 | 上拉输入 | 上拉 | 高（释放） | 低（按下） |

**电路接法**：
```
3.3V ──[内部上拉]── GPIO ── Key ── GND
                           ↑
        按下时拉低，释放时被上拉拉高
```

**电气特性**：
- 驱动方式：无驱动（纯输入检测）
- 工作电压：3.3V
- 消抖时间：30ms（软件消抖）

## 3. 文件清单

| 文件 | 路径 | 行数 | 大小 |
|---|---|---|---|
| 头文件 | [[Key.h]] | 19 | 583 B |
| 源文件 | [[Key.c]] | 48 | 1859 B |

## 4. 接口函数

### 4.1 `Button_Init` （调用频率：初始化一次）
```c
void Button_Init(Button_Handle_t *btn, GPIO_TypeDef* port, uint16_t pin);
```
- **作用**：初始化按键句柄，设置端口、引脚、消抖时间
- **参数**：
  - `btn`：按键句柄指针
  - `port`：端口（如 `GPIOB`）
  - `pin`：引脚号（如 `GPIO_PIN_1`）
- **返回值**：无
- **调用频率**：main 函数开头，初始化一次
- **使用示例**：
```c
Button_Init(&Key1, Key1_GPIO_Port, Key1_Pin);
Button_Init(&Key2, Key2_GPIO_Port, Key2_Pin);
```
- **注意事项**：必须先调用，否则 `Button_Handler` 会读未初始化的句柄

### 4.2 `Button_Handler` （调用频率：主循环每次）
```c
void Button_Handler(Button_Handle_t *btn);
```
- **作用**：按键状态机处理（消抖 + 事件检测）
- **参数**：`btn`：按键句柄指针
- **返回值**：无（通过 `press_flag` 传递事件）
- **调用频率**：主循环每次都要调用
- **使用示例**：
```c
while (1) {
    Button_Handler(&Key1);   // 必须持续调用
    Button_Handler(&Key2);

    if (Key1.press_flag == 1) {
        Key1.press_flag = 0;  // 消费后必须清零！
        LED_Toggle_Pin(LED2_GPIO_Port, LED2_Pin);
    }
}
```
- **注意事项**：不能放在按键事件里（只调一次会漏检），必须在主循环持续调用

## 5. 数据结构

### `Button_Handle_t` （12 字节）
```c
typedef struct {
    GPIO_TypeDef* port;        // 端口                        ← 占 4 字节
    uint16_t      pin;         // 引脚                        ← 占 2 字节
    uint8_t       state;       // 状态机：0/1/2                ← 占 1 字节
    uint32_t      last_tick;   // 上次状态改变的时间戳         ← 占 4 字节
    uint32_t      debounce_ms; // 消抖时间（ms）               ← 占 4 字节
    uint8_t       press_flag;  // 按下触发标志                 ← 占 1 字节
} Button_Handle_t;
```
- **内存占用**：约 16 字节（含对齐填充）
- **生命周期**：全局静态（`Key1`、`Key2` 在 Key.h 中声明）

## 6. 关键宏定义

| 宏名 | 值 | 含义 | 硬件相关性 |
|---|---|---|---|
| 无 | - | - | - |

> **注意**：当前版本消抖时间 `30` 硬编码在 `Button_Init` 中，建议改为宏

**建议补充的宏**：
```c
#define BUTTON_DEBOUNCE_MS  30    // 消抖时间
#define BUTTON_PRESSED      1     // 按下标志
#define BUTTON_STATE_IDLE   0     // 状态：空闲
#define BUTTON_STATE_DEBOUNCE 1   // 状态：消抖中
#define BUTTON_STATE_WAIT_RELEASE 2  // 状态：等待释放
```

## 7. 调用流程

### 7.1 初始化（main 函数开头）
```c
// 1. HAL 初始化
HAL_Init();
SystemClock_Config();
MX_GPIO_Init();

// 2. 按键初始化
Button_Init(&Key1, Key1_GPIO_Port, Key1_Pin);
Button_Init(&Key2, Key2_GPIO_Port, Key2_Pin);
```

### 7.2 主循环（while(1)）
```c
while (1) {
    // 1. 持续调用状态机（非阻塞）
    Button_Handler(&Key1);
    Button_Handler(&Key2);

    // 2. 事件处理（消费标志位）
    if (Key1.press_flag == 1) {
        Key1.press_flag = 0;  // ← 必须清零！
        LED_Toggle_Pin(LED2_GPIO_Port, LED2_Pin);
    }
    if (Key2.press_flag == 1) {
        Key2.press_flag = 0;  // ← 必须清零！
        Buzzer_Toggle_Pin(Buzzer_GPIO_Port, Buzzer_Pin);
    }
}
```

### 7.3 中断（如有）
```c
// 当前版本未使用外部中断，纯轮询检测
// 未来可扩展为 EXTI 中断 + 消抖定时器
```

## 8. 依赖关系

- **依赖的 HAL 函数**：
  - `HAL_GPIO_ReadPin`（读取 IDR 寄存器）
  - `HAL_GetTick`（读取 SysTick 计数器）
- **依赖的其他模块**：`main.h`、`gpio.h`（获取 `Key1_GPIO_Port` 等宏）
- **被谁调用**：`main.c`
- **调用谁**：无（只产生 `press_flag`，不直接操作其他设备）

## 9. 底层寄存器（进阶）

| 寄存器 | 功能 | 模块如何使用 |
|---|---|---|
| `IDR` | 输入数据寄存器（16 位） | `HAL_GPIO_ReadPin` 读取对应位，判断按键电平 |
| `CRL/CRH` | 端口配置（模式+速度） | `MX_GPIO_Init` 配置为上拉输入模式 |
| `SysTick->VAL` | 系统滴答定时器 | `HAL_GetTick` 底层读取，提供时间基准 |

**寄存器补充说明**：
- `IDR` 是只读寄存器，反映引脚当前真实电平
- 按键抖动本质是 IDR 在短时间内快速变化
- 消抖算法就是过滤 IDR 的短时跳变

## 10. 设计要点

- **三状态机消抖**：比"延时消抖"更优，不阻塞主循环，可多按键并行
- **事件标志模式**：`press_flag` 实现了"边沿检测"，主循环消费后清零
- **防重复触发**：状态 2（等待释放）确保按住不放只触发一次
- **非阻塞设计**：所有时间判断用 `HAL_GetTick()` 差值，不用 `HAL_Delay`

## 11. 测试方法

**验证步骤**：
1. 编译无错误无警告
2. 下载运行，观察 LED1 是否正常闪烁（500ms）
3. 按 Key1，LED2 应翻转一次
4. 按 Key2，蜂鸣器应响/停切换
5. 按住 Key1 不放，LED2 只翻转一次（不重复触发）

**预期现象**：
- Key1 按一下：LED2 翻转一次
- Key2 按一下：蜂鸣器开关切换
- 快速抖动按键：不会误触发（30ms 消抖）

**常见故障排查**：

| 现象 | 可能原因 | 解决方案 |
|---|---|---|
| 按键无响应 | 没调 `Button_Handler` | 检查主循环是否调用 |
| 按键狂闪 | `press_flag` 没清零 | 消费后立即 `= 0` |
| 按一下触发多次 | 删除了状态 2（等待释放） | 恢复三状态机 |
| 按键时好时坏 | 消抖时间太短 | 增大 `debounce_ms` 到 30~50ms |
| 两个按键互相干扰 | 共用句柄 | 每个按键独立句柄 |

## 12. 已知问题 / 改进方向

- [ ] 状态号用魔法数字（0/1/2），应改为枚举 `Button_State_t`
- [ ] `Key1`/`Key2` 在头文件中全局声明，应移到 .c 或由用户在 main.c 定义
- [ ] 消抖时间硬编码为 30ms，应改为宏或参数
- [ ] 缺少长按检测功能（未来可扩展状态 3：长按）
- [ ] 缺少双击检测功能（未来可扩展状态 4：双击）
- [ ] **下一版本计划**：v2.0 增加枚举状态 + 长按支持
