---
类型: 板卡
状态: 进行中
标签: [STM32, STM32F103C8T6, 引脚, PinMux, 芯片资源]
型号: STM32F103C8T6
封装: LQFP48 (7×7mm)
平台: [STM32]
可信度: L0
来源类型: ST 官方参考手册 + 官方数据手册
来源版本: RM0008 参考手册中文第10版（对应英文版 Rev 10） + DS5319 数据手册 Rev 20
创建: 2026-09-02
---

# STM32F103C8T6｜芯片资源与PinMux速查

> 适用边界：STM32F103C8T6，中等容量增强型（64KB Flash），LQFP48 封装。
> 本文档基于《STM32F10xxx参考手册》RM0008 与数据手册 DS5319 蒸馏，**不等于上板验证**（trust_level=L0）。
> 引脚分配以 LQFP48（7×7mm）为基准。

## 1. 芯片身份与核心规格

| 项 | 值 |
|---|---|
| 型号 | STM32F103C8T6 |
| 内核 | ARM Cortex-M3，72MHz，1.25 DMIPS/MHz（0 等待周期访问） |
| Flash / SRAM | 64KB / 20KB |
| 封装 | LQFP48（7×7mm），工作温度 -40~+85℃（型号后缀 6） |
| 供电 | VDD 2.0~3.6V；VDDA 独立模拟电源 |
| GPIO | 37 个高速 I/O，几乎全部兼容 5V（FT 标记） |
| 已用案例 | 江科大入门、2026 电赛视觉小车主控 |

**型号命名解读**：`STM32 | F | 103 | C | 8 | T | 6`
- `F103` = 增强型子系列（主频最高 72MHz）
- `C` = 48 引脚；`8` = 64KB Flash；`T` = LQFP 封装；`6` = -40~85℃

## 2. 外设资源总览

### 通信接口（共 9 个）
| 外设 | 数量 | 说明 / 引脚复用 |
|---|---|---|
| USART | 3 | USART1（PA9/PA10）、USART2（PA2/PA3）、USART3（PB10/PB11） |
| I2C | 2 | I2C1（PB6/PB7）、I2C2（PB10/PB11），支持 SMBus/PMBus |
| SPI | 2 | SPI1（PA5~PA7）、SPI2（PB13~PB15），最高 18Mb/s |
| CAN | 1 | CAN 2.0B Active，PA11(RX)/PA12(TX) |
| USB | 1 | USB 2.0 全速，PA11(DM)/PA12(DP) |

### 定时器（共 7 个）
| 定时器 | 位数 | 说明 |
|---|---|---|
| TIM1 | 16 | 高级控制定时器，带死区控制 + 紧急刹车（BKIN） |
| TIM2 / TIM3 / TIM4 | 16 | 通用定时器，各 4 通道，支持正交编码器输入 |
| SysTick | 24 | 系统嘀嗒（由内核提供） |
| IWDG | — | 独立看门狗（由 LSI 40kHz 驱动） |
| WWDG | — | 窗口看门狗 |

### 其他关键外设
- **2× ADC**（12 位，最快 1µs，各最多 16 通道，模拟输入范围 0~3.6V）
  - ADC1_IN0~IN7 多分布在 PA0~PA7；ADC1_IN8~IN15 多分布在 PB0~PB1 及 PC0~PC7（C8 封装无 PC0~PC7，具体见表）
  - 内建温度传感器
- **DMA**：7 通道，支持定时器/ADC/I2C/SPI/USART
- **NVIC**：60 个中断 + 10 个异常，16 级优先级
- **CRC**：硬件 CRC 计算单元；**96 位唯一 ID**
- **RTC + 后备寄存器**：VBAT 域供电，带校准的 32kHz 振荡器
- **低功耗**：Sleep / Stop / Standby 三种模式
- **电源监控**：POR / PDR / PVD（可编程电压检测）

## 3. 时钟（RM0008）

| 时钟源 | 频率 | 用途 |
|---|---|---|
| HSI（内） | 8MHz RC | 高速系统时钟（出厂校准） |
| HSE（外） | 4~16MHz 晶振 | 外部高速时钟（PD0/PD1，即 OSC_IN/OSC_OUT） |
| LSI（内） | 40kHz RC | 独立看门狗、RTC（低精度） |
| LSE（外） | 32.768kHz 晶振 | RTC（PC14/PC15） |
| PLL | 倍频 | 由 HSI/HSE 倍频产生系统时钟（最高 72MHz） |

系统时钟 SYSCLK 最高 72MHz；AHB/APB1/APB2 分频：APB1≤36MHz，APB2≤72MHz。

## 4. 调试接口（SWJ-DP，RM0008 表34）

| 信号 | 引脚 |
|---|---|
| JTMS / SWDIO | PA13 |
| JTCK / SWCLK | PA14 |
| JTDI | PA15 |
| JTDO / TRACESWO | PB3 |
| NJTRST | PB4 |

