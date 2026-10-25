---
类型: 工具
状态: 进行中
标签: [eide, jlink, cortex-debug, vscode, 调试, stm32]
创建: 2026-10-25
---

# EIDE + JLink 调试环境搭建（VS Code / cortex-debug）

> 2026-10-25 在 UART_Task 工程（STM32F103C8T6 / EIDE 构建）从零配通的实录整理。三个报错都是一次性配置缺失，**换工程还会再踩**，故存档。
> 实操背景与首次单步记录见 [[FreeRTOS_学习进度_2026-10-24]]。

## 1. 最终可用配置（`.vscode/launch.json`）

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "JLink Debug",
            "type": "cortex-debug",
            "request": "launch",
            "cwd": "${workspaceFolder}",
            "executable": "${workspaceFolder}/build/Debug/UART_Task.elf",
            "servertype": "jlink",
            "serverpath": "C:/Program Files/SEGGER/JLink_V934a/JLinkGDBServerCL.exe",
            "device": "STM32F103C8",
            "interface": "swd",
            "armToolchainPath": "C:/Users/17107/.eide/tools/gcc_arm/bin",
            "runToEntryPoint": "main",
            "showDevDebugOutput": "none"
        }
    ]
}
```

要点：`type: cortex-debug`（需装 Cortex-Debug 插件）+ `servertype: jlink` + SWD + `runToEntryPoint: main`。`executable` 指向 EIDE 产物（见 §3）。

## 2. 三个常见报错对照表

| 报错 | 原因 | 解决 |
| --- | --- | --- |
| 找不到 `arm-none-eabi-gdb`（`spawn ENOENT`） | 未指定 `armToolchainPath` | 填 EIDE 自带工具链：`C:/Users/17107/.eide/tools/gcc_arm/bin` |
| 无法启动 JLink GDB Server | 未指定 `serverpath` | 填 `C:/Program Files/SEGGER/JLink_V934a/JLinkGDBServerCL.exe` |
| 无法替换命令变量 `eide.project.build` | `tasks.json` 里 `${command:eide.project.build}` **无返回值**，新版 VS Code 要求返回字符串 | 删掉 `preLaunchTask`，改手动 `Ctrl+Shift+B` 编译后再 F5 |

## 3. 产物路径（EIDE）

由 `.eide/eide.yml` 决定：`outDir: build` + 目标名 `Debug` → `build/Debug/UART_Task.elf`。

| 文件 | 用途 |
| --- | --- |
| `.elf` | 给调试器（含符号表） |
| `.hex` | 给烧录 |
| `.map` | 看内存分布 |

## 4. 单步三键分工（肌肉记忆重点）

| 键 | 行为 | 什么时候用 |
| --- | --- | --- |
| **F10** 单步跳过 | 整个函数当一条指令 | 看自己的逻辑（**默认键，90% 场景**） |
| F11 单步进入 | 钻进函数第一行 | 专门要看库内部 |
| Shift+F11 单步跳出 | 跑完当前函数立刻返回 | 进错了赶紧出来 |

> 踩坑实录：首次实操全程拿 F11 当"走一步"，一次简单的路径检查钻进了五层库函数。**F10 才是默认键**；练习纪律：只在明确要看库内部时按 F11，从库里脱身用 Shift+F11。

## 5. `<optimized out>` 的正确判断

两个原因叠加：

1. **GDB 箭头指向"即将执行的下一行"** → 当前行还没跑，变量自然空；已执行过的行（如上一行的赋值）看得到值。
2. **`-Og`**（EIDE 的 `optimization: level-debug`）下，无稳定栈位置的临时变量被报告为 optimized out。

**判断法**：按一次 F10，变量冒出数值 → 只是"还没执行到"；走完整函数仍是 `<optimized out>` → 才是被优化掉。

## 6. 常用观察手段

- **调试控制台**：`p xTotalHeapSize`、`p xFreeBytesRemaining`（实测剩余堆）、`p pxCurrentTCB->pcTaskName`
- **WATCH 待练**：`pxCurrentTCB->pcTaskName`、`uxCriticalNesting`、`uwTick`
- **Peripherals 待练**：TIM 的 `SR` / `DIER`，NVIC 的 `ISER` / `SHPR`，SCB 的 `ICSR` / `BASEPRI`
- **技巧：变量是快照，寄存器是实时值** —— 停在 HAL 函数里时，局部变量是进函数时对 `SR` 的快照；库代码中途已清标志的话，要看"标志位一直置着不清"这类 bug，必须读寄存器，不能只看变量。

## 7. 尚未覆盖（缺口）

- WATCH / Peripherals / Memory / Disassembly 四块面板一次没打开过（断点 / F5 / F11 / 调用栈 / 局部变量已会）。
- 实验类：冻结时按暂停看现场、`xPortPendSVHandler` 断点看切换时机。

## 相关链接

- [[FreeRTOS_中断管理与临界区]] — 本轮原理笔记（调试器里验证的对象）
- [[FreeRTOS_学习进度_2026-10-24]] — 首次单步路径与堆实测记录
- [[FreeRTOS_索引]] — FreeRTOS 目录入口
