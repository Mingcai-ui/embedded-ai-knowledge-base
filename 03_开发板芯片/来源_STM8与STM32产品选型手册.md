---
类型: 来源
状态: 归档
标签: [stm32, stm8, 选型]
来源: ST 官方《STM8 and STM32 product selection guide》
创建: 2026-09-02
---

# 来源_STM8与STM32产品选型手册

## 出处
- 文档：STM8 和 STM32 产品选型手册（Selection Guide，英文）
- 版本：版本页缺失，共 56 页
- 来源：ST 官方选型手册
- 本地原件：`08_归档/ST官方参考资料/STM8和STM32产品选型手册.pdf`

## 一句话总结
STM8 与 STM32 全系产品的选型总表，方便按主频、Flash、封装、外设快速挑型号。

## 关键要点
- 覆盖 STM8（主流/超低功耗）与 STM32 全系列：
  - STM32 主流型（F0/F1/F2/F3…）、高性能（F4/F7/H7…）、超低功耗（L0/L4/L4+/L5…）
  - STM8 主流 / STM8L（超低功耗）系列
- 每个系列给出 MCU 型号、内核、最大主频、Flash/RAM、封装、关键外设等选型要点。
- 章节目录（示例）：Mainstream MCUs / High-performance MCUs / Ultra-low-power MCUs / STM8 相关系列。
- 适合做 **型号对比与初次选型**，确定型号后再查对应数据手册看详情。

## 与我的关联 / 启发
- 本次归档仅做索引；后续选主控（STM32F103 之外）时来此查可选项。
- 当前主用：STM32F103C8T6（见 [[STM32F103C8T6-芯片资源与PinMux速查]]）。

## 归档去向
- [x] 已归档为来源索引笔记（本文档）
- [ ] 选新 MCU 时据此缩小候选再翻数据手册