- 默认（复位后）为 JTAG+SWD；可用软件把 PA13/14/15、PB3/PB4 重定义为普通 GPIO。
- 异步跟踪（TRACESWO）只能与 SW-DP 配合使用，不能用于 JTAG-DP。
- 常用烧录：SWD 两根线 S5PDIO(SWDIO)+CLK(SWCLK) + GND，另需 VDDA。

## 5. 启动配置（RM0008 表6）

BOOT0 为第 44 引脚；BOOT1 复位时复用第 20 引脚（PB2）。复位后第 4 个 SYSCLK 上升沿锁存。

| BOOT1 | BOOT0 | 启动区域 |
|---|---|---|
| X | 0 | 主闪存存储器（正常用户程序） |
| 0 | 1 | 系统存储器（厂商 Bootloader，常用于串口下载） |
| 1 | 1 | 内置 SRAM（调试用） |

## 6. 引脚分配（LQFP48 完整表）

**信号类型**：S=电源，FT=兼容 5V，I=输入，O=输出。ADC12_INx 表示 ADC1_INx 或 ADC2_INx 均可。

### 6.1 电源与特殊引脚

| 引脚号 | 名称 | 类型 | 说明 |
|---|---|---|---|
| 1 | VBAT | S | 后备电池电源（为 RTC/后备寄存器供电，1.8~3.6V） |
| 7 | NRST | I/O | 复位，低电平有效 |
| 8 | VSSA | S | 模拟地 |
| 9 | VDDA | S | 模拟电源（须接 VDD） |
| 23 | VSS_1 | S | 数字地 |
| 24 | VDD_1 | S | 数字电源 2.0~3.6V |
| 35 | VSS_2 | S | 数字地 |
| 36 | VDD_2 | S | 数字电源 |
| 44 | BOOT0 | I | 启动模式选择 |
| 47 | VSS_3 | S | 数字地 |
| 48 | VDD_3 | S | 数字电源 |

### 6.2 时钟 / 复位相关

| 引脚号 | 名称 | 类型 | 复位后主功能 | 复用/重定义 |
|---|---|---|---|---|
| 3 | PC14-OSC32_IN | I/O | PC14 | OSC32_IN（接 32.768kHz） |
| 4 | PC15-OSC32_OUT | I/O | PC15 | OSC32_OUT |
| 5 | PD0-OSC_IN | I | OSC_IN | 重定义 PD0（外接 4~16MHz 晶振） |
| 6 | PD1-OSC_OUT | O | OSC_OUT | 重定义 PD1 |

> PC13/PC14/PC15 由后备区电源开关供电，限流 3mA；同一时刻仅一个可作输出、仅 2MHz 模式、最大负载 30pF，不能当电流源驱动 LED。
> PD0/PD1 复位后为 OSC 功能，软件可重定义为普通 IO（输出模式仅支持 50MHz）。

### 6.3 GPIO 与复用功能（PA/PB 全 37 个 IO）

| 引脚 | 名称(FT) | 复位后 | 可复用功能（AF） | 备注 |
|---|---|---|---|---|
| 2 | PC13-TAMPER-RTC | PC13 | TAMPER / RTC | 后备区，限流 3mA |
| 10 | PA0-WKUP | PA0 | WKUP/USART2_CTS/ADC12_IN0/TIM2_CH1_ETR | 外部中断一路 |
| 11 | PA1 | PA1 | USART2_RTS/ADC12_IN1/TIM2_CH2 | |
| 12 | PA2 | PA2 | USART2_TX/ADC12_IN2/TIM2_CH3 | |
| 13 | PA3 | PA3 | USART2_RX/ADC12_IN3/TIM2_CH4 | |
| 14 | PA4 | PA4 | SPI1_NSS/USART2_CK/ADC12_IN4 | |
| 15 | PA5 | PA5 | SPI1_SCK/ADC12_IN5 | |
| 16 | PA6 | PA6 | SPI1_MISO/ADC12_IN6/TIM3_CH1 | 重定义 TIM1_BKIN |
| 17 | PA7 | PA7 | SPI1_MOSI/ADC12_IN7/TIM3_CH2 | 重定义 TIM1_CH1N |
| 18 | PB0 | PB0 | ADC12_IN8/TIM3_CH3 | 重定义 TIM1_CH2N |
| 19 | PB1 | PB1 | ADC12_IN9/TIM3_CH4 | 重定义 TIM1_CH3N |
| 20 | PB2(FT) | PB2/BOOT1 | — | 复位前 4 周期作 BOOT1 |
| 21 | PB10(FT) | PB10 | I2C2_SCL/USART3_TX | 重定义 TIM2_CH3 |
| 22 | PB11(FT) | PB11 | I2C2_SDA/USART3_RX | 重定义 TIM2_CH4 |
| 25 | PB12(FT) | PB12 | SPI2_NSS/I2C2_SMBAI/USART3_CK/TIM1_BKIN | |
| 26 | PB13(FT) | PB13 | SPI2_SCK/USART3_CTS/TIM1_CH1N | |
| 27 | PB14(FT) | PB14 | SPI2_MISO/USART3_RTS/TIM1_CH2N | |
| 28 | PB15(FT) | PB15 | SPI2_MOSI/TIM1_CH3N | |
| 29 | PA8(FT) | PA8 | USART1_CK/TIM1_CH1/MCO | MCO 时钟输出 |
| 30 | PA9(FT) | PA9 | USART1_TX/TIM1_CH2 | |
| 31 | PA10(FT) | PA10 | USART1_RX/TIM1_CH3 | |
| 32 | PA11(FT) | PA11 | USART1_CTS/USBDM/CAN_RX/TIM1_CH4 | USB DM |
| 33 | PA12(FT) | PA12 | USART1_RTS/USBDP/CAN_TX/TIM1_ETR | USB DP |
| 34 | PA13(FT) | JTMS/SWDIO | — | 重定义 PA13（默认调试） |
| 37 | PA14(FT) | JTCK/SWCLK | — | 重定义 PA14（默认调试） |
| 38 | PA15(FT) | JTDI | — | 重定义 PA15/TIM2_CH1_ETR/SPI1_NSS |
| 39 | PB3(FT) | JTDO | TRACESWO | 重定义 PB3/TIM2_CH2/SPI1_SCK |
| 40 | PB4(FT) | NJTRST | — | 重定义 PB4/TIM3_CH1/SPI1_MISO |
| 41 | PB5 | PB5 | I2C1_SMBAI/SPI1_MOSI/TIM3_CH2 | |
| 42 | PB6(FT) | PB6 | I2C1_SCL/TIM4_CH1 | 重定义 USART1_TX |
| 43 | PB7 | PB7 | I2C1_SDA/TIM4_CH2 | 重定义 USART1_RX |
| 45 | PB8(FT) | PB8 | TIM4_CH3 | 重定义 I2C1_SCL/CAN_RX |
| 46 | PB9(FT) | PB9 | TIM4_CH4 | 重定义 I2C1_SDA/CAN_TX |

