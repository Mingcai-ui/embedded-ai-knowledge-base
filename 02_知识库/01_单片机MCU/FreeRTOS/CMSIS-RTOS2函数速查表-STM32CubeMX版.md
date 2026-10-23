---
类型: 知识
状态: 进行中
标签: [freertos, cmsis-rtos2, stm32, cubemx, 速查表, api]
创建: 2026-10-23
---

# CMSIS-RTOS2 函数速查表（STM32CubeMX 版）

> **适用环境**：STM32CubeMX 勾选 FreeRTOS 中间件 + `Interface = CMSIS_V2` 生成的工程
> **配套文档**：`FreeRTOS函数速查表-初学者版.md`（原生 API 版）——本文件第 11 章给出两套 API 的完整对照
> **一句话总结**：你用 `osDelay()`、`osThreadNew()` 写的程序，**跑的就是 FreeRTOS**，只是通过 ARM 定义的标准接口层调用而已。

---

## 目录

- [0. 阅读前必看](#0-阅读前必看)
  - [0.1 三层关系：你写 CMSIS-RTOS2，内核跑 FreeRTOS](#01-三层关系你写-cmsis-rtos2内核跑-freertos)
  - [0.2 CubeMX 生成的工程结构](#02-cubemx-生成的工程结构)
  - [0.3 命名与类型速查](#03-命名与类型速查)
- 
  - [0.4 返回值 osStatus_t](#04-返回值-osstatus_t)
  - [0.5 时间与超时（重点：单位是 tick）](#05-时间与超时重点单位是-tick)
  - [0.6 优先级（重点）](#06-优先级重点)
  - [0.7 ISR 安全性一览（重点，与原生 API 最大差异）](#07-isr-安全性一览重点与原生-api-最大差异)
  - [0.8 CubeMX 默认配置参数](#08-cubemx-默认配置参数)
- [1. 内核控制 API](#1-内核控制-api)
- [2. 线程（任务）管理 API](#2-线程任务管理-api)
- [3. 延时 API](#3-延时-api)
- [4. 软件定时器 API](#4-软件定时器-api)
- [5. 事件标志 API（osEventFlags）](#5-事件标志-apiosflags)
- [6. 互斥量 API](#6-互斥量-api)
- [7. 信号量 API](#7-信号量-api)
- [8. 消息队列 API](#8-消息队列-api)
- [9. 内存池 API](#9-内存池-api)
- [10. CubeMX 风格完整示例](#10-cubemx-风格完整示例)
- [11. CMSIS-RTOS2 ↔ 原生 FreeRTOS 对照总表](#11-cmsis-rtos2--原生-freertos-对照总表)
- [12. CMSIS-RTOS v1 ↔ v2 对照表（看老教程必备）](#12-cmsis-rtos-v1--v2-对照表看老教程必备)
- [13. 初学者最常踩的 12 个坑](#13-初学者最常踩的-12-个坑)
- [14. 学习路线](#14-学习路线)

---

## 0. 阅读前必看

### 0.1 三层关系：你写 CMSIS-RTOS2，内核跑 FreeRTOS

很多初学者疑惑："我学的到底是 FreeRTOS 还是 CMSIS-RTOS？"

答案：**两个都是**。CMSIS-RTOS2 是 ARM 定义的一套**标准 API 规范**（不是操作系统），它给 RTOS 套了一层统一接口。STM32CubeMX 里勾选 FreeRTOS 后，实际生成的代码结构是：

```
你的应用代码
     │  调用  osDelay() / osThreadNew() / osMessageQueuePut() ...
     ▼
CMSIS-RTOS2 适配层   ←  Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c
     │  翻译成  vTaskDelay() / xTaskCreate() / xQueueSend() ...
     ▼
FreeRTOS 内核        ←  tasks.c / queue.c / list.c / timers.c / heap_4.c
     │
     ▼
Cortex-M 移植层      ←  port.c（PendSV / SVC / SysTick）
```

**这带来三个直接结论**：

| 结论 | 说明 |
|---|---|
| 网上 FreeRTOS 教程完全适用 | 只是函数名不同，底层机制（队列/信号量/优先级继承/时间片）一模一样 |
| 两套 API 可以混用 | 都是操作同一个内核对象。`osThreadNew` 创建的任务，用 `vTaskDelete` 也能删 |
| 换 RTOS 时应用层几乎不用改 | 这是 ARM 推出 CMSIS-RTOS 的初衷（可移植性） |

> **建议**：初学者**全程只用一套 API**。既然 CubeMX 生成的是 CMSIS-RTOS2，就以 CMSIS-RTOS2 为主；只有在 CMSIS-RTOS2 没封装的功能（如 `uxTaskGetStackHighWaterMark`、`vTaskList`）时才混用原生 API，并在注释里写明原因。

### 0.2 CubeMX 生成的工程结构

| 文件 / 目录 | 作用 |
|---|---|
| `Core/Src/main.c` | 包含 `osKernelInitialize()` → `MX_FREERTOS_Init()` → `osKernelStart()` 的启动序列 |
| `Core/Src/freertos.c`（部分 CubeMX 版本为 `app_freertos.c`） | `MX_FREERTOS_Init()`：在此创建你在 GUI 里配置的线程、队列、信号量、互斥量、定时器 |
| `Core/Inc/FreeRTOSConfig.h` | 所有 `configXXX` / `INCLUDE_XXX` 宏，**内核行为的唯一开关集中地** |
| `Middlewares/Third_Party/FreeRTOS/Source/` | FreeRTOS 内核源码 |
| `Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c` | CMSIS-RTOS2 适配层实现（**想知道 osXxx 到底干了什么，就读这个文件**） |
| `Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.h` | 所有 `osXxx` 声明、类型、枚举、宏 |

**CubeMX 生成的 main.c 启动序列**（务必理解这个顺序）：

```c
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();          /* 外设初始化：全部在调度器启动之前 */
  MX_USART1_UART_Init();

  /* ---- FreeRTOS 启动三步 ---- */
  osKernelInitialize();    /* 1. 初始化内核（此时还不能创建对象？可以，见下方说明） */
  MX_FREERTOS_Init();      /* 2. 创建线程/队列/信号量（在 freertos.c 里） */
  osKernelStart();         /* 3. 启动调度器，此后 main 不再返回 */

  while (1) { }            /* 正常情况永远到不了这里 */
}
```

**CubeMX 生成的 freertos.c 骨架**：

```c
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,                        /* ★ 单位是字节！128 字 × 4 = 512 字节 */
  .priority = (osPriority_t) osPriorityNormal,  /* ★ 注意是强制转换成 osPriority_t */
};

void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* USER CODE END RTOS_QUEUES */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  for (;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}
```

> **铁律**：所有代码只写在 `/* USER CODE BEGIN xxx */` 和 `/* USER CODE END xxx */` 之间。**这两个注释之外的内容在下次 CubeMX 重新生成代码时会被全部覆盖。**

### 0.3 命名与类型速查

**命名规律**：所有函数以 `os` 开头 + 对象名 + 动词，非常规整。

| 对象 | 前缀 | 句柄类型 | 创建函数 |
|---|---|---|---|
| 线程（任务） | `osThread` | `osThreadId_t` | `osThreadNew` |
| 软件定时器 | `osTimer` | `osTimerId_t` | `osTimerNew` |
| 事件标志 | `osEventFlags` | `osEventFlagsId_t` | `osEventFlagsNew` |
| 互斥量 | `osMutex` | `osMutexId_t` | `osMutexNew` |
| 信号量 | `osSemaphore` | `osSemaphoreId_t` | `osSemaphoreNew` |
| 消息队列 | `osMessageQueue` | `osMessageQueueId_t` | `osMessageQueueNew` |
| 内存池 | `osMemoryPool` | `osMemoryPoolId_t` | `osMemoryPoolNew` |
| 内核 | `osKernel` | — | — |

**类型速查**：

| 类型 | 含义 |
|---|---|
| `osStatus_t` | 几乎所有函数的统一返回值（枚举） |
| `osThreadId_t` / `osMutexId_t` / ... | 各种对象句柄（本质是 `void *`，`NULL` 表示无效） |
| `osThreadFunc_t` | 线程入口函数指针：`void (*)(void *argument)` |
| `osTimerFunc_t` | 定时器回调函数指针：`void (*)(void *argument)` |
| `osPriority_t` | 优先级枚举 |
| `osThreadState_t` | 线程状态枚举 |
| `osTimerType_t` | 定时器类型：`osTimerOnce` / `osTimerPeriodic` |
| `osVersion_t` | 版本号结构体（`osKernelGetInfo` 用） |

**CMSIS-RTOS2 的构造方式统一为"属性结构体"**，这是 v2 相对 v1 最大的改进：

```c
typedef struct {
  const char          *name;         /* 名字，调试用 */
  uint32_t             attr_bits;    /* 属性位（如 osThreadJoinable） */
  void                *cb_mem;       /* 控制块内存（NULL = 自动动态分配） */
  uint32_t             cb_size;
  void                *stack_mem;    /* 栈内存（仅线程有；NULL = 自动分配） */
  uint32_t             stack_size;   /* ★ 字节数 */
  osPriority_t         priority;     /* 优先级 */
  uint32_t             tz_module;    /* TrustZone 模块（一般填 0） */
  uint32_t             reserved;
} osThreadAttr_t;
```

**通用规律**：所有 `osXxxNew()` 的最后一个参数都是 `const osXxxAttr_t *attr`，**传 `NULL` 表示全部使用默认值**（默认走自动动态内存分配）。

### 0.4 返回值 osStatus_t

CMSIS-RTOS2 用统一的枚举做返回值，**不像原生 API 那样返回 `pdTRUE`/`pdFALSE`**：

| 值 | 含义 | 常见触发场景 |
|---|---|---|
| `osOK` | 成功 | — |
| `osError` | 未指明的错误 | 内核未启动、函数在此移植中不支持 |
| `osErrorTimeout` | 等待超时 | `osMutexAcquire` / `osMessageQueueGet` / `osSemaphoreAcquire` 超时 |
| `osErrorResource` | 资源不可用 | 同一线程重复 Acquire 同一个互斥量 |
| `osErrorParameter` | 参数错误 | 句柄为 `NULL`、`flags == 0`、数组越界等 |
| `osErrorNoMemory` | 内存不足 | `osThreadNew` / `osMessageQueueNew` 堆空间不够 |
| `osErrorISR` | **在中断中调用了不允许中断调用的函数** | 在 ISR 里调了 `osMutexAcquire`、`osThreadNew` 等 |
| `osErrorISRRecursive` | 同一 ISR 中重复调用 | 主要用于内核内部 |
| `osErrorValue` | 值超出范围 | 定时器周期为 0 等 |

**返回"数字"而不是 `osStatus_t` 的函数**（初学阶段容易看错）：

| 类型 | 函数示例 | 返回含义 |
|---|---|---|
| 创建函数 | `osThreadNew` `osMessageQueueNew` | 返回**句柄**，失败为 `NULL` |
| 计数类 | `osMessageQueueGetCount` `osThreadGetStackSpace` | 返回 `uint32_t` 数值 |
| 标志类 | `osEventFlagsSet` `osThreadFlagsWait` | 返回**标志位值**，bit31 置位表示出错（`osFlagsError`） |

### 0.5 时间与超时（重点：单位是 tick）

> ⚠️ **这是从"看网上教程"到"自己写代码"最容易翻车的一点。**

**所有 CMSIS-RTOS2 的时间参数单位都是"内核 tick"，不是毫秒。** 只不过 CubeMX 默认 `configTICK_RATE_HZ = 1000`，此时 1 tick = 1 ms，所以 `osDelay(500)` 看起来就像"500 毫秒"，让很多人误以为单位是毫秒。

```c
/* 你的工程若 configTICK_RATE_HZ = 1000（CubeMX 默认）→ osDelay(500) = 500 ms ✓ */
/* 你的工程若 configTICK_RATE_HZ = 100（改过）      → osDelay(500) = 5000 ms ✗ 慢 10 倍 */
```

**保险写法**：先确认节拍频率再写延时。

```c
uint32_t tick_freq = osKernelGetTickFreq();      /* 返回每秒 tick 数，默认 1000 */
/* 用 osKernelGetTickFreq() 换算，或干脆用宏 */
#define DELAY_MS(ms)  ((ms) * osKernelGetTickFreq() / 1000U)
osDelay(DELAY_MS(500));
```

**常量与语义**：

| 常量 / 值 | 含义 |
|---|---|
| `0` | 不等待，立即返回（无论资源是否可用） |
| `1` | 等到下一个 tick（实际等待 0 ~ 1 tick，可能非常短） |
| `n` | 最多等 n 个 tick（因为对齐 tick，实际等待时间为 (n-1) ~ n tick） |
| `osWaitForever` | 无限等待（= `0xFFFFFFFFU`） |

> **重要差异**：原生 FreeRTOS 的 `portMAX_DELAY` **只有在 `INCLUDE_vTaskSuspend = 1` 时才是"真·无限等待"**；而 CMSIS-RTOS2 的 `osWaitForever` 在适配层里也被翻译成 `portMAX_DELAY`，所以**同样受这个开关影响**。CubeMX 默认 `INCLUDE_vTaskSuspend = 1`，一般无需担心。

### 0.6 优先级（重点）

CMSIS-RTOS2 用**抽象优先级枚举**取代了原生 API 的裸数字：

| 枚举值 | 数值 | 含义 |
|---|---|---|
| `osPriorityNone` | 0 | 无优先级 |
| `osPriorityIdle` | 1 | 空闲级（最低） |
| `osPriorityLow` ~ `osPriorityLow7` | 8 ~ 15 | 低 |
| `osPriorityBelowNormal` ~ `..BelowNormal7` | 16 ~ 23 | 低于普通 |
| **`osPriorityNormal`** ~ `..Normal7` | **24 ~ 31** | 普通（**CubeMX 默认**） |
| `osPriorityAboveNormal` ~ `..AboveNormal7` | 32 ~ 39 | 高于普通 |
| `osPriorityHigh` ~ `osPriorityHigh7` | 40 ~ 47 | 高 |
| `osPriorityRealtime` ~ `..Realtime7` | 48 ~ 55 | 实时（最高） |
| `osPriorityISR` | 56 | 中断级（保留） |
| `osPriorityError` | 0x84 | 错误 |

**适配层的映射规则**（FreeRTOS 版 CMSIS-RTOS2 从 v11.0.1 起修正为）：

```
FreeRTOS 实际优先级 = (UBaseType_t)osPriority - 1
```

| 你写的 | 换算后 FreeRTOS 优先级 | 说明 |
|---|---|---|
| `osPriorityNormal` (24) | **23** | 数值越大越优先，和原生 FreeRTOS 方向一致 |
| `osPriorityHigh` (40) | 39 | |
| `osPriorityRealtime` (48) | 47 | 需 `configMAX_PRIORITIES` 足够大 |
| `osPriorityIdle` (1) | 0 | 和空闲任务同级 ⚠️ |

> ⚠️ **CubeMX 默认 `configMAX_PRIORITIES = 56`**，刚好容纳全部 CMSIS 优先级，改小它要慎重。
>
> **如果 `configMAX_PRIORITIES` 被改小**（例如改成 8），所有 ≥ 8 的优先级都会被 FreeRTOS 钳位到 `configMAX_PRIORITIES - 1 = 7`，结果是 **`osPriorityNormal` 和 `osPriorityRealtime` 变成同一个优先级，抢占关系彻底失效**。这是"任务不按预期执行"的隐蔽原因之一。

> ⚠️ **绝对不要把线程设为 `osPriorityIdle`（=1 → FreeRTOS 0）**：那是空闲任务的优先级，该线程几乎永远得不到 CPU 时间。

### 0.7 ISR 安全性一览（重点，与原生 API 最大差异）

**原生 FreeRTOS**：中断里必须用带 `FromISR` 后缀的函数，且必须手动调用 `portYIELD_FROM_ISR()`。

**CMSIS-RTOS2**：**没有 `FromISR` 后缀**。适配层会自己检测是否处于中断上下文，允许的自动走中断安全路径，不允许的直接返回 `osErrorISR`。

**✅ 可以在 ISR 中调用的完整白名单**（ARM 官方规范，务必记牢）：

| 分类 | 可在 ISR 中调用的函数 |
|---|---|
| 内核 | `osKernelGetInfo` `osKernelGetState` `osKernelGetTickCount` `osKernelGetTickFreq` `osKernelGetSysTimerCount` `osKernelGetSysTimerFreq` |
| 线程 | `osThreadGetName` `osThreadGetId` `osThreadFlagsSet` |
| 定时器 | `osTimerGetName` |
| 事件标志 | `osEventFlagsGetName` `osEventFlagsSet` `osEventFlagsClear` `osEventFlagsGet` `osEventFlagsWait`（**仅 timeout = 0**） |
| 互斥量 | `osMutexGetName`（**仅此一个**，互斥量的 Acquire/Release 严禁在 ISR 中用） |
| 信号量 | `osSemaphoreGetName` `osSemaphoreAcquire`（**仅 timeout = 0**）`osSemaphoreRelease` `osSemaphoreGetCount` |
| 内存池 | `osMemoryPoolGetName` `osMemoryPoolAlloc`（timeout = 0）`osMemoryPoolFree` `osMemoryPoolGetCapacity` `osMemoryPoolGetBlockSize` `osMemoryPoolGetCount` `osMemoryPoolGetSpace` |
| 消息队列 | `osMessageQueueGetName` `osMessageQueuePut` `osMessageQueueGet` `osMessageQueueGetCapacity` `osMessageQueueGetMsgSize` `osMessageQueueGetCount` `osMessageQueueGetSpace`（Put/Get 的 timeout 必须为 0） |

**❌ 其余函数（创建类、删除类、挂起恢复、`osDelay`、`osKernelStart` 等）在 ISR 中调用会返回 `osErrorISR`。**

> **好消息**：因为不用写 `portYIELD_FROM_ISR`，CMSIS-RTOS2 在中断里的代码比原生 API **更简洁**：
>
> ```c
> /* 原生 FreeRTOS 写法 */
> void EXTI0_IRQHandler(void) {
>     BaseType_t woken = pdFALSE;
>     xSemaphoreGiveFromISR(xSem, &woken);
>     portYIELD_FROM_ISR(woken);            /* 必须手动切换 */
> }
>
> /* CMSIS-RTOS2 写法 */
> void EXTI0_IRQHandler(void) {
>     osSemaphoreRelease(mySem);            /* 一行搞定，适配层自动处理任务切换 */
> }
> ```
>
> 注意：CMSIS-RTOS2 的中断入口**不需要 `osKernel*` 包裹**，但你的中断优先级数值必须 ≥ `configMAX_SYSCALL_INTERRUPT_PRIORITY`（CubeMX 默认 `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`），否则会触发优先级断言。

### 0.8 CubeMX 默认配置参数

CubeMX 里 FreeRTOS 中间件的 `Parameter Settings` 页各项默认值（以 Cortex-M4 为例）：

| 参数 | CubeMX 默认值 | 说明 |
|---|---|---|
| `configTICK_RATE_HZ` | **1000** | 1 tick = 1 ms |
| `configMAX_PRIORITIES` | **56** | 与 CMSIS 优先级范围匹配 |
| `configMINIMAL_STACK_SIZE` | **128** | 单位：**字**（4 字节）= 512 字节 |
| `configTOTAL_HEAP_SIZE` | 因芯片而异（常见 3072 / 15360 / 65536） | 动态内存池总大小 |
| `configUSE_PREEMPTION` | 1 | 抢占式调度 |
| `configUSE_TIME_SLICING` | 1 | 同优先级时间片轮转 |
| `configUSE_MUTEXES` | 1 | |
| `configUSE_COUNTING_SEMAPHORES` | 1 | |
| `configUSE_TASK_NOTIFICATIONS` | 1 | |
| `configCHECK_FOR_STACK_OVERFLOW` | 0（**建议改 2**） | 栈溢出检测 |
| `configUSE_MALLOC_FAILED_HOOK` | 0（**建议改 1**） | 内存分配失败钩子 |
| `INCLUDE_vTaskSuspend` | 1 | 影响 `osWaitForever` 是否为真无限等待 |
| Heap 方案 | `heap_4` | 支持碎片合并，最通用 |
| 时基源 | SysTick | **启用 FreeRTOS 后 HAL 时基建议改为其他 TIM**（见第 13 章坑 5） |

---

## 1. 内核控制 API

| 函数 | 原型 | 功能 | 参数说明 | 返回值 | 要点 |
|---|---|---|---|---|---|
| `osKernelInitialize` | `osStatus_t osKernelInitialize(void)` | 初始化内核 | 无 | `osOK` / `osError` | CubeMX 已在 main.c 调好，一般不用自己写。**必须在任何创建函数之前调用** |
| `osKernelStart` | `osStatus_t osKernelStart(void)` | 启动调度器 | 无 | `osOK` / `osError` | **必须最后调用**，调用后 main() 不再返回 |
| `osKernelGetState` | `osKernelState_t osKernelGetState(void)` | 查询内核状态 | 无 | `osKernelInactive` / `osKernelReady` / `osKernelRunning` / `osKernelLocked` / `osKernelSuspended` / `osKernelError` | **ISR✅** |
| `osKernelGetTickCount` | `uint32_t osKernelGetTickCount(void)` | 当前节拍计数 | 无 | tick 数 | **ISR✅**。相当于系统"开机毫秒数"（默认配置下） |
| `osKernelGetTickFreq` | `uint32_t osKernelGetTickFreq(void)` | 每秒多少个 tick | 无 | 通常 1000 | **ISR✅**。写延时前先用它确认单位 |
| `osKernelGetSysTimerCount` | `uint32_t osKernelGetSysTimerCount(void)` | 读取底层硬件计数器 | 无 | 计数值 | **ISR✅**。比 tick 更精细，用于高精度计时 |
| `osKernelGetSysTimerFreq` | `uint32_t osKernelGetSysTimerFreq(void)` | 底层计数器频率 | 无 | Hz | **ISR✅** |
| `osKernelGetInfo` | `osStatus_t osKernelGetInfo(osVersion_t *version, char *id_buf, uint32_t id_size)` | 获取内核版本与标识 | ①版本结构体（可 NULL） ②名称缓冲区 ③缓冲区大小 | `osOK` / `osErrorParameter` | **ISR✅**。打印 `"FreeRTOS Vxx"` 用 |
| `osKernelLock` | `int32_t osKernelLock(void)` | 锁定调度器（不关中断） | 无 | 新的锁定计数（≥0）或负错误码 | 保护"多步共享数据操作"，比临界区轻量 |
| `osKernelUnlock` | `int32_t osKernelUnlock(void)` | 解锁调度器 | 无 | 解锁后计数 | 与 `osKernelLock` 成对 |
| `osKernelRestoreLock` | `int32_t osKernelRestoreLock(int32_t lock)` | 恢复到指定锁定状态 | 目标计数 | 之前的状态 | 嵌套锁定场景用 |
| `osKernelSuspend` | `osStatus_t osKernelSuspend(void)` | 挂起内核（低功耗） | 无 | **在 FreeRTOS 移植下返回 `osError`（不支持）** | ⚠️ 常见困惑点：CubeMX 的 FreeRTOS 版**未实现**，别去用它做低功耗 |
| `osKernelResume` | `uint32_t osKernelResume(uint32_t sleep_ticks)` | 恢复内核 | 睡眠 tick | **未实现** | 同上。低功耗请用 `vApplicationIdleHook()` + `__WFI()` |

---

## 2. 线程（任务）管理 API

> CMSIS-RTOS2 把 FreeRTOS 的"任务"叫"线程"（Thread）。**线程函数绝对不能 return**。

### 2.1 创建与身份

| 函数                      | 原型                                                                                          | 功能             | 参数说明                                                         | 返回值                                                                                                                   | 要点                                                                           |
| ----------------------- | ------------------------------------------------------------------------------------------- | -------------- | ------------------------------------------------------------ | --------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------- |
| `osThreadNew`           | `osThreadId_t osThreadNew(osThreadFunc_t func, void *argument, const osThreadAttr_t *attr)` | **创建线程**       | ①线程函数 `void f(void *arg)` ②传给它的参数 ③属性结构体（**可传 `NULL` 用默认值**） | 句柄 / `NULL`（失败）                                                                                                       | **最重要**。属性里 `stack_size` 单位是**字节**；失败一般是 `configTOTAL_HEAP_SIZE` 不够。**ISR❌** |
| `osThreadGetId`         | `osThreadId_t osThreadGetId(void)`                                                          | 获取当前线程句柄       | 无                                                            | 当前线程句柄                                                                                                                | **ISR✅**。若在中断里调用返回 `NULL`                                                    |
| `osThreadGetName`       | `const char *osThreadGetName(osThreadId_t thread_id)`                                       | 获取线程名          | 线程句柄                                                         | 名字字符串                                                                                                                 | **ISR✅**。日志打印用                                                               |
| `osThreadGetState`      | `osThreadState_t osThreadGetState(osThreadId_t thread_id)`                                  | 查询线程状态         | 线程句柄                                                         | `osThreadInactive` / `osThreadReady` / `osThreadRunning` / `osThreadBlocked` / `osThreadTerminated` / `osThreadError` | 调试利器                                                                         |
| `osThreadGetCount`      | `uint32_t osThreadGetCount(void)`                                                           | 线程总数           | 无                                                            | 数量                                                                                                                    |                                                                              |
| `osThreadEnumerate`     | `uint32_t osThreadEnumerate(osThreadId_t *thread_array, uint32_t array_items)`              | 枚举所有线程句柄       | ①句柄数组 ②数组容量                                                  | 实际写入个数                                                                                                                | 自己做"任务监控面板"时用                                                                |
| `osThreadGetStackSize`  | `uint32_t osThreadGetStackSize(osThreadId_t thread_id)`                                     | 线程栈总大小         | 线程句柄                                                         | **字节数**                                                                                                               |                                                                              |
| `osThreadGetStackSpace` | `uint32_t osThreadGetStackSpace(osThreadId_t thread_id)`                                    | 线程栈**历史最小剩余量** | 线程句柄                                                         | **字节数**                                                                                                               | 🔑 **调栈大小必备**。返回个位数 → 快溢出了，必须加大                                              |

### 2.2 删除 / 挂起 / 恢复 / 让出

| 函数                  | 原型                                                     | 功能                 | 参数说明 | 返回值                         | 要点                                                                                                          |
| ------------------- | ------------------------------------------------------ | ------------------ | ---- | --------------------------- | ----------------------------------------------------------------------------------------------------------- |
| `osThreadTerminate` | `osStatus_t osThreadTerminate(osThreadId_t thread_id)` | 终止线程并释放资源          | 线程句柄 | `osOK` / `osErrorParameter` | **ISR❌**。传自己的句柄 = 自杀。⚠️ 用 `osThreadNew` 创建（detached）的线程终止后其资源由空闲任务回收                                        |
| `osThreadExit`      | `__NO_RETURN void osThreadExit(void)`                  | 线程主动退出             | 无    | 不返回                         | 线程函数末尾想结束时的正确姿势（**不要用 `return`，更不要用 `osThreadTerminate(osThreadGetId())`**）                                 |
| `osThreadSuspend`   | `osStatus_t osThreadSuspend(osThreadId_t thread_id)`   | 挂起线程（无限期不参与调度）     | 线程句柄 | `osOK` / `osErrorParameter` | **ISR❌**。挂起 ≠ 删除，内存仍占用；慎用（容易造成资源悬挂）                                                                         |
| `osThreadResume`    | `osStatus_t osThreadResume(osThreadId_t thread_id)`    | 恢复被挂起的线程           | 线程句柄 | `osOK` / `osErrorParameter` | **ISR❌**。⚠️ CMSIS-RTOS2 **没有** `osThreadResumeFromISR`，中断里想唤醒线程请用 `osThreadFlagsSet` 或 `osSemaphoreRelease` |
| `osThreadYield`     | `osStatus_t osThreadYield(void)`                       | 主动让出 CPU           | 无    | `osOK` / `osError`          | **ISR❌**。同优先级线程轮转                                                                                           |
| `osThreadDetach`    | `osStatus_t osThreadDetach(osThreadId_t thread_id)`    | 把线程设为"分离态"（资源自动回收） | 线程句柄 | `osOK` / `osErrorParameter` | `osThreadNew` 默认就是 detached，一般不需要手动调用                                                                       |
| `osThreadJoin`      | `osStatus_t osThreadJoin(osThreadId_t thread_id)`      | 等待线程结束并回收其资源       | 线程句柄 | `osOK` / `osErrorParameter` | 需线程创建时 `attr_bits` 含 `osThreadJoinable`                                                                     |

### 2.3 优先级

| 函数 | 原型 | 功能 | 参数说明 | 返回值 | 要点 |
|---|---|---|---|---|---|
| `osThreadSetPriority` | `osStatus_t osThreadSetPriority(osThreadId_t thread_id, osPriority_t priority)` | 修改线程优先级 | ①句柄（`osThreadGetId()` 可改自己） ②新优先级 | `osOK` / `osErrorParameter` | **ISR❌**。⚠️ 只能传 CMSIS 枚举，别把原生数字混进来 |
| `osThreadGetPriority` | `osPriority_t osThreadGetPriority(osThreadId_t thread_id)` | 读优先级 | 线程句柄 | `osPriority_t` 枚举 | 返回的是**枚举**，不是 FreeRTOS 裸数字 |

### 2.4 线程标志 osThreadFlags（"轻量级信号量"）

> 每线程自带 32 个标志位，**只能被设置给"某一个具体线程"**（一对一），本质就是 FreeRTOS 的**任务通知**。速度快、RAM 省，用于"中断 → 单个线程"的唤醒。

| 函数 | 原型 | 功能 | 参数说明 | 返回值 | 要点 |
|---|---|---|---|---|---|
| `osThreadFlagsSet` | `uint32_t osThreadFlagsSet(osThreadId_t thread_id, uint32_t flags)` | **给指定线程**置标志位 | ①目标线程句柄 ②标志位掩码 | 置位后的完整标志值 | ⭐ **ISR✅！中断里可安全调用**。这是 CMSIS-RTOS2 里中断唤醒线程最推荐的方式 |
| `osThreadFlagsClear` | `uint32_t osThreadFlagsClear(uint32_t flags)` | 清除**当前线程**的标志位 | 标志掩码 | 清除前的值 | **ISR❌**。注意：只能操作自己 |
| `osThreadFlagsGet` | `uint32_t osThreadFlagsGet(void)` | 读取**当前线程**的标志位 | 无 | 当前标志值 | **ISR❌** |
| `osThreadFlagsWait` | `uint32_t osThreadFlagsWait(uint32_t flags, uint32_t options, uint32_t timeout)` | 等待（并消费）标志位 | ①关心的位 ②选项 ③超时 tick | 满足时的标志值 / `osFlagsErrorTimeout` 等 | **ISR❌**。超时返回 `osFlagsErrorTimeout`（bit31 置位） |

**`osThreadFlagsWait` 的 options**：

| 常量 | 值 | 含义 |
|---|---|---|
| `osFlagsWaitAny` | 0x00000000 | 任一位置位即返回（默认） |
| `osFlagsWaitAll` | 0x00000001 | 全部指定位置位才返回 |
| `osFlagsNoClear` | 0x00000002 | 返回后不自动清除标志位 |

> ⚠️ **bit31 保留**：不要用 `0x80000000` 作为标志位，它被用作错误指示（`osFlagsError`）。

---

## 3. 延时 API

| 函数 | 原型 | 功能 | 参数说明 | 返回值 | 要点 |
|---|---|---|---|---|---|
| `osDelay` | `osStatus_t osDelay(uint32_t ticks)` | **相对延时**（从调用时刻开始算） | 延时 tick 数（默认配置下 = 毫秒） | `osOK` / `osErrorISR` | 延时期间线程处于**阻塞态，不消耗 CPU**。**ISR❌** |
| `osDelayUntil` | `osStatus_t osDelayUntil(uint32_t ticks)` | **绝对延时**到指定 tick 时刻 | **目标时刻的绝对 tick 值**（不是延时时长！） | `osOK` / `osErrorISR` | ⭐ **周期性任务必用**，可避免 `osDelay` 造成的周期漂移 |

**`osDelay` 与 `osDelayUntil` 的区别（重点）**：

```c
/* ✗ osDelay：周期会漂移
   假设任务体执行 3 ms，osDelay(10) 后实际周期是 13 ms，误差不断累积 */
void Task_Bad(void *argument)
{
    for (;;)
    {
        Work();            /* 耗时 3 ms */
        osDelay(10);       /* 实际周期 ≈ 13 ms，且随 Work() 耗时变化 */
    }
}

/* ✓ osDelayUntil：周期固定
   传的是"下一个应该醒来的绝对时刻"，任务体耗时波动不影响周期 */
void Task_Good(void *argument)
{
    uint32_t tick = osKernelGetTickCount();      /* 必须先取一次当前时刻 */
    for (;;)
    {
        Work();                                   /* 耗时变化无所谓 */
        tick += 10;                               /* 下一个目标时刻 */
        osDelayUntil(tick);                       /* 睡到那个绝对时刻 */
    }
}
```

> **与 HAL_Delay 的区别**（初学高频疑问）：
> `osDelay()` 会**让出 CPU**，其他线程照常运行，系统保持实时性；
> `HAL_Delay()` 是**忙等**（轮询 SysTick 计数），在这期间**整个 CPU 都被它占着**，所有同/低优先级线程全部卡死。**RTOS 工程里禁止在任务中使用 `HAL_Delay`。**

---

## 4. 软件定时器 API

> CMSIS-RTOS2 的定时器由 FreeRTOS 的**定时器服务任务**统一管理，回调函数在**该任务的上下文**中执行，因此：
> ① **回调里绝对不能阻塞**（不能 `osDelay`、不能带超时地等信号量）；
> ② 需要 `configUSE_TIMERS = 1`（CubeMX 勾选 Timers 后自动配置）；
> ③ 定时器需要在 `osKernelStart()` 之前创建。

| 函数 | 原型 | 功能 | 参数说明 | 返回值 | 要点 |
|---|---|---|---|---|---|
| `osTimerNew` | `osTimerId_t osTimerNew(osTimerFunc_t func, osTimerType_t type, void *argument, const osTimerAttr_t *attr)` | 创建软件定时器 | ①回调函数 `void f(void *arg)` ②`osTimerOnce` / `osTimerPeriodic` ③传给回调的参数 ④属性（可 `NULL`） | 句柄 / `NULL` | 创建后**处于停止状态**，需要 `osTimerStart` |
| `osTimerStart` | `osStatus_t osTimerStart(osTimerId_t timer_id, uint32_t ticks)` | 启动 / 重新启动定时器 | ①句柄 ②周期（**tick 数，不能为 0**） | `osOK` / `osErrorParameter` / `osErrorResource` | **ISR❌**。重复调用 = 重新计时。周期传 0 会返回 `osErrorParameter` |
| `osTimerStop` | `osStatus_t osTimerStop(osTimerId_t timer_id)` | 停止定时器 | 句柄 | `osOK` / `osErrorResource` | **ISR❌**。单次定时器到期后自动停止 |
| `osTimerIsRunning` | `uint32_t osTimerIsRunning(osTimerId_t timer_id)` | 定时器是否在运行 | 句柄 | 1 = 运行中，0 = 停止 | **ISR❌** |
| `osTimerDelete` | `osStatus_t osTimerDelete(osTimerId_t timer_id)` | 删除定时器 | 句柄 | `osOK` / `osErrorParameter` | **ISR❌** |
| `osTimerGetName` | `const char *osTimerGetName(osTimerId_t timer_id)` | 获取定时器名 | 句柄 | 名字 | **ISR✅**（唯一可在 ISR 用的定时器函数） |

> ⚠️ **CMSIS-RTOS2 没有定时器的 ISR 版本函数**。中断里想启动/停止定时器，请改用 `osThreadFlagsSet()` 通知一个线程去操作，或者混用原生 `xTimerStartFromISR()`。

**典型用法**：

```c
osTimerId_t ledTimerId;

static void LedTimerCallback(void *argument)
{
    (void)argument;
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    /* ✗ 严禁在此阻塞：osDelay / 带超时的 Acquire / 阻塞式 printf */
}

void MX_FREERTOS_Init(void)
{
    /* USER CODE BEGIN RTOS_TIMERS */
    ledTimerId = osTimerNew(LedTimerCallback, osTimerPeriodic, NULL, NULL);
    configASSERT(ledTimerId != NULL);
    osTimerStart(ledTimerId, 500);          /* 默认配置下 = 500 ms */
    /* USER CODE END RTOS_TIMERS */
}
```

---

## 5. 事件标志 API（osEventFlags）

> 事件标志组 = 一组 32 位的标志（**bit31 保留**，实际可用 31 位），支持：
> - 一个线程**等多个条件的组合**（"全部满足"或"任一满足"）
> - **一个事件唤醒多个线程**（广播）
> - 中断里置位、多个线程各自等待不同的位组合

| 函数 | 原型 | 功能 | 参数说明 | 返回值 | 要点 |
|---|---|---|---|---|---|
| `osEventFlagsNew` | `osEventFlagsId_t osEventFlagsNew(const osEventFlagsAttr_t *attr)` | 创建事件标志组 | 属性（可 `NULL`） | 句柄 / `NULL` | 创建后所有位为 0。**ISR❌** |
| `osEventFlagsSet` | `uint32_t osEventFlagsSet(osEventFlagsId_t ef_id, uint32_t flags)` | 置位（可一次置多位） | ②要置的位掩码 | 置位后的事件组完整值 | ⭐ **ISR✅！中断里可安全调用** |
| `osEventFlagsClear` | `uint32_t osEventFlagsClear(osEventFlagsId_t ef_id, uint32_t flags)` | 清位 | ②要清的位掩码 | 清除**前**的值 | **ISR✅** |
| `osEventFlagsGet` | `uint32_t osEventFlagsGet(osEventFlagsId_t ef_id)` | 读取当前值 | 句柄 | 当前完整标志值 | **ISR✅** |
| `osEventFlagsWait` | `uint32_t osEventFlagsWait(osEventFlagsId_t ef_id, uint32_t flags, uint32_t options, uint32_t timeout)` | 等待标志位 | ①句柄 ②关心的位 ③选项（见下） ④超时 tick | 满足时的标志值 / `osFlagsErrorTimeout` | **ISR✅ 但仅当 timeout = 0**。非 0 超时在 ISR 中会返回 `osFlagsErrorParameter` |
| `osEventFlagsDelete` | `osStatus_t osEventFlagsDelete(osEventFlagsId_t ef_id)` | 删除事件标志组 | 句柄 | `osOK` / `osErrorParameter` | **ISR❌**。会唤醒所有等待者 |

**`options` 常量**：

| 常量 | 值 | 含义 |
|---|---|---|
| `osFlagsWaitAny` | 0x00000000 | 任一位置位即返回（默认） |
| `osFlagsWaitAll` | 0x00000001 | 全部指定位置位才返回 |
| `osFlagsNoClear` | 0x00000002 | 返回后**不清除**标志位 |

> ⚠️ **两个高频错误**：
> 1. `flags` 参数**不能为 0** → 返回 `osFlagsErrorParameter`。
> 2. 默认行为是"返回后自动清除"这些位。如果你希望标志保持，必须加 `osFlagsNoClear`。

**典型用法**：

```c
#define FLAG_WIFI_READY    (1UL << 0)
#define FLAG_SERVER_READY  (1UL << 1)

osEventFlagsId_t netFlagsId;

/* 线程：等"WiFi 就绪 且 服务器就绪" */
static void UploadTask(void *argument)
{
    for (;;)
    {
        uint32_t flags = osEventFlagsWait(netFlagsId,
                                          FLAG_WIFI_READY | FLAG_SERVER_READY,
                                          osFlagsWaitAll,      /* 全部满足 */
                                          osWaitForever);      /* 一直等 */
        if ((flags & osFlagsError) == 0)      /* ⚠️ 必须检查 bit31 判断是否出错 */
        {
            UploadData();
        }
    }
}

/* 中断里置位：CMSIS-RTOS2 直接调用即可，不需要 FromISR */
void WIFI_IRQHandler(void)
{
    osEventFlagsSet(netFlagsId, FLAG_WIFI_READY);
}
```

---

## 6. 互斥量 API

> 互斥量用于**保护共享资源**（串口、I2C/SPI 总线、全局变量、文件）。它比普通信号量多了**优先级继承**，可缓解优先级翻转。
> ⚠️ **互斥量绝不能在中断中使用**（Acquire/Release 都不行）。

| 函数 | 原型 | 功能 | 参数说明 | 返回值 | 要点 |
|---|---|---|---|---|---|
| `osMutexNew` | `osMutexId_t osMutexNew(const osMutexAttr_t *attr)` | 创建互斥量 | 属性（可 `NULL`） | 句柄 / `NULL` | **创建后即可 Acquire**（这点和信号量不同）。属性位见下表 |
| `osMutexAcquire` | `osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout)` | 获取互斥量（上锁） | ①句柄 ②等待超时 tick | `osOK` / `osErrorTimeout` / `osErrorResource` / `osErrorISR` | **ISR❌**。`osWaitForever` = 一直等 |
| `osMutexRelease` | `osStatus_t osMutexRelease(osMutexId_t mutex_id)` | 释放互斥量（解锁） | 句柄 | `osOK` / `osErrorResource` / `osErrorParameter` | **ISR❌**。必须由**持有者线程**释放 |
| `osMutexGetOwner` | `osThreadId_t osMutexGetOwner(osMutexId_t mutex_id)` | 查询当前持有者 | 句柄 | 线程句柄 / `NULL` | 排查死锁很有用 |
| `osMutexGetName` | `const char *osMutexGetName(osMutexId_t mutex_id)` | 获取名字 | 句柄 | 名字 | **ISR✅**（互斥量中唯一允许在 ISR 调用的） |
| `osMutexDelete` | `osStatus_t osMutexDelete(osMutexId_t mutex_id)` | 删除互斥量 | 句柄 | `osOK` / `osErrorParameter` | **ISR❌**。⚠️ 被线程持有时删除会导致未定义行为 |

**`osMutexAttr_t.attr_bits` 可用属性**：

| 属性 | 含义 | 说明 |
|---|---|---|
| `osMutexRecursive` | 递归互斥量 | 同一线程可重复 Acquire 多次，**Acquire 几次就要 Release 几次** |
| `osMutexPrioInherit` | 优先级继承 | CubeMX 生成代码默认带上（Native FreeRTOS 互斥量天然支持） |
| `osMutexRobust` | 健壮互斥量 | ⚠️ **FreeRTOS 移植不支持**，加了也不生效 |

**典型用法**：

```c
osMutexId_t uartMutexId;
const osMutexAttr_t uartMutex_attr = {
    .name = "uartMutex",
    .attr_bits = osMutexPrioInherit,        /* 优先级继承 */
    .cb_mem = NULL,
    .cb_size = 0U,
};

static void LogTask(void *argument)
{
    for (;;)
    {
        /* 最多等 100 tick，等不到就放弃本次，避免死等 */
        if (osMutexAcquire(uartMutexId, 100U) == osOK)
        {
            printf("Task running...\r\n");     /* 临界资源 */
            osMutexRelease(uartMutexId);       /* 必须成对释放 */
        }
        osDelay(500);
    }
}

void MX_FREERTOS_Init(void)
{
    /* USER CODE BEGIN RTOS_MUTEX */
    uartMutexId = osMutexNew(&uartMutex_attr);
    configASSERT(uartMutexId != NULL);
    /* USER CODE END RTOS_MUTEX */
}
```

> **🔴 死锁预防**：多个互斥量时，约定**全局统一的加锁顺序**，所有线程都按同一顺序拿锁。反向加锁必然死锁。

---

## 7. 信号量 API

> CMSIS-RTOS2 **只用 `osSemaphoreNew` 一个函数**同时覆盖二值信号量和计数信号量，靠参数区分：
> - `osSemaphoreNew(1, 0, NULL)` → **二值信号量**（初始为 0）
> - `osSemaphoreNew(N, M, NULL)` → **计数信号量**（最大 N，初始 M）

| 函数 | 原型 | 功能 | 参数说明 | 返回值 | 要点 |
|---|---|---|---|---|---|
| `osSemaphoreNew` | `osSemaphoreId_t osSemaphoreNew(uint32_t max_count, uint32_t initial_count, const osSemaphoreAttr_t *attr)` | 创建信号量 | ①最大计数 ②初始计数 ③属性（可 `NULL`） | 句柄 / `NULL` | ⚠️ `initial_count` 必须 ≤ `max_count`，且 `max_count` 不能为 0 |
| `osSemaphoreAcquire` | `osStatus_t osSemaphoreAcquire(osSemaphoreId_t semaphore_id, uint32_t timeout)` | 获取信号量（计数 −1） | ①句柄 ②超时 tick | `osOK` / `osErrorTimeout` / `osErrorResource` | ⭐ **ISR✅ 但仅当 timeout = 0** |
| `osSemaphoreRelease` | `osStatus_t osSemaphoreRelease(osSemaphoreId_t semaphore_id)` | 释放信号量（计数 +1） | 句柄 | `osOK` / `osErrorResource`（已满） | ⭐ **ISR✅**。**中断通知任务最常用的函数** |
| `osSemaphoreGetCount` | `uint32_t osSemaphoreGetCount(osSemaphoreId_t semaphore_id)` | 查询当前计数 | 句柄 | 当前计数 | **ISR✅** |
| `osSemaphoreGetName` | `const char *osSemaphoreGetName(osSemaphoreId_t semaphore_id)` | 获取名字 | 句柄 | 名字 | **ISR✅** |
| `osSemaphoreDelete` | `osStatus_t osSemaphoreDelete(osSemaphoreId_t semaphore_id)` | 删除信号量 | 句柄 | `osOK` / `osErrorParameter` | **ISR❌**。也可用于删除互斥量 |

> ⚠️ **二值信号量创建后是"空的"（计数 = 0）**。这和互斥量相反，是最容易踩的坑之一。
> 如果希望任务第一次就能立刻拿到，创建后需要先 `osSemaphoreRelease()` 一次。

**典型用法**：

```c
osSemaphoreId_t keySemId;

/* 中断通知线程：一行搞定，无需 FromISR，无需手动切换任务 */
void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(KEY_Pin);       /* 内部清中断标志 */
    osSemaphoreRelease(keySemId);            /* 计数 +1，自动唤醒等待线程 */
}

/* 等待线程 */
static void KeyTask(void *argument)
{
    for (;;)
    {
        if (osSemaphoreAcquire(keySemId, osWaitForever) == osOK)
        {
            HandleKeyPress();
        }
    }
}

void MX_FREERTOS_Init(void)
{
    /* USER CODE BEGIN RTOS_SEMAPHORES */
    keySemId = osSemaphoreNew(1, 0, NULL);   /* 最大 1、初始 0 = 二值信号量 */
    configASSERT(keySemId != NULL);
    /* USER CODE END RTOS_SEMAPHORES */
}
```

**二值 / 计数 / 互斥量 三者的选择**：

| 需求 | 用哪个 | 创建方式 |
|---|---|---|
| 中断 → 线程单次同步 | 二值信号量 | `osSemaphoreNew(1, 0, NULL)` |
| 统计事件次数（多次累加） | 计数信号量 | `osSemaphoreNew(max, 0, NULL)` |
| 保护共享资源 | 互斥量 | `osMutexNew(&attr)` |
| **只需要唤醒一个固定线程** | 线程标志（更快更省） | `osThreadFlagsSet` + `osThreadFlagsWait` |

---

## 8. 消息队列 API

> 队列用于**线程间传递数据**，采用**值拷贝**（元素被复制进队列，而不是传指针），因此元素大小与队列长度都要在创建时确定。

| 函数 | 原型 | 功能 | 参数说明 | 返回值 | 要点 |
|---|---|---|---|---|---|
| `osMessageQueueNew` | `osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t *attr)` | 创建消息队列 | ①最大消息条数 ②单条消息**字节数** ③属性（可 `NULL`） | 句柄 / `NULL` | **ISR❌**。失败一般是 `configTOTAL_HEAP_SIZE` 不够 |
| `osMessageQueuePut` | `osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout)` | 往队尾发送消息 | ①句柄 ②数据地址（会**拷贝**） ③消息优先级（⚠️ FreeRTOS 移植**忽略此参数**） ④队满时最多等多久 | `osOK` / `osErrorTimeout` / `osErrorResource`（满） | ⭐ **ISR✅ 但仅当 timeout = 0** |
| `osMessageQueueGet` | `osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout)` | 从队头取出消息 | ①句柄 ②接收缓冲区地址 ③消息优先级输出（一般传 `NULL`） ④队空时最多等多久 | `osOK` / `osErrorTimeout` / `osErrorResource`（空） | ⭐ **ISR✅ 但仅当 timeout = 0** |
| `osMessageQueueGetCapacity` | `uint32_t osMessageQueueGetCapacity(osMessageQueueId_t mq_id)` | 队列最大条数 | 句柄 | 容量 | **ISR✅** |
| `osMessageQueueGetMsgSize` | `uint32_t osMessageQueueGetMsgSize(osMessageQueueId_t mq_id)` | 单条消息大小 | 句柄 | 字节数 | **ISR✅** |
| `osMessageQueueGetCount` | `uint32_t osMessageQueueGetCount(osMessageQueueId_t mq_id)` | 当前已排队的消息数 | 句柄 | 条数 | **ISR✅** |
| `osMessageQueueGetSpace` | `uint32_t osMessageQueueGetSpace(osMessageQueueId_t mq_id)` | 剩余空位 | 句柄 | 空位数 | **ISR✅** |
| `osMessageQueueReset` | `osStatus_t osMessageQueueReset(osMessageQueueId_t mq_id)` | 清空队列 | 句柄 | `osOK` | **ISR❌**。只清数据，不删除队列 |
| `osMessageQueueDelete` | `osStatus_t osMessageQueueDelete(osMessageQueueId_t mq_id)` | 删除队列 | 句柄 | `osOK` / `osErrorParameter` | **ISR❌** |
| `osMessageQueueGetName` | `const char *osMessageQueueGetName(osMessageQueueId_t mq_id)` | 获取名字 | 句柄 | 名字 | **ISR✅** |

> ⚠️ **`msg_prio` 参数说明**：CMSIS-RTOS2 规范里允许"按优先级入队"，但**FreeRTOS 移植的底层 `xQueueSend` 是纯 FIFO，该参数被直接忽略**。别指望它实现插队。

**典型用法**：

```c
osMessageQueueId_t sensorQueueId;

typedef struct {
    uint8_t  id;
    int16_t  value;
} SensorMsg_t;

/* 生产者线程 */
static void SensorTask(void *argument)
{
    SensorMsg_t msg;
    for (;;)
    {
        msg.id    = 1;
        msg.value = ReadSensor();
        /* 队列满时最多等 100 tick，等不到就丢弃本帧 */
        osMessageQueuePut(sensorQueueId, &msg, 0U, 100U);
        osDelay(20);
    }
}

/* 消费者线程 */
static void ProcessTask(void *argument)
{
    SensorMsg_t msg;
    for (;;)
    {
        /* 一直等到有数据（阻塞不消耗 CPU） */
        if (osMessageQueueGet(sensorQueueId, &msg, NULL, osWaitForever) == osOK)
        {
            Process(msg);
        }
    }
}

/* 中断中发送：timeout 必须为 0 */
void ADC_IRQHandler(void)
{
    SensorMsg_t msg = { .id = 0, .value = AdcValue };
    osMessageQueuePut(sensorQueueId, &msg, 0U, 0U);
}

void MX_FREERTOS_Init(void)
{
    /* USER CODE BEGIN RTOS_QUEUES */
    sensorQueueId = osMessageQueueNew(10U, sizeof(SensorMsg_t), NULL);
    configASSERT(sensorQueueId != NULL);
    /* USER CODE END RTOS_QUEUES */
}
```

---

## 9. 内存池 API

> 内存池用于**按固定块大小动态分配/释放内存**，避免堆碎片。但要注意：
> ⚠️ **在 CMSIS-FreeRTOS 适配层中，内存池是用 FreeRTOS 队列实现的**（每个块当作一个队列元素），因此实际内存占用**明显高于** `block_count × block_size`，且效率不高。初学者一般用不到，**了解即可**。

| 函数 | 原型 | 功能 | 参数说明 | 返回值 | 要点 |
|---|---|---|---|---|---|
| `osMemoryPoolNew` | `osMemoryPoolId_t osMemoryPoolNew(uint32_t block_count, uint32_t block_size, const osMemoryPoolAttr_t *attr)` | 创建内存池 | ①块数量 ②每块字节数 ③属性 | 句柄 / `NULL` | **ISR❌**。⚠️ FreeRTOS 移植下不支持 `cb_mem` 自定义控制块 |
| `osMemoryPoolAlloc` | `void *osMemoryPoolAlloc(osMemoryPoolId_t mp_id, uint32_t timeout)` | 申请一块内存 | ①句柄 ②超时 tick | 指针 / `NULL`（超时或无块） | **ISR✅ 但仅当 timeout = 0** |
| `osMemoryPoolFree` | `osStatus_t osMemoryPoolFree(osMemoryPoolId_t mp_id, void *block)` | 归还内存块 | ①句柄 ②指针 | `osOK` / `osErrorParameter` | **ISR✅** |
| `osMemoryPoolGetCapacity` | `uint32_t osMemoryPoolGetCapacity(osMemoryPoolId_t mp_id)` | 块总数 | 句柄 | 数量 | **ISR✅** |
| `osMemoryPoolGetBlockSize` | `uint32_t osMemoryPoolGetBlockSize(osMemoryPoolId_t mp_id)` | 每块大小 | 句柄 | 字节数 | **ISR✅** |
| `osMemoryPoolGetCount` | `uint32_t osMemoryPoolGetCount(osMemoryPoolId_t mp_id)` | 已用块数 | 句柄 | 数量 | **ISR✅** |
| `osMemoryPoolGetSpace` | `uint32_t osMemoryPoolGetSpace(osMemoryPoolId_t mp_id)` | 剩余可用块数 | 句柄 | 数量 | **ISR✅** |
| `osMemoryPoolDelete` | `osStatus_t osMemoryPoolDelete(osMemoryPoolId_t mp_id)` | 删除内存池 | 句柄 | `osOK` / `osErrorParameter` | **ISR❌** |
| `osMemoryPoolGetName` | `const char *osMemoryPoolGetName(osMemoryPoolId_t mp_id)` | 获取名字 | 句柄 | 名字 | **ISR✅** |

> 💡 **更实用的替代方案**：CubeMX 工程里直接用 FreeRTOS 原生 `pvPortMalloc()` / `vPortFree()`（`heap_4`，支持碎片合并），或直接定义静态数组 + 队列传指针。

---

## 10. CubeMX 风格完整示例

下面这段代码可以直接放进 CubeMX 工程的 `freertos.c`，演示线程 + 队列 + 信号量 + 定时器 + 互斥量的组合用法：

```c
/* ===== freertos.c 中的用户代码段 ===== */

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* USER CODE BEGIN Variables */
osThreadId_t ledTaskHandle;
osThreadId_t logTaskHandle;
osThreadId_t cmdTaskHandle;

osMessageQueueId_t cmdQueueId;
osSemaphoreId_t    keySemId;
osMutexId_t        uartMutexId;
osTimerId_t        ledTimerId;

typedef struct { uint8_t cmd; uint16_t param; } CmdMsg_t;
/* USER CODE END Variables */

/* ---------- 线程 1：LED 定时闪烁（本质是定时器回调在工作） ---------- */
static void LedTimerCallback(void *argument)
{
    (void)argument;
    /* 定时器回调中不能阻塞，只做极短操作 */
    osThreadFlagsSet(ledTaskHandle, 0x01U);      /* 通知 LED 线程去翻转 */
}

static void LedTask(void *argument)
{
    (void)argument;
    for (;;)
    {
        /* 等标志位，等到就翻转 LED —— 无需轮询 */
        if ((osThreadFlagsWait(0x01U, osFlagsWaitAny, osWaitForever) & osFlagsError) == 0U)
        {
            HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        }
    }
}

/* ---------- 线程 2：按键事件处理（信号量同步） ---------- */
static void CmdTask(void *argument)
{
    CmdMsg_t cmd;
    for (;;)
    {
        if (osSemaphoreAcquire(keySemId, osWaitForever) == osOK)
        {
            /* 从队列取命令并执行 */
            if (osMessageQueueGet(cmdQueueId, &cmd, NULL, 0U) == osOK)
            {
                ExecuteCmd(&cmd);
            }
        }
    }
}

/* ---------- 线程 3：日志输出（互斥量保护串口） ---------- */
static void LogTask(void *argument)
{
    (void)argument;
    for (;;)
    {
        if (osMutexAcquire(uartMutexId, osWaitForever) == osOK)
        {
            printf("[LOG] tick=%lu\r\n", (unsigned long)osKernelGetTickCount());
            osMutexRelease(uartMutexId);
        }
        osDelay(2000);
    }
}

/* ---------- 中断：按键按下 → 发命令 + 释放信号量 ---------- */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == KEY_Pin)
    {
        static CmdMsg_t cmd = { .cmd = 0x01, .param = 0 };
        /* 中断中 Put 的 timeout 必须为 0 */
        osMessageQueuePut(cmdQueueId, &cmd, 0U, 0U);
        /* 中断中安全，无需 FromISR，自动切换任务 */
        osSemaphoreRelease(keySemId);
    }
}

/* ---------- 初始化：在 CubeMX 生成的 MX_FREERTOS_Init 中补齐 ---------- */
void MX_FREERTOS_Init(void)
{
    /* USER CODE BEGIN Init */

    /* --- 1. 属性定义 --- */
    const osThreadAttr_t ledTask_attributes = {
        .name       = "ledTask",
        .stack_size = 256 * 4,                    /* ★ 字节：256 字 = 1024 字节 */
        .priority   = (osPriority_t) osPriorityNormal,
    };
    const osThreadAttr_t cmdTask_attributes = {
        .name       = "cmdTask",
        .stack_size = 256 * 4,
        .priority   = (osPriority_t) osPriorityNormal7,   /* 略高于普通 */
    };
    const osThreadAttr_t logTask_attributes = {
        .name       = "logTask",
        .stack_size = 512 * 4,                    /* printf 栈占用较大，给多点 */
        .priority   = (osPriority_t) osPriorityLow,
    };
    const osMutexAttr_t uartMutex_attributes = {
        .name      = "uartMutex",
        .attr_bits = osMutexPrioInherit,
    };

    /* --- 2. 创建内核对象（必须在 osKernelStart() 之前） --- */
    ledTaskHandle = osThreadNew(LedTask, NULL, &ledTask_attributes);
    cmdTaskHandle = osThreadNew(CmdTask, NULL, &cmdTask_attributes);
    logTaskHandle = osThreadNew(LogTask, NULL, &logTask_attributes);

    cmdQueueId  = osMessageQueueNew(8U, sizeof(CmdMsg_t), NULL);
    keySemId    = osSemaphoreNew(1U, 0U, NULL);        /* 二值信号量，初始 0 */
    uartMutexId = osMutexNew(&uartMutex_attributes);
    ledTimerId  = osTimerNew(LedTimerCallback, osTimerPeriodic, NULL, NULL);

    /* --- 3. 检查创建结果（CubeMX 生成代码不会帮你检查，自己加） --- */
    configASSERT(ledTaskHandle != NULL);
    configASSERT(cmdTaskHandle != NULL);
    configASSERT(logTaskHandle != NULL);
    configASSERT(cmdQueueId    != NULL);
    configASSERT(keySemId      != NULL);
    configASSERT(uartMutexId   != NULL);
    configASSERT(ledTimerId    != NULL);

    /* --- 4. 启动定时器 --- */
    osTimerStart(ledTimerId, 500U);                    /* 默认配置下 = 500 ms */

    /* USER CODE END Init */

    /* CubeMX 自动生成的默认任务创建代码（保留） */
    /* ... defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes); ... */
}
```

---

## 11. CMSIS-RTOS2 ↔ 原生 FreeRTOS 对照总表

> 两套 API 可以混用，操作的是同一批内核对象。**但同一类操作请保持一致，不要来回换算优先级数字。**

### 11.1 内核与调度

| 功能 | CMSIS-RTOS2 | 原生 FreeRTOS |
|---|---|---|
| 初始化内核 | `osKernelInitialize()` | 无（FreeRTOS 无此步骤） |
| 启动调度器 | `osKernelStart()` | `vTaskStartScheduler()` |
| 内核状态 | `osKernelGetState()` | `xTaskGetSchedulerState()` |
| 获取节拍数 | `osKernelGetTickCount()` | `xTaskGetTickCount()` |
| 节拍频率 | `osKernelGetTickFreq()` | `configTICK_RATE_HZ` |
| 锁调度器 | `osKernelLock()` / `osKernelUnlock()` | `vTaskSuspendAll()` / `xTaskResumeAll()` |
| 进入临界区 | 无（用 `osKernelLock` 或直接调原生） | `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` |
| 关中断 | 无 | `taskDISABLE_INTERRUPTS()` / `taskENABLE_INTERRUPTS()` |
| 断言 | `configASSERT` 同 | `configASSERT` |

### 11.2 线程（任务）

| 功能 | CMSIS-RTOS2 | 原生 FreeRTOS |
|---|---|---|
| 创建 | `osThreadNew()`（动态） | `xTaskCreate()` / `xTaskCreateStatic()` |
| 当前句柄 | `osThreadGetId()` | `xTaskGetCurrentTaskHandle()` |
| 任务名 | `osThreadGetName()` | `pcTaskGetName()` |
| 任务状态 | `osThreadGetState()` | `eTaskGetState()` |
| 删除 | `osThreadTerminate(id)` | `vTaskDelete(id)` |
| 自删（正常退出） | `osThreadExit()` | `vTaskDelete(NULL)` |
| 挂起 | `osThreadSuspend()` | `vTaskSuspend()` |
| 恢复 | `osThreadResume()` | `vTaskResume()`（ISR 中用 `xTaskResumeFromISR()`） |
| 让出 CPU | `osThreadYield()` | `taskYIELD()` |
| 设优先级 | `osThreadSetPriority()` | `vTaskPrioritySet()` |
| 读优先级 | `osThreadGetPriority()` | `uxTaskPriorityGet()` |
| **栈历史最小剩余** | `osThreadGetStackSpace()`（**字节**） | `uxTaskGetStackHighWaterMark()`（**字**） |
| 栈总大小 | `osThreadGetStackSize()` | 无对应（自己记录） |
| 线程总数 | `osThreadGetCount()` | `uxTaskGetNumberOfTasks()` |
| 枚举所有线程 | `osThreadEnumerate()` | `uxTaskGetSystemState()` |
| 延时 | `osDelay(ticks)` | `vTaskDelay(ticks)` |
| 周期延时 | `osDelayUntil(ticks)` | `xTaskDelayUntil(&t, inc)` / `vTaskDelayUntil()` |
| 线程标志 | `osThreadFlagsSet/Wait` | 任务通知 `xTaskNotify*` / `ulTaskNotifyTake()` |

### 11.3 通信与同步

| 功能 | CMSIS-RTOS2 | 原生 FreeRTOS |
|---|---|---|
| 二值信号量创建 | `osSemaphoreNew(1, 0, NULL)` | `xSemaphoreCreateBinary()` |
| 计数信号量创建 | `osSemaphoreNew(max, init, NULL)` | `xSemaphoreCreateCounting(max, init)` |
| 获取信号量 | `osSemaphoreAcquire(id, timeout)` | `xSemaphoreTake()`（ISR：`xSemaphoreTakeFromISR()`） |
| 释放信号量 | `osSemaphoreRelease(id)` | `xSemaphoreGive()`（ISR：`xSemaphoreGiveFromISR()`） |
| 信号量计数 | `osSemaphoreGetCount()` | `uxSemaphoreGetCount()` |
| 互斥量创建 | `osMutexNew(&attr)` | `xSemaphoreCreateMutex()` |
| 递归互斥量 | `osMutexNew(&attr)` 且 `attr_bits` 含 `osMutexRecursive` | `xSemaphoreCreateRecursiveMutex()` |
| 上锁 | `osMutexAcquire(id, timeout)` | `xSemaphoreTake()` / `xSemaphoreTakeRecursive()` |
| 解锁 | `osMutexRelease(id)` | `xSemaphoreGive()` / `xSemaphoreGiveRecursive()` |
| 互斥量持有者 | `osMutexGetOwner()` | `xSemaphoreGetMutexHolder()` |
| 队列创建 | `osMessageQueueNew(count, size, NULL)` | `xQueueCreate(len, item_size)` |
| 入队 | `osMessageQueuePut(id, ptr, prio, timeout)` | `xQueueSend()` / `xQueueSendToBack()`（ISR：`xQueueSendFromISR()`） |
| 出队 | `osMessageQueueGet(id, ptr, prio, timeout)` | `xQueueReceive()`（ISR：`xQueueReceiveFromISR()`） |
| 预览不移除 | 无 | `xQueuePeek()` |
| 清空队列 | `osMessageQueueReset()` | `xQueueReset()` |
| 已排队数 | `osMessageQueueGetCount()` | `uxQueueMessagesWaiting()` |
| 剩余空位 | `osMessageQueueGetSpace()` | `uxQueueSpacesAvailable()` |
| 事件标志创建 | `osEventFlagsNew(NULL)` | `xEventGroupCreate()` |
| 置位 | `osEventFlagsSet(id, flags)` | `xEventGroupSetBits()`（ISR：`xEventGroupSetBitsFromISR()`） |
| 清位 | `osEventFlagsClear(id, flags)` | `xEventGroupClearBits()` |
| 读取 | `osEventFlagsGet(id)` | `xEventGroupGetBits()` |
| 等待 | `osEventFlagsWait(id, f, opt, to)` | `xEventGroupWaitBits()` |
| 多任务同步点 | 无（可组合实现） | `xEventGroupSync()` |
| 定时器创建 | `osTimerNew(func, type, arg, NULL)` | `xTimerCreate(...)` |
| 启动/停止/删除 | `osTimerStart/Stop/Delete()` | `xTimerStart/Stop/Delete()`（ISR：`xTimerStartFromISR()` 等） |
| 定时器在跑? | `osTimerIsRunning()` | `xTimerIsTimerActive()` |
| 内存池 | `osMemoryPoolNew/Alloc/Free` | 无直接对应（用 `pvPortMalloc`/`vPortFree` 或自定义） |
| 邮箱（Mailbox） | 无（用内存池 + 队列组合） | 长度 1 的队列 + `xQueueOverwrite()` |

### 11.4 三个必须牢记的差异

| 差异点 | CMSIS-RTOS2 | 原生 FreeRTOS |
|---|---|---|
| **栈大小单位** | **字节** | **字**（32 位机上 1 字 = 4 字节） |
| **中断里怎么写** | 直接调用白名单内的函数（**无 FromISR 后缀**，自动处理任务切换） | 必须用 `xxxFromISR()` + `portYIELD_FROM_ISR()` |
| **返回值** | `osStatus_t` 枚举（`osOK`/`osErrorTimeout`/...） | `pdTRUE`/`pdFALSE`/`pdPASS`/`errQUEUE_FULL`... |
| **优先级** | `osPriority_t` 枚举（24 = Normal） | 裸数字 0 ~ `configMAX_PRIORITIES-1` |

---

## 12. CMSIS-RTOS v1 ↔ v2 对照表（看老教程必备）

> 网上大量 STM32 教程是 2019 年前的，用的是 **CMSIS-RTOS v1**（`osThreadCreate`、`osMessagePut`…）。在 CubeMX 里可以在 `FREERTOS → Interface` 下拉框中切换 `CMSIS_V1` / `CMSIS_V2`。**新工程一律选 CMSIS_V2。**

| 功能 | CMSIS-RTOS v1（老） | CMSIS-RTOS v2（新） |
|---|---|---|
| **线程定义/创建** | `osThreadDef(name, func, prio, instances, stacksz)`<br>`osThreadCreate(osThread(name), arg)` | `osThreadAttr_t attr = {...}`<br>`osThreadNew(func, arg, &attr)` |
| 线程栈大小单位 | 字（4 字节） | **字节** |
| 优先级枚举 | `osPriorityIdle = -3` ~ `osPriorityRealtime = +3`（**负数！**） | `osPriorityIdle = 1` ~ `osPriorityRealtime = 48`（正数） |
| 延时 | `osDelay(ticks)` | `osDelay(ticks)`（相同） |
| 线程让出 | `osThreadYield()` | `osThreadYield()`（相同） |
| 线程终止 | `osThreadTerminate(id)` | `osThreadTerminate(id)`（相同） |
| 挂起/恢复 | `osThreadSuspend/Resume(id)` | `osThreadSuspend/Resume(id)`（相同） |
| 信号 | `osSignalSet(id, sig)`<br>`osSignalWait(sig, timeout)` | `osThreadFlagsSet(id, flags)`<br>`osThreadFlagsWait(flags, options, timeout)` |
| **互斥量定义/创建** | `osMutexDef(name)`<br>`osMutexCreate(osMutex(name))` | `osMutexAttr_t attr = {...}`<br>`osMutexNew(&attr)` |
| 互斥量上锁 | `osMutexWait(id, timeout)` | **`osMutexAcquire(id, timeout)`**（改名） |
| 互斥量解锁 | `osMutexRelease(id)` | `osMutexRelease(id)`（相同） |
| **信号量定义/创建** | `osSemaphoreDef(name)`<br>`osSemaphoreCreate(osSemaphore(name), count)` | `osSemaphoreNew(max_count, initial_count, &attr)` |
| 信号量获取 | `osSemaphoreWait(id, timeout)` | **`osSemaphoreAcquire(id, timeout)`**（改名） |
| 信号量释放 | `osSemaphoreRelease(id)` | `osSemaphoreRelease(id)`（相同） |
| 信号量计数 | `osSemaphoreGetCount(id)` | `osSemaphoreGetCount(id)`（相同） |
| **消息队列定义/创建** | `osMessageQDef(name, queue_sz, type)`<br>`osMessageCreate(osMessageQ(name), NULL)` | `osMessageQueueNew(count, sizeof(type), &attr)` |
| 发消息 | `osMessagePut(id, info, timeout)` | `osMessageQueuePut(id, ptr, prio, timeout)` |
| 收消息 | `osMessageGet(id, timeout)`（返回 `osEvent` 结构体） | `osMessageQueueGet(id, ptr, prio, timeout)` |
| **邮箱（Mail）** | `osMailQDef` / `osMailCreate` / `osMailAlloc` / `osMailPut` / `osMailGet` / `osMailFree` | **v2 取消了邮箱**，改用"内存池 + 消息队列"组合 |
| **内存池** | `osPoolDef` / `osPoolCreate` / `osPoolAlloc` / `osPoolFree` | `osMemoryPoolNew` / `osMemoryPoolAlloc` / `osMemoryPoolFree` |
| **事件标志** | `osEventFlagsDef` / `osEventFlagsCreate`<br>`osEventFlagsSet/Clear/Wait/Get` | `osEventFlagsNew(&attr)`<br>其余同名 |
| 定时器创建 | `osTimerDef(name, func)`<br>`osTimerCreate(osTimer(name), type, arg)` | `osTimerNew(func, type, arg, &attr)` |
| 定时器启停 | `osTimerStart/Stop/Delete(id, ticks)` | `osTimerStart/Stop/Delete(id, ticks)`（相同） |
| 错误码 | `osOK` / `osError` / `osEventTimeout` / `osErrorISR` 等 | `osOK` / `osError` / **`osErrorTimeout`** / `osErrorISR` / `osErrorParameter` / `osErrorNoMemory` / `osErrorResource` |

**v1 → v2 迁移时最容易出的三个错**：

| # | 错误 | 后果 | 修正 |
|---|---|---|---|
| 1 | 把 v1 的栈大小直接搬过来 | v1 的 `128` 是 512 字节，v2 的 `128` 只有 128 字节 → **必然栈溢出** | v2 里写 `128 * 4` |
| 2 | 继续用 `osMutexWait` / `osSemaphoreWait` | 编译报错（未定义） | 改成 `osMutexAcquire` / `osSemaphoreAcquire` |
| 3 | 继续用 v1 的负优先级枚举 | 优先级错乱 | 改用 v2 的正值枚举 |

---

## 13. 初学者最常踩的 12 个坑

| # | 坑 | 现象 | 正确做法 |
|---|---|---|---|
| 1 | **把 tick 当毫秒** | 改了 `configTICK_RATE_HZ` 后所有延时都不对 | 用 `osKernelGetTickFreq()` 确认；或定义 `#define MS(x) ((x)*osKernelGetTickFreq()/1000U)` |
| 2 | **`stack_size` 单位当成"字"** | 栈溢出、莫名 HardFault | `osThreadAttr_t.stack_size` 是**字节**。CubeMX 生成的是 `128 * 4`，照抄即可 |
| 3 | **忽略创建函数的返回值** | 内存不够时静默失败，运行起来才发现线程没跑 | 每次 `osThreadNew` / `osXxxNew` 后加 `configASSERT(handle != NULL)` |
| 4 | **在中断里调用非白名单函数** | 返回 `osErrorISR` 或触发断言 | 记住第 0.7 章白名单；不确定时用 `osThreadFlagsSet` / `osSemaphoreRelease` / `osMessageQueuePut(…,0U)` |
| 5 | **RTOS 任务里用 `HAL_Delay()`** | 其他任务全部卡死，系统失去实时性 | 用 `osDelay()`。CubeMX 启用 FreeRTOS 后，**HAL 时基建议改为其他 TIM**（CubeMX 会提示 `Timebase Source` 冲突） |
| 6 | **`osMessageQueuePut`/`Get` 在中断里传非 0 超时** | 返回 `osErrorParameter` | 中断里第二个时间参数必须写 `0U` |
| 7 | **改了 `configMAX_PRIORITIES`** | 优先级"塌陷"，抢占失效（Normal 和 Realtime 变成同级） | 保持默认 56；非要改小，就要同步降低所有线程的 CMSIS 优先级 |
| 8 | **线程设为 `osPriorityIdle`** | 线程几乎拿不到 CPU | 最低用 `osPriorityLow` 及以上 |
| 9 | **`osEventFlagsWait` 的 flags 传 0，或不检查 bit31** | 返回 `osFlagsErrorParameter`；把错误码当标志值用 | flags 必须非 0；返回值必须先判 `(ret & osFlagsError) == 0` |
| 10 | **线程函数里 `return`** | 进入 HardFault | 用 `for(;;)` 死循环；要退出用 `osThreadExit()` |
| 11 | **在 `osKernelStart()` 之前调用阻塞 API** | 断言失败或行为异常 | `osKernelStart()` 之前只能创建对象，不能 `osDelay`、不能带超时地 Acquire |
| 12 | **代码写在 `USER CODE` 区之外** | 下次 CubeMX 生成代码时全部丢失 | 严格在 `/* USER CODE BEGIN xxx */` 和 `/* USER CODE END xxx */` 之间写 |

**补充建议：一开始就把这两个钩子打开**

CubeMX → FreeRTOS → `Advanced settings` / `Config parameters` 里：

```
configCHECK_FOR_STACK_OVERFLOW = 2    /* 栈溢出检测 */
configUSE_MALLOC_FAILED_HOOK  = 1     /* 内存分配失败钩子 */
```

然后在 `freertos.c` 的用户代码区实现：

```c
/* USER CODE BEGIN Application */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("[FATAL] Stack overflow: %s\r\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

void vApplicationMallocFailedHook(void)
{
    printf("[FATAL] Malloc failed: heap exhausted\r\n");
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}
/* USER CODE END Application */
```

**查看各线程剩余栈空间**（CMSIS-RTOS2 原生支持）：

```c
/* 在某个低优先级监控线程里定期打印 */
extern osThreadId_t ledTaskHandle, cmdTaskHandle, logTaskHandle;

void PrintStackUsage(void)
{
    printf("led: %lu free  cmd: %lu free  log: %lu free\r\n",
           (unsigned long)osThreadGetStackSpace(ledTaskHandle),
           (unsigned long)osThreadGetStackSpace(cmdTaskHandle),
           (unsigned long)osThreadGetStackSpace(logTaskHandle));
    /* 数值持续下降 → 说明该线程栈不够，加大 stack_size */
}
```

---

## 14. 学习路线

| 阶段 | 目标 | 要掌握的 API | 练习 |
|---|---|---|---|
| **第 1 步** | 看懂 CubeMX 生成的代码骨架 | `osKernelInitialize` `osKernelStart` `osThreadNew` `osDelay` | 在 CubeMX 里建两个线程，分别以不同频率闪 LED |
| **第 2 步** | 会用队列传数据 | `osMessageQueueNew/Put/Get` | 串口接收中断 → 队列 → 处理线程 |
| **第 3 步** | 会用信号量做中断同步 | `osSemaphoreNew/Acquire/Release` | 按键中断唤醒线程打印 |
| **第 4 步** | 会保护共享资源 | `osMutexNew/Acquire/Release` | 多线程共享串口打印乱码 → 加互斥量修复 |
| **第 5 步** | 会用定时器 / 事件标志 / 线程标志 | `osTimerNew/Start` `osEventFlags*` `osThreadFlags*` | 周期采集 + 多条件触发上报 |
| **第 6 步** | 会观测与调优 | `osThreadGetStackSpace` `osKernelGetTickCount` + 原生 `vTaskList` | 打印各线程栈余量，优化栈与优先级 |
| **第 7 步** | 打通两套 API | 见第 11 章对照表 | 用原生 `uxTaskGetStackHighWaterMark`、`vTaskList`、`xPortGetMinimumEverFreeHeapSize` 做监控面板 |

**一条总原则**：先用**最简单**的机制解决问题。能用"一个线程 + 一个队列"搞定的事，就别上"信号量 + 事件标志 + 定时器"的组合。RTOS 的复杂度应该是被需求推出来的，不是一开始堆出来的。

---

## 附录：一句话记忆口诀

| 机制 | 一句话 |
|---|---|
| **CMSIS-RTOS2** | ARM 定义的标准接口层，底层还是 FreeRTOS，两套 API 可混用 |
| **时间单位** | 全部是 tick，不是毫秒；CubeMX 默认 1000 Hz 才恰好 1 tick = 1 ms |
| **`osThreadNew`** | 栈大小单位是**字节**（`128 * 4` = 512 字节） |
| **中断里** | 直接用 `osSemaphoreRelease` / `osThreadFlagsSet` / `osMessageQueuePut(…,0U)`，不需要 FromISR |
| **互斥量** | 中断里**绝对不能用**，创建后即可 Acquire |
| **信号量** | `osSemaphoreNew(1, 0, NULL)` 就是二值信号量，**创建后是空的** |
| **队列** | `osMessageQueuePut/Get`，值拷贝，`msg_prio` 参数被忽略 |
| **事件标志** | bit31 保留，默认返回后自动清位；`flags` 不能传 0 |
| **标志返回值** | `osEventFlagsSet/Wait` 返回的是**位值**不是 `osStatus_t`，bit31 置位表示出错 |
| **优先级** | `osPriorityNormal` = 24 → FreeRTOS 优先级 23；别设成 `osPriorityIdle` |
| **返回值** | 创建类返回**句柄**（失败 `NULL`），操作类返回 `osStatus_t` |
| **USER CODE** | 所有代码写在 `/* USER CODE BEGIN/END */` 之间，否则会被覆盖 |

---

> **参考依据**：ARM CMSIS-RTOS2 官方规范（arm-software.github.io/CMSIS_6）、CMSIS-FreeRTOS 版本修订记录、STM32CubeMX 生成代码模板。
> 本表以 **STM32CubeMX + FreeRTOS 10.3.x/10.5.x + CMSIS-RTOS2 适配层** 的通用行为整理；具体默认值（如 `configTOTAL_HEAP_SIZE`）因芯片系列与 CubeMX 版本不同而有差异，请以实际工程的 `FreeRTOSConfig.h` 和 `cmsis_os2.c` 为准。
