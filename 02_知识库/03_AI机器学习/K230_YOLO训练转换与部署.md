---
类型: 知识
状态: 已完成
标签: [K230, CanMV, YOLO, nncase, 目标检测, 边缘AI, 电赛]
创建: 2026-08-23
来源: 学不会电磁场B站教程蒸馏（核心视频 BV1dvHAzrEkB）+ 转换镜像 .bash_history 一手证据 + 嘉楠官方文档交叉验证
---

# K230 YOLO 模型训练、转换与部署

## 简介

从 YOLO 训练产物到 K230 板端推理的完整流水线：

```text
训练 best.pt → 导出 ONNX → nncase 2.9.0 转换 kmodel → K230 CanMV 部署
```

配套笔记分工：

| 想查什么 | 去哪篇 |
|---|---|
| 转换环境怎么搭（WSL/Docker/SSH/镜像） | [[K230模型转换环境搭建_Docker与SSH连接]] |
| 本篇 | 训练→导出→转换→部署的**流程与命令** |
| 实际模型资产（铁球检测） | [[铁球检测模型_2026电赛]] |
| 分类任务部署（扑克牌，非检测） | [[K230_nncase图像分类部署]] |

## 来源与可信度

| 层级 | 来源 | 可信度 |
|------|------|--------|
| L1 一手证据 | 转换镜像 `k230_model_converter:2.0` 内 `.bash_history` 与 config | 已确认 |
| L2 教程元信息 | B 站核心视频（BV1dvHAzrEkB，33 分钟，播放 6.2 万）、UP 主置顶评论 | 已确认 |
| L3 补充资料 | 嘉楠官方文档、勘智社区、CSDN 等部署教程 | 参考 |

**UP 主「学不会电磁场」**（B 站 mid 568468320，约 5 万粉，专注电赛教学；GitHub：chen-haoshu；资料站：<https://xbhdcc.top/ziliao>）：本知识库所用转换镜像 `k230_model_converter:2.0` 的作者，铁球检测模型即用其镜像转换，两者环境链路同源。其置顶评论明确这套方法**与检测对象无关**——换数据集重训即可识别数字、颜色、动物等任何有明显特征的目标，通用性强。

---

## 一、训练与导出 ONNX

```python
from ultralytics import YOLO
model = YOLO("runs/detect/.../weights/best.pt")
model.export(format="onnx", imgsz=320, opset=12, simplify=True, batch=1)
```

关键约束：
- `imgsz` 必须与训练时一致（铁球检测是 320×320）
- `opset=12`、`simplify=True`、`batch=1` 是 K230/nncase 的稳妥组合

## 二、转换 kmodel

### 2.1 nncase 工具链安装

> 已用 Docker 镜像的话跳过本节（镜像内已装好，见 [[K230模型转换环境搭建_Docker与SSH连接]] 的「镜像内部环境」）。

- **Linux**：`pip install nncase==2.9.0 nncase-kpu==2.9.0` + `sudo apt install dotnet-sdk-7.0`
- **Windows**：nncase 可 pip 在线装，但 **nncase-kpu 需离线 whl**（GitHub Releases 下载 `nncase_kpu-2.9.0-py2.py3-none-win_amd64.whl`）；dotnet-7 自行装并配环境变量
- 配套库：`pip install onnx onnxruntime onnxsim`

### 2.2 转换命令

```bash
# 官方脚本 test_yolov5.zip 解压到 yolov5 目录
python to_kmodel.py --target k230 --model best.onnx \
  --dataset 校准图片目录 --input_width 320 --input_height 320 --ptq_option 0
```

`--ptq_option` 量化策略（NoClip/Kld × 权重/激活位宽组合）：

| 值 | 组合 |
|----|------|
| 0 | NoClip + [uint8, uint8] |
| 1 | NoClip + [uint8, int16] |
| 2 | NoClip + [int16, uint8] |
| 3 | Kld + [uint8, uint8] |
| 4 | Kld + [uint8, int16] |
| 5 | Kld + [int16, uint8] |

- 量化校准集：取训练数据子集（官方推荐约 20%，一般 50–100 张代表图即可）
- 生成的 `.kmodel` 在 ONNX 同级目录

## 三、K230 端部署（CanMV / MicroPython）

- 烧录与 nncase 版本匹配的固件（如 `nncase_v2.9.0`），把 kmodel 拷到 SD 卡
- 用 `libs.YOLO.YOLOv5` 类封装推理：
  ```python
  from libs.YOLO import YOLOv5
  det = YOLOv5(task_type="detect", mode="video",
               kmodel_path="best.kmodel", labels=["qiu"],
               rgb888p_size=[1280,720], display_size=[800,480],
               model_input_size=[320,320], conf_thresh=0.7, nms_thresh=0.7)
  ```
- **检测**（YOLO）与**分类**（softmax）后处理不同：检测输出是框坐标+置信度，需 NMS；分类是 softmax 取最大。详见 [[K230_nncase图像分类部署]] 与 [[铁球检测模型_2026电赛]]

## 四、已知坑与避坑清单（L3 补充）

1. **版本敏感是头号坑**：nncase 与 nncase-kpu 版本必须严格一致，且与开发板固件匹配（2.9.0 对应 2.9.0 固件）。这是绝大多数「转换成功但结果随机」的根因。
2. **PyTorch 2.6+ 加载报错**：`Weights only load failed: Unsupported global` → 导入 torch 前设 `os.environ["TORCH_LOAD_WEIGHTS_ONLY"]="0"`，或 `add_safe_globals` 注册。
3. **GLIBCXX_3.4.30 缺失**（Linux 虚拟环境）：删除 conda 环境里自带的 `libstdc++` 即可。
4. **dotnet 不要装进 Anaconda 环境**：易引发兼容问题，装到系统级。
5. **板端精度下降**：排查顺序 = 校准图是否代表真实场景 → 输入分辨率是否与训练/导出一致 → 颜色通道/归一化是否对齐 → 后处理是否匹配 YOLO 版本/任务。
6. **「转换成功但结果像随机答题」**：几乎都是电脑端与板端**预处理没对齐**（resize/letterbox/归一化/通道顺序）。

## 相关链接

- [[K230模型转换环境搭建_Docker与SSH连接]] — 环境搭建（本篇的容器/工具链来源）
- [[铁球检测模型_2026电赛]] — 实际转换实例（单类检测）
- [[K230_nncase图像分类部署]] — 分类任务部署（后处理不同）
- [[K230开发板档案]] — 板卡资源与 CanMV API
- 官方参考：<https://www.kendryte.com/k230_canmv/zh/main/example/ai/yolo_battle.html>
