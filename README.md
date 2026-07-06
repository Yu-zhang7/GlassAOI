# GlassAOI

> 玻璃缺陷检测算法核心模块

---

## 简介

GlassAOI 是一套面向玻璃制造质检场景的**缺陷检测算法库**，基于 NVIDIA TensorRT GPU 推理引擎与 OpenCV 图像处理体系，实现对玻璃表面**气泡、划伤、结石、脏污、水渍、镀膜不良**等多种缺陷的自动识别、特征提取与严重度分级。

本仓库包含算法层的全部核心源码，可集成至上层检测系统中使用。

---

## 模块总览

| 模块 | 核心类 | 功能描述 |
|:---:|:---:|:---|
| **InferTrt** | `GLASSAOI` | 基于 TensorRT 的 YOLO 目标检测，支持 ONNX→Engine 自动构建、批量 GPU 推理、Hard NMS 后处理 |
| **SegmentAlgorithm** | `GeneralMethod` | 图像处理工具集：缺陷绘制、图像拼接/分割、坐标变换、ROI 裁剪、边缘点提取 |
| | `defectLevel` | 缺陷分级引擎：连通域分析 + 基于面积/长度/周长的多级判定（轻微/中等/严重） |
| | `segImage` | 分割检测：黑边检测、缺陷边缘绘制、缺陷区域面积与对角线计算 |
| | `EightDirectionSobel` | 八方向 Sobel 边缘检测算子（0°~157.5°，间隔 22.5°） |

---

## 目录结构

```
GlassAOI/
│
├── InferTrt/                         # TensorRT 推理模块
│   ├── infertrt.hpp                  # GLASSAOI 类声明 + 模型参数定义
│   └── infertrt.cpp                  # Engine 构建 / 推理 / NMS 实现
│
├── SegmentAlgorithm/                 # 图像处理与缺陷分析模块
│   ├── GeneralMethod.h               # 通用工具类声明
│   ├── GeneralMethod.cpp             # 绘制/拼接/变换/裁剪等实现
│   ├── defectLevel.h                 # 缺陷分级类声明
│   ├── defectLevel.cpp               # 分级逻辑/连通域分析/二次检测
│   ├── segImage.h                    # 分割检测类声明
│   ├── segImage.cpp                  # 黑边检测/边缘绘制/面积计算
│   ├── EightDirectionSobel.h         # 八方向边缘检测声明
│   └── EightDirectionSobel.cpp       # 卷积核生成/多方向响应融合
│
└── README.md
```

---

## 技术栈

| 依赖 | 版本 | 用途 |
|:---:|:---:|:---|
| C++ | 17 | 开发语言标准 |
| NVIDIA TensorRT | 8.x / 10.x | GPU 深度学习推理引擎 |
| CUDA | 11.8 | GPU 计算后端 |
| OpenCV | 4.8.1 | 图像处理基础库 |
| NVIDIA NvInfer / NvOnnxParser | 匹配 TensorRT 版本 | 模型解析与推理 |
| MSVC | v143 (VS 2022) | 编译器 |
| 运行平台 | Windows x64 | 目标平台 |

---

## 核心能力详解

### GLASSAOI — TensorRT 推理引擎

```
ONNX 模型 ──→ buildEngineFromOnnx() ──→ TensorRT Engine
                                              │
输入图像批次 ──→ detect() ──→ GPU 前处理 ──→ 推理 ──→ Hard NMS ──→ 检测结果
```

- 支持 **ONNX → TensorRT Engine** 自动编译与持久化
- 批量推理（最大 batch = 8），输入分辨率 256×256
- 13 类缺陷检测，1344 个 anchor boxes
- 使用 Pinned Memory 加速 H2D/D2H 数据传输
- 兼容 TensorRT 8.x 与 10.x（通过 `NV_TENSORRT_MAJOR` 宏自动适配）

### defectLevel — 缺陷分级

- **连通域分析**：提取缺陷的面积、外接矩形、周长等特征
- **分级判定**：基于 `RankJudgment` 参数对每类缺陷独立配置阈值
- **二次检测**：`secondDetect` / `DoubleChangeErrorType` 支持多通道灰度差分，减少误检
- **等级体系**：`NORMAL` → `MINOR`（轻微）→ `MEDIUM`（中等）→ `SERIOUS`（严重）

### GeneralMethod — 图像处理工具

| 能力 | 关键方法 |
|:---|:---|
| 缺陷可视化 | `drawDefectImage`, `drawResults`, `drawSemiTransparentRedOverlayWithCircle` |
| 图像拼接/分割 | `divideImage`, `MergeImage`, `efficientVerticalConcat` |
| 坐标变换 | `real2Virtual`, `rotateDrawInfoCCW90`, `transformToROICoordinates` |
| ROI 裁剪 | `computeCropRect`, `computeCropRect2Color` |
| 缺陷过滤 | `filterDefectsByBorderRegion`, `filterAndSortDefects`, `mergeDrawInformations` |
| 玻璃区域提取 | `getGlassRect`, `OnlyGetGlassRect` |

### EightDirectionSobel — 八方向边缘检测

8 个方向的 Sobel 卷积核（0°, 22.5°, 45°, 67.5°, 90°, 112.5°, 135°, 157.5°），支持三种输出模式：

| 模式 | 说明 |
|:---:|:---|
| `output_type=0` | 合并所有方向响应 |
| `output_type=1` | 各方向独立结果 |
| `output_type=2` | 最大响应方向 |

---

## 模型参数

```cpp
constexpr int   MAX_BATCH_SIZE   = 8;
constexpr int   MODEL_HEIGHT     = 256;
constexpr int   MODEL_WIDTH      = 256;
constexpr int   NUM_CLASSES      = 13;
constexpr int   NUM_BOXES        = 1344;
constexpr float COMMON_CONFIDENCE = 0.25f;
```

---

## 集成方式

### 方式一：源码集成

将 `InferTrt/` 和 `SegmentAlgorithm/` 中的源文件加入你的工程，配置以下头文件与库搜索路径：

```
IncludePath:  <OpenCV>/include  <TensorRT>/include  <CUDA>/include
LibraryPath:  <OpenCV>/x64/vc16/lib  <TensorRT>/lib  <CUDA>/lib/x64
```

### 方式二：动态库集成

编译为 DLL 后通过导出接口调用：

```cpp
#include "GeneralMethod.h"
#include "defectLevel.h"
#include "infertrt.hpp"

// 初始化推理引擎
GLASSAOI detector;
detector.init("model.onnx", "model.engine");

// 执行检测
std::vector<cv::Mat> images = { img1, img2 };
std::vector<std::vector<Object>> results;
detector.detect(images, results);

// 缺陷分级
defectLevel levelProcessor;
auto graded = levelProcessor.GetDefectLevel(drawInfos, params);
```

---

## 缺陷类型定义

| 枚举值 | 缺陷类型 |
|:---:|:---|
| `TYPE_POOR_COATING` | 镀膜不良 |
| `TYPE_SCRATCH` | 划伤 |
| `TYPE_CALCULUS` | 结石 |
| `TYPE_BUBBLE` | 气泡 |
| `TYPE_WATER_STAIN` | 水渍 |
| `TYPE_SMUDGE` | 脏污 |
| `TYPE_TRADEMARK` | 商标 |

---

## 许可证

本项目为私有代码库，保留所有权利。未经授权不得复制、分发或商用。
