# 代码介绍：LED 模块

> **模块类型**：LED（数字输出设备）
> **所属节号**：1-2 按键与LED闪烁与蜂鸣器
> **版本**：v1.0
> **硬件平台**：STM32F103C8T6
> **最后更新**：2026-08-20

## 1. 模块概述

**一句话功能**：控制 LED 的开关、翻转和非阻塞闪烁。

**设计模式**：非阻塞延时（基于 `HAL_GetTick()`）+ 硬件抽象封装

**复杂度**：★★☆☆☆

## 2. 硬件信息

**引脚连接**：

| 设备 | 端口 | 引脚 | 模式 | 上下拉 | 默认电平 | 有效电平 |
|---|---|---|---|---|---|---|
| LED1 | GPIOA | PA1 | 推挽输出 | 无 | 高（灭） | 低（亮） |
| LED2 | GPIOA | PA2 | 推挽输出 | 无 | 高（灭） | 低（亮） |

**电路接法**：
```
3.3V ── LED+ ──[330Ω]── GPIO    （灌电流，低电平点亮）
                          ↑
              引脚输出低电平时，电流从 3.3V 流入 GPIO，LED 亮
```

**电气特性**：
- 驱动方式：灌电流（GPIO 吸收电流）
- 最大电流：8mA（STM32 单引脚限制）
- 限流电阻：330Ω（典型值，3.3V / 330Ω ≈ 10mA，安全范围）
- 工作电压：3.3V

## 3. 文件清单

| 文件 | 路径 | 行数 | 大小 |
|---|---|---|---|
| 头文件 | [[LED.h]] | 24 | 575 B |
| 源文件 | [[LED.c]] | 38 | 1176 B |

## 4. 接口函数

### 4.1 `LED_Init` （调用频率：初始化一次）
```c
void LED_Init(void);
```
- **作用**：关闭所有 LED（拉高引脚）
- **参数**：无
- **返回值**：无
- **调用频率**：main 函数开头，初始化一次
- **使用示例**：
```c
LED_Init();  // 上电后调用，确保 LED 初始为灭
```
- **注意事项**：函数名有误导性，实际是"关闭所有 LED"，不是真正的初始化

### 4.2 `LED_write` （调用频率：事件触发）
```c
void LED_write(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState state);
```
- **作用**：设置 LED 亮灭状态
- **参数**：
  - `GPIOx`：端口（如 `GPIOA`）
  - `GPIO_Pin`：引脚号（如 `GPIO_PIN_1`）
  - `state`：`LED_ON`（亮）或 `LED_OFF`（灭）
- **返回值**：无
- **调用频率**：需要改变 LED 状态时调用
- **使用示例**：
```c
LED_write(LED1_GPIO_Port, LED1_Pin, LED_ON);  // 点亮 LED1
LED_write(LED1_GPIO_Port, LED1_Pin, LED_OFF); // 熄灭 LED1
```

### 4.3 `LED_Blink_Handler` （调用频率：主循环每次）
```c
void LED_Blink_Handler(LED_Blink_t *led);
```
- **作用**：非阻塞闪烁处理（到时间自动翻转）
- **参数**：`led`：LED 闪烁句柄指针
- **返回值**：无
- **调用频率**：主循环每次都要调用
- **使用示例**：
```c
LED_Blink_t led1 = {LED1_GPIO_Port, LED1_Pin, 500, 0, 0};  // 500ms 闪烁
while (1) {
    LED_Blink_Handler(&led1);  // 每次循环调用
}
```
- **注意事项**：不能放在按键事件里（只调用一次不会闪烁），必须在主循环持续调用

### 4.4 `LED_Toggle_Pin` （调用频率：事件触发）
```c
void LED_Toggle_Pin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
```
- **作用**：翻转任意 GPIO 引脚（通用版本，可操作其他引脚）
- **参数**：
  - `GPIOx`：端口
  - `GPIO_Pin`：引脚号
- **返回值**：无
- **调用频率**：按键事件触发时调用
- **使用示例**：
```c
if (Key1.press_flag == 1) {
    Key1.press_flag = 0;
    LED_Toggle_Pin(LED2_GPIO_Port, LED2_Pin);  // 翻转 LED2
}
```

## 5. 数据结构

### `LED_Blink_t` （16 字节）
```c
typedef struct {
    GPIO_TypeDef* port;        // 端口                        ← 占 4 字节
    uint16_t      pin;         // 引脚                        ← 占 2 字节
    uint32_t      interval_ms; // 闪烁间隔（ms）              ← 占 4 字节
    uint32_t      last_tick;   // 上次翻转时间戳               ← 占 4 字节
    uint8_t       state;       // 当前亮灭状态（0/1）           ← 占 1 字节
} LED_Blink_t;
```
- **内存占用**：约 16 字节（含对齐填充）
- **生命周期**：局部静态（通常在 main 函数中定义）

