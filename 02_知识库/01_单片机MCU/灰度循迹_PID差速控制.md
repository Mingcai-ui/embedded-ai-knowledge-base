---
类型: 知识
状态: 已完成
标签: [STM32, PID, 灰度循迹, 差速控制, 电机]
创建: 2026-08-22
---

# 灰度循迹 PID 差速控制

## 简介

8 路灰度传感器贴地检测黑线 → **加权平均算位置偏差** → **位置式 PID** → **左右轮差速**修正航向。这是循迹小车的标准控制链，跑在固定周期（本例 40ms）的定时器中断里。

## 信号链

```text
8路灰度(0=黑线,1=白) → 加权平均偏差 → 一阶低通滤波 → 位置式PD → 差速限幅 → 双电机PWM
```

## 1. 偏差计算（Tracking.c）

每路传感器一个权重，**左负右正对称分布**；检测到黑线的传感器求加权平均：

```c
uint8_t TrackState[8];    // 0=检测到黑线，1=白线

float CalculateTrackError(void)
{
    static float last_error = 0;
    TrackSensor_Read();                       // 读 8 路 GPIO
    float weights[8] = {-5.5, -4, -3, -2, 2, 3, 4, 5.5};   // 左负右正

    float sum = 0;
    uint8_t sensor_count = 0;
    for (int i = 0; i < 8; i++) {
        if (TrackState[i] == 0) {             // 压线的传感器才计入
            sum += weights[i];
            sensor_count++;
        }
    }

    if (sensor_count > 0) {
        // 加权平均 + 一阶低通滤波（平滑灰度抖动）
        float position_error = sum / sensor_count;
        position_error = 0.7f * position_error + 0.3f * last_error;
        last_error = position_error;
        return position_error;                // 负=车偏左(线偏右)，正=车偏右
    } else {
        return -1;                            // 丢线：返回最左误差，保持打角找回线
    }
}
```

**要点**：
- 平均而非求和：压线传感器数量变化（1 个 vs 3 个）时偏差尺度一致。
- 0.7/0.3 一阶滤波抑制灰度输出毛刺，代价是引入小延迟。
- 丢线返回固定值（保持上次的打角方向），比返回 0（直行冲出赛道）安全。

## 2. 位置式 PID（PID.c）

```c
typedef struct {
    float Target, Actual, Out;      // 目标 / 实际 / 输出
    float Kp, Ki, Kd;               // 系数
    float Error0, Error1, ErrorInt; // 当前误差 / 上次误差 / 积分累积
    float OutMax, OutMin;           // 输出限幅
} PID_t;

void PID_Update(PID_t *p)
{
    p->Error1 = p->Error0;                  // 保存上次误差
    p->Error0 = p->Target - p->Actual;      // 当前误差
    if (p->Ki != 0) p->ErrorInt += p->Error0;
    else            p->ErrorInt = 0;        // 无积分作用时清零
    if (p->ErrorInt >  400) p->ErrorInt =  400;   // 积分限幅（抗饱和）
    if (p->ErrorInt < -400) p->ErrorInt = -400;

    p->Out = p->Kp * p->Error0
           + p->Ki * p->ErrorInt
           + p->Kd * (p->Error0 - p->Error1);     // 微分项

    if (p->Out > p->OutMax) p->Out = p->OutMax;   // 输出限幅
    if (p->Out < p->OutMin) p->Out = p->OutMin;
}
```

循迹用 **PD 即可**（Ki=0）：位置环有天然的自平衡，积分项反而容易在弯道积累后直道上超调。本项目最终版参数：Kp=2.3、Kd=2.8、限幅 ±600（基础版 1.1/1.3/±400，提速后重调）。

## 3. 差速输出（TIM2 40ms 中断里）

```c
R_pid.Target = 0;                    // 目标：偏差为 0（居中）
R_pid.Actual = CalculateTrackError();
PID_Update(&R_pid);

int16_t base_speed = 300;                       // 基础速度
int16_t diff = R_pid.Out * 20;                  // PID 输出放大为差速量
int16_t max_diff = base_speed * 0.8;            // 差速上限（防止原地转）
if (diff >  max_diff) diff =  max_diff;
if (diff < -max_diff) diff = -max_diff;

L_Speet = base_speed + diff;        // 左轮 = 基速 + 差速
R_Speet = base_speed - diff;        // 右轮 = 基速 - 差速

if (L_Speet < 0) L_Speet = 0;       // 负值截 0（驱动不支持反转时的单向方案）
if (R_Speet < 0) R_Speet = 0;
if (L_Speet > 600) L_Speet = 600;   // 上限 = PWM 满量程
if (R_Speet > 600) R_Speet = 600;

Motor_Left_SetSpeed(L_Speet);       // 写电机（H桥方向 + PWM 占空比）
Motor_Right_SetSpeed(R_Speet);
```

**要点**：
- **差速上限 = 基速 × 0.8**：保证弯道内侧轮仍有余速，不原地 spin。
- **控制周期 40ms 固定**：PID 的 Kd、速度尺度都与周期绑定，改周期必须重调参。
- 本例把负速度截 0 是**单向驱动**的妥协（内侧轮停转）；电机驱动支持反转时保留负值可实现更强纠偏，但要小心速度变量必须为**有符号类型**（见排坑）。

## 调参顺序（实测经验）

1. 先 `Kp` 从小加大，直到直道轻微 S 形摆动的前一刻。
2. 加 `Kd` 抑制摆动（本例 Kd > Kp，灰度噪声大时微分项权重高）。
3. 提速（base_speed ↑）后重调：速度快了同样的 Kp 会导致振荡加剧（本项目提速后 Kp 1.1→2.3、限幅 400→600）。
4. 弯道丢线/出线优先调差速上限和丢线保持策略，而不是猛加 Kp。

## 相关链接

- [[PID灰度循迹_无符号速度变量全速]]（速度变量必须带符号的坑）
- [[直流电机_占空比速度非线性]]（开环占空比与速度非线性）
- [[STM32_编码器软件正交解码]]（里程/圈数测量）
- [[2026校赛_视觉小车]]（项目实例）
