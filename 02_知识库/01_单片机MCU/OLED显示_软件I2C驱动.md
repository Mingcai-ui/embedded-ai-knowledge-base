---
类型: 知识
状态: 进行中
标签: [stm32, oled, i2c, 显示]
创建: 2026-08-21
---

# OLED 0.96寸显示：软件 I2C 驱动

> 来源：江协科技开源 OLED 驱动 V2.0（jiangxiekeji.com，可自由用于项目，版权归江协科技）

## 硬件接线
- 4 针 I2C：`SCL = PB8`、`SDA = PB9`（在 `main.h` 中定义为 `OLED_SCL_Pin` / `OLED_SDA_Pin`）。
- GPIO 配置：**开漏输出(Output Open Drain) + 上拉(Pull-Up)**，即 `gpio.c` 里的 `GPIO_MODE_OUTPUT_OD` + `GPIO_PULLUP`。
- 从机地址：`0x78`。

## 软件 I2C 核心时序（开漏实现的关键）
开漏+上拉 ⇒ 输出 1 = 释放总线，从机可拉低，实现真正的双向通信。

```c
// 起始：SCL 高电平期间 SDA 拉低
OLED_W_SDA(1); OLED_W_SCL(1); OLED_W_SDA(0); OLED_W_SCL(0);
```

```c
// 发送一个字节（高位在前），第9个时钟收 ACK
for (i = 0; i < 8; i++) {
    OLED_W_SDA(!!(Byte & (0x80 >> i)));   // !! 把非零值归一为 1
    OLED_W_SCL(1); OLED_W_SCL(0);
}
OLED_W_SDA(1); OLED_W_SCL(1); OLED_W_SCL(0);  // 第9时钟：ACK
```

```c
// 读一个字节（供 MPU6050 等外部器件复用），Ack: 0=ACK 1=NACK
OLED_W_SDA(1);
for (i = 0; i < 8; i++) {
    OLED_W_SCL(1);
    if (HAL_GPIO_ReadPin(SDA_GPIOX, SDA_PIN)) Byte |= (0x80 >> i);
    OLED_W_SCL(0);
}
```

## 显存模型（重要）
所有显示函数只改显存 `OLED_DisplayBuf[8][128]`（8 页 × 128 列，纵向 8 点为一字节、高位在下）。改完必须调 `OLED_Update()`（全屏）或 `OLED_UpdateArea()`（局部）才真正显示。

## 常用 API
- 初始化：`OLED_Init()`
- 字符串：`OLED_ShowString(x, y, "str", OLED_8X16)`（中文需在 `OLED_Data.c` 的 `OLED_CF16x16` 定义字模，否则显示方框+问号）
- 数字：`OLED_ShowNum / ShowSignedNum / ShowHexNum / ShowBinNum / ShowFloatNum`
- 格式化：`OLED_Printf(x, y, OLED_8X16, "val=%d", n)`
- 绘图：`OLED_DrawLine / DrawRectangle / DrawTriangle / DrawCircle / DrawEllipse / DrawArc`

## 使用示例（重点函数调用方式）

```c
OLED_Init();                                 // 1) 先初始化（I2C 引脚 + 屏幕配置）
OLED_ShowString(0, 0, "Hello", OLED_8X16);   // 2) 写显存
OLED_ShowNum(0, 16, 12345, 5, OLED_8X16);    // 3) 显示数字，Length=占位宽度
OLED_Update();                               // 4) 必须 Update 才真正上屏

while (1) {
    // 动态刷新：改显存 → Update
    OLED_ShowSignedNum(0, 32, (int32_t)val, 4, OLED_8X16);
    OLED_Update();
}
```

| 函数 | 作用 | 要点 |
| --- | --- | --- |
| `OLED_Init()` | 初始化 I2C 引脚与屏幕 | `main()` 里最早调用 |
| `OLED_ShowString(x,y,str,font)` | 显示字符串（中英混排） | 中文需先在 `OLED_Data.c` 定义字模 |
| `OLED_ShowNum(x,y,num,len,font)` | 显示十进制正整数 | `len` 为固定占位宽度，不足补空格 |
| `OLED_Printf(x,y,font,fmt,...)` | 格式化输出（最省事） | 底层 `vsprintf` 到缓冲区再显示 |
| `OLED_Update()` / `OLED_UpdateArea()` | 把显存刷到屏幕 | **所有显示函数后必须调用** |

## 排坑 / 注意
- 1.3 寸屏用 SH1106 驱动（132 列），需在 `OLED_SetCursor` 解除 `X += 2;` 注释。
- 单片机过快时，在 `OLED_W_SCL` / `OLED_W_SDA` 里添加适量延时。

## 相关
- [[STM32_GPIO输出_LED与蜂鸣器]]
- 原始代码：`08_归档/可移植模块代码/OLED/OLED.c` 等