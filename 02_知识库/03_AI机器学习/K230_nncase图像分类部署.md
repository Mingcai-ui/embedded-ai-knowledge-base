---
类型: 知识
状态: 已完成
标签: [K230, CanMV, nncase, 图像分类, 边缘AI, MicroPython]
创建: 2026-08-22
---

# K230 nncase 图像分类部署（CanMV / MicroPython）

## 简介

在 K230 上跑自己训练的图像分类模型（本例：54 类扑克牌识别）的完整链路：**训练导出 kmodel → SD 卡部署 → CanMV 加载 → 摄像头双通道采集 → ai2d 预处理 → KPU 推理 → softmax 后处理 → UART 发结果**。识别结果通过自定义帧协议发给 STM32 做运动控制。

## 部署文件约定

模型文件不放板载 Flash，统一放 SD 卡：

```text
/sdcard/mp_deployment_source/
├── xxx.kmodel              # nncase 编译后的模型
└── deploy_config.json      # 部署配置
```

`deploy_config.json` 关键字段：

```json
{
  "kmodel_path": "扑克牌分类.kmodel",
  "categories": ["2A", "2B", "...", "KingA", "KingB"],
  "confidence_threshold": 0.9,
  "img_size": [224, 224],
  "num_classes": 54
}
```

## 核心流程（main.py 结构）

### 1. 初始化区（只执行一次）

```python
import nncase_runtime as nn
import ulab.numpy as np
from media.sensor import *
from media.display import *
from media.media import *
from machine import UART

# 读部署配置
deploy_conf = read_deploy_config("/sdcard/mp_deployment_source/deploy_config.json")
img_size = deploy_conf["img_size"]

# KPU + ai2d（预处理：resize 到模型输入尺寸）
kpu = nn.kpu()
kpu.load_kmodel("/sdcard/mp_deployment_source/" + deploy_conf["kmodel_path"])
ai2d = nn.ai2d()
ai2d.set_dtype(nn.ai2d_format.NCHW_FMT, nn.ai2d_format.NCHW_FMT, np.uint8, np.uint8)
ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)
ai2d_builder = ai2d.build([1,3,720,1280], [1,3,img_size[0],img_size[1]])
```

### 2. 摄像头双通道（关键概念）

K230 的 Sensor 支持多通道同时输出不同分辨率/格式——**通道 0 给屏幕看，通道 2 给 AI 吃**，互不干扰：

```python
sensor = Sensor()
sensor.reset()
# 通道0：显示用，YUV420，1080p（HDMI）
sensor.set_framesize(width=1920, height=1080)
sensor.set_pixformat(PIXEL_FORMAT_YUV_SEMIPLANAR_420)
# 通道2：AI 用，RGB888 planar，720p
sensor.set_framesize(width=1280, height=720, chn=CAM_CHN_ID_2)
sensor.set_pixformat(PIXEL_FORMAT_RGB_888_PLANAR, chn=CAM_CHN_ID_2)
# 通道0 绑定到显示层
sensor_bind_info = sensor.bind_info(x=0, y=0, chn=CAM_CHN_ID_0)
Display.bind_layer(**sensor_bind_info, layer=Display.LAYER_VIDEO1)
Display.init(Display.LT9611, to_ide=True)   # HDMI；LCD 板用 Display.ST7701
MediaManager.init()
sensor.run()
```

### 3. 推理主循环（while True）

```python
while True:
    rgb888p_img = sensor.snapshot(chn=CAM_CHN_ID_2)       # 取 AI 通道帧
    ai2d_input = rgb888p_img.to_numpy_ref()               # 转 numpy 引用（零拷贝）
    ai2d_input_tensor = nn.from_numpy(ai2d_input)
    ai2d_builder.run(ai2d_input_tensor, ai2d_output_tensor)  # resize 预处理

    kpu.set_input_tensor(0, ai2d_output_tensor)
    kpu.run()                                              # NPU 推理
    output_data = kpu.get_output_tensor(0)
    result = output_data.to_numpy()                        # [1, num_classes]

    # 多分类：softmax 取最大
    softmax_res = softmax(result[0])
    cls_idx = np.argmax(softmax_res)
    if softmax_res[cls_idx] > confidence_threshold:
        send_card_info(labels[cls_idx])                    # UART 发给 STM32

    # OSD 叠加显示结果
    osd_img.clear()
    osd_img.draw_string_advanced(5,5,32,"result:"+labels[cls_idx],color=(0,255,0))
    Display.show_image(osd_img, 0, 0, Display.LAYER_OSD3)
    rgb888p_img = None
    gc.collect()                                           # 每帧回收内存
```

### 4. 退出清理（finally，必须有）

```python
finally:
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    sensor.stop()
    Display.deinit()
    MediaManager.deinit()
    gc.collect()
    nn.shrink_memory_pool()
```

漏了清理会导致下次运行报 buffer 占用错误。

## 设计要点与已知坑

1. **内存紧张**：720p RGB888 一帧约 2.7MB，加上 OSD 和推理 buffer，MicroPython 堆吃紧——每帧 `gc.collect()`；tensor 用完即 `del`。
2. **发送限流**：推理是逐帧的（约 30fps），但 UART 结果 200ms 发一条就够，防止淹没 STM32（见 [[UART自定义帧协议_接收状态机与防误识别]]）。
3. **置信度阈值**：`confidence_threshold` 在 config 里调，本项目 0.9+连续帧确认双保险防误识别。
4. **二分类用 sigmoid，多分类用 softmax**：按 `num_classes` 分支。
5. **镜像/翻转**：`sensor.set_hmirror()` / `set_vflip()` 按摄像头安装方向调。
6. **异常兜底**：整个主循环套 try/except/finally，Ctrl+C 或出错时走清理路径。

## 相关链接

- [[K230开发板档案]]（板卡资源与 CanMV API 速记）
- [[UART自定义帧协议_接收状态机与防误识别]]（发送端协议）
- [[2026校赛_视觉小车]]（项目实例）
