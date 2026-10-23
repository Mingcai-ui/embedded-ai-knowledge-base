---
类型: 知识
状态: 进行中
标签: [freertos, stm32, 任务, 消息队列]
创建: 2026-10-15
---

# FreeRTOS 任务管理与消息队列

> 一句话：任务负责「做事」，队列负责「传数据」；把高频采集和慢速处理解耦，是降低串口丢包的关键。

> 说明：本文为学习要点记录（基于标准 FreeRTOS API），后续可补上本人工程里的实现代码。

## 1. 任务创建与删除

```c
BaseType_t xTaskCreate(
    TaskFunction_t pvTaskCode,      // 任务函数：void f(void *pvParameters)
    const char     *pcName,         // 任务名（调试用）
    uint16_t        usStackDepth,   // 栈深度，单位=字（F1 上 1 字=4 字节），不是字节
    void           *pvParameters,   // 传参
    UBaseType_t     uxPriority,      // 优先级，越大越高
    TaskHandle_t   *pxCreatedTask); // 句柄：删除 / 通知用

void vTaskDelete(TaskHandle_t xTaskToDelete);   // 传 NULL = 删除自己
```

要点：

- 栈深度单位是「字」（4 字节），用 printf / 字符串的任务要留够，不够就 HardFault。
- 创建失败返回 `pdFAIL`（堆不足）。
- 任务函数**不能返回**（返回 = 隐式删除自己）；空闲任务负责回收被删任务的栈。
- `vTaskStartScheduler()` 之后由调度器接管任务。

用法（放 `main()` 初始化一次）：

```c
xTaskCreate(vTaskCollect, "collect", 256, NULL, 3, &xCollectHandle);
xTaskCreate(vTaskProcess, "process", 512, NULL, 2, &xProcessHandle);
vTaskStartScheduler();
```

## 2. 消息队列

```c
QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize);

BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItem, TickType_t xTicksToWait);    // 入队尾
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait);     // 出队头
// 中断里必须用 FromISR 版本：xQueueSendFromISR / xQueueReceiveFromISR
```

要点：

- 队列按**值拷贝**传数据（发的是元素副本）——元素别太大，大块数据改传指针。
- 队列占用 = 长度 × (元素大小 + 结构开销)，从 FreeRTOS 堆分配，注意 heap 是否够。
- `xTicksToWait`：0 = 不等待立即返回；`portMAX_DELAY` = 永久等待（需开 `INCLUDE_vTaskSuspend`）。

用法（`main()` 初始化一次）：

```c
xQueue = xQueueCreate(10, sizeof(SensorData));       // 10 个 SensorData 元素
// 生产者任务里：xQueueSend(xQueue, &d, 0);
// 消费者任务里：xQueueReceive(xQueue, &d, portMAX_DELAY);
```

## 3. 设计要点：用队列降低串口丢包

**丢包场景**：串口中断 / DMA 一批数据进来，若在中断或采集任务里直接做解析、打印、上报这些耗时动作，下一包就来不及处理 → 丢包。

**解法：生产者-消费者解耦**：

- 生产者（串口回调 / 采集任务）：只把数据整包塞进队列，立刻返回去接下一包。
- 消费者（协议解析 / 上报任务）：从队列慢慢取、慢慢处理。
- 队列起「缓冲 + 削峰」作用；再配合**环形缓冲区**形成两级缓冲：
  - 环形缓冲（字节级）：吃掉 UART/DMA 的高频字节流；
  - 消息队列（包级）：把「一整帧」交给处理任务。

**中断安全**：中断里必须用 `FromISR` 版本；判断 `xHigherPriorityTaskWoken`，返回前 `portYIELD_FROM_ISR(xHigherPriorityTaskWoken)`，否则被唤醒的任务要等下一个 tick 才跑。

## 4. 常见坑

- 队列元素按值拷贝 → 传大结构体浪费空间 / CPU；大数据改传指针（注意指针内存生命周期）。
- 栈深度单位搞错 → 任务一跑就栈溢出 / HardFault。
- 中断里用了非 `FromISR` API → 卡死或断言失败。
- 队列满时 `xQueueSend` 会按超时阻塞；采集任务不想被卡就用 0 超时并**统计丢包**（丢包要能观测）。

## 相关

- [[环形缓冲区]] — 字节级缓冲，与队列配合做两级缓冲
- [[UART空闲中断接收不定长数据_DMA方式]]、[[UART空闲中断接收不定长数据_IT方式]] — 丢包场景来源
- 学习节奏：[[嵌入式学习路线]]
