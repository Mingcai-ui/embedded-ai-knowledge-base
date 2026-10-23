# 2026 校赛视觉小车 · 归档说明

> 归档时间：2026-08-22
> 来源：`00_收件箱/` 两个完整 STM32CubeIDE 工程 + K230 main.py
> 对应项目复盘：[[2026校赛_视觉小车]]

## 目录结构

```text
2026校赛视觉小车/
├── 基础版_STM32/      # 用户手搓的基础代码（含 MPU6050，无串口通信）
├── 最终版_STM32/      # AI 协助完善的校赛最终主控代码（USART3 接 K230）
├── K230视觉/main.py   # K230 扑克牌识别 + UART 发送（校赛最终版）
└── README.md
```

## 归档范围说明（重要）

按知识库约定（见 [[AI导航]] 第 5 节），本归档**只含用户代码模块**：

- 已归档：`Core/Inc` 与 `Core/Src` 中的用户模块（Encoder / Key / Motor / OLED / PID / PWM / Tracking / MPU6050）、`Project.ioc`、`platformio.ini`。
- **未归档**（CubeMX 自动生成，不入库）：`main.c`、`main.h`、`gpio.c/h`、`tim.c/h`、`usart.c/h`、`i2c.c/h`、`stm32f1xx_*.c`、`Drivers/`（HAL + CMSIS）、`Startup/`。
- `main.c` USER CODE 区的用户业务逻辑（UART 帧状态机、循迹 PID 控制、编码器解码回调、蜂鸣器任务）**已提取到知识笔记**，见下文链接。
- 如需重建完整可编译工程：用 CubeIDE 打开 `Project.ioc` 重新生成代码，再把本归档模块文件放回 `Core/` 即可（原始完整工程已从收件箱处理）。

## 引脚映射（从 Project.ioc 提取，最终版）

| 功能 | 引脚 | 说明 |
| --- | --- | --- |
| 电机 PWM | PA0 / PA1 | TIM2_CH1 / TIM2_CH2，1kHz，占空比 0~999 |
| 右电机方向 | PA4 / PA5 | IN1 / IN2（H 桥） |
| 左电机方向 | PA6 / PA7 | IN1 / IN2（H 桥） |
| 灰度循迹 IN0~IN3 | PA15 / PA12 / PA10 / PA8 | GPIO 输入，0=黑线 |
| 灰度循迹 IN4~IN7 | PB7 / PB6 / PB4 / PB3 | GPIO 输入，0=黑线 |
| 编码器 A/B 相 | PB14 / PB15 | 上拉输入，TIM3 100μs 中断采样软件解码（基础版为 PB12/PB13） |
| USART3 | PB10(TX) / PB11(RX) | 115200-8N1，接 K230 UART2 |
| 蜂鸣器 | PA9 | 高电平关，低电平响 1s |
| OLED | PB8(SCL) / PB9(SDA) 软件 I2C（江协驱动，见 OLED.c 的 OLED_W_SCL/OLED_W_SDA） | 0.96寸 4 针 I2C |
| SWD | PA13 / PA14 | 调试口 |
| 按键 KEY1/KEY2 | PB0 / PB1 | 输入上拉，按下为低 |

K230 侧（main.py）：UART2 TX=GPIO11、RX=GPIO12，115200。

## 两版差异速览

| 项 | 基础版（手搓） | 最终版（AI 完善） |
| --- | --- | --- |
| 通信 | 无 | USART3 接 K230，帧协议 `AA 55 花色 点数 校验和 0D 0A` |
| MPU6050 | 有（I2C2，角度环尝试） | 移除 |
| 编码器引脚 | PB12/PB13，TIM3 1ms 采样 | PB14/PB15，TIM3 100μs 采样（10 倍采样率） |
| ENCODER_PPR | 5500 | 25160 |
| PID 参数 | Kp=1.1, Kd=1.3, 限幅 ±400 | Kp=2.3, Kd=2.8, 限幅 ±600 |
| 控制周期 | TIM2 30 次计数 | TIM2 40 次计数（40ms） |
| 功能 | 循迹 + OLED 调试显示 | 模式1 循迹2圈 + 模式2 识别扑克牌跑1圈、按键启动、蜂鸣器、停车逻辑 |

## 相关知识笔记

- [[UART自定义帧协议_接收状态机与防误识别]]
- [[K230_nncase图像分类部署]]
- [[STM32_编码器软件正交解码]]
- [[灰度循迹_PID差速控制]]