## 7. 常用外设引脚规划约束

- **USART1 更常用**（PA9=TX，PA10=RX），PA9/PA10/PA11/PA12 为 FT。
- **I2C1 默认在 PB6/PB7**（软件 GPIO 模拟常用 PB8/PB9）。本库 OLED 用软件 I2C：SCL=PB8，SDA=PB9。
- **SPI1** 标准四线：PA5=SCK，PA6=MISO，PA7=MOSI，PA4=NSS。
- **ADC 采样**优先 PA0~PA7（ADC12_IN0~7），若需更多通道注意 C8 无 PC0~PC7 引脚。
- **TIM1 高级定时器**可输出带死区的 PWM，但 CH 复用会占用 PA8~PA11、PB0/PB1 等。
- **正交编码器**：TIM2（PA0/PA1 或 PA15? 见库内编码器笔记）、TIM3/TIM4；本库电机编码器配置见 [[Encoder.h]] 所在项目归档。
- 5V 兼容（FT）引脚可安全接收 5V 逻辑，非 FT 引脚（如 PA0~PA7、PB0/PB1/PB5/PB7 等）输入需限幅。

## 8. 电气特性注意事项（DS5319）

- 工作电压 2.0~3.6V，普遍 3.3V 供电；内核由内部稳压器提供 1.8V。
- ADC 参考：LQFP48 无 VREF+/- 引脚，VREF 内部连接到 VDDA/VSSA。
- 复位：内部 POR/PDR 在 VDD 达阈值时自动复位，20µs 以上 NRST 低电平触发外部复位。
- I/O 驱动能力与 5V 兼容性以 FT 标记为准；PC13/14/15 输出能力受限（见 6.2）。

## 9. 封装说明

- LQFP48，7×7mm，0 间距，ECOPACK 兼容。
- 本型号无更大的引脚封装复用差异；如需大封装（64/100脚）外设差异见数据手册 DS5319。

## 10. 开发资源

- 参考手册：`00_收件箱/STM32F10xxx参考手册（中文）.pdf`（RM0008）
- 引脚定义表格：[STM32F103C8T6引脚定义.xlsx](STM32F103C8T6引脚定义.xlsx)（同目录）
- 数据手册：ST 官网 DS5319（`STM32F103x8/STM32F103xB`）
- 离线开发：PlatformIO / Arduino STM32 / STM32CubeMX 均可选型该型号

## 11. 待确认与冲突

- [待确认] 所有引脚分配与参数基于手册（RM0008 + DS5319）L0 级，未做上板逐脚验证。
- [冲突待处理] 「PB6/PB7 主功能与重定义」在清华教程表 3.2 中 I2C1_SCL/TIM4_CH1 列为默认复用、USART1_TX 列为重定义，与 ST 官方一致；若遇接线差异以 ST 官方数据手册引脚表为准。

## 12. 相关笔记

- [[STM32F103C8T6]] — 板卡级简卡
- [[OLED显示_软件I2C驱动]] — OLED（SCL=PB8，SDA=PB9）
- [[STM32_GPIO输出_LED与蜂鸣器]] / [[STM32_按键消抖_状态机]] / [[STM32_编码器软件正交解码]] / [[灰度循迹_PID差速控制]]
- 源码归档：`08_归档/2026校赛视觉小车/`