## 6. 关键宏定义

| 宏名 | 值 | 含义 | 硬件相关性 |
|---|---|---|---|
| `LED_ON` | `GPIO_PIN_RESET` | 低电平点亮（灌电流） | 板子相关，换板子要改 |
| `LED_OFF` | `GPIO_PIN_SET` | 高电平熄灭 | 板子相关，换板子要改 |

## 7. 调用流程

### 7.1 初始化（main 函数开头）
```c
// 1. HAL 初始化
HAL_Init();
SystemClock_Config();
MX_GPIO_Init();

// 2. LED 初始化（关闭所有 LED）
LED_Init();

// 3. 配置闪烁句柄
LED_Blink_t led1 = {LED1_GPIO_Port, LED1_Pin, 500, 0, 0};  // 500ms 闪烁
```

### 7.2 主循环（while(1)）
```c
while (1) {
    // 1. 持续调用闪烁处理（非阻塞）
    LED_Blink_Handler(&led1);

    // 2. 按键事件处理
    if (Key1.press_flag == 1) {
        Key1.press_flag = 0;
        LED_Toggle_Pin(LED2_GPIO_Port, LED2_Pin);
    }
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
  - `HAL_GetTick`（读取 SysTick 计数器）
- **依赖的其他模块**：`main.h`、`gpio.h`（获取 `LED1_GPIO_Port` 等宏）
- **被谁调用**：`main.c`
- **调用谁**：无（只操作 GPIO，不调用其他模块）

## 9. 底层寄存器（进阶）

| 寄存器 | 功能 | 模块如何使用 |
|---|---|---|
| `BSRR` | 位设置/复位（原子操作） | `HAL_GPIO_WritePin` 底层用，低 16 位复位，高 16 位设置 |
| `ODR` | 输出数据寄存器 | `HAL_GPIO_TogglePin` 底层先读 ODR，异或后写回 BSRR |
| `CRL/CRH` | 端口配置（模式+速度） | `MX_GPIO_Init` 配置为推挽输出，速度 LOW |
| `SysTick->VAL` | 系统滴答定时器 | `HAL_GetTick` 底层读取，提供时间基准 |

**寄存器补充说明**：
- `BSRR` 是"原子操作"寄存器：写 1 设置/复位，写 0 无影响，单周期完成
- 比"读 ODR → 改位 → 写 ODR"更安全，不会被中断打断
- `HAL_GPIO_TogglePin` 内部其实是读 ODR 异或后写 BSRR

## 10. 设计要点

- **低电平点亮（灌电流）**：LED 正极接 3.3V，负极接 GPIO，低电平时 LED 亮
- **非阻塞闪烁**：用 `HAL_GetTick()` 差值比较，不阻塞主循环，可多任务并行
- **溢出安全**：`HAL_GetTick() - last_tick` 用减法，uint32_t 溢出也能正确计算
- **硬件映射宏**：`LED_ON/LED_OFF` 隔离了"低电平点亮"这个硬件特性，换板子只改宏

## 11. 测试方法

**验证步骤**：
1. 编译无错误无警告
2. 下载运行，观察 LED1 是否以 500ms 间隔闪烁
3. 按 Key1，LED2 应翻转一次
4. 按住 Key1 不放，LED2 只翻转一次（状态机防重复）

**预期现象**：
- LED1 以 500ms 间隔闪烁
- Key1 按一下：LED2 翻转一次
- 上电瞬间 LED 不会误亮（`LED_Init` 拉高）

**常见故障排查**：

| 现象 | 可能原因 | 解决方案 |
|---|---|---|
| LED 不亮 | 引脚配置错误 / 电平逻辑反 | 检查 GPIO 模式、`LED_ON` 宏值 |
| LED 一直亮 | `LED_Init` 没调用 / 默认电平错 | 检查 gpio.c 默认电平为 SET |
| LED 不闪烁 | `LED_Blink_Handler` 没放主循环 | 确保在 while(1) 里持续调用 |
| 闪烁间隔不对 | `interval_ms` 设置错误 | 检查句柄初始化的 interval 值 |
| 按键触发 LED 狂闪 | `press_flag` 没清零 | 消费后立即 `= 0` |

## 12. 已知问题 / 改进方向

- [ ] `LED_Init` 函数名有误导性，应改为 `LED_AllOff` 或 `LED_DeInit`
- [ ] `LED_write` 函数名下划线开头，不符合 HAL 命名规范（应首字母大写 `LED_Write`）
- [ ] 缺少初始化闪烁句柄的专用函数（当前靠手动初始化结构体）
- [ ] **下一版本计划**：v2.0 与 Buzzer 整合为通用 `Dev_GPIO` 模块
