# GlassInspection_v3

> 玻璃缺陷在线检测系统（上位机应用）

---

## 简介

`GlassInspection_v3` 是面向玻璃产线质检场景的**上位机应用程序**，集成**线阵相机采图、TensorRT GPU 推理、OpenCV 图像处理、缺陷分级与可视化、多语言界面、数据持久化与授权管理**等完整能力，可对玻璃表面的气泡、划伤、结石、脏污、水渍、镀膜不良等多种缺陷进行自动识别与严重度分级。

本仓库包含应用层的全部源码（`Src/`）、项目工程文件、依赖头文件（`Include/`）与导入库（`Libraries/`）、运行资源（`Resource/`、`Translation/`）、本地配置（`Conifg/`）与示例数据库（`DataBase/`）。

---

## 目录结构

```
GlassInspection_v3/
│
├── Conifg/                         # 运行时配置（INI 格式）
├── DataBase/                       # 本地 SQLite 缺陷/统计数据库（示例 .db）
├── Detect/                         # 推理模型资源目录（*.onnx / *.engine 已被 gitignore）
├── Include/                        # 第三方依赖头文件（QXlsx、MCDLL、highfive/HDF5 等）
├── Libraries/                      # 第三方依赖导入库（.lib）
├── Resource/                       # Qt 资源（图标、配方、按钮皮肤、qrc）
├── Src/                            # 应用源码
│   ├── Algorithm/                  #   算法调度层：检测流程编排、坐标换算、虚拟视图
│   ├── Camera/                     #   相机抽象层：线阵相机（HiKvision / Harmony）适配
│   ├── Global/                     #   全局定义：日志、配置管理、美化工具、消息总线
│   ├── Project/                    #   业务子系统：主流程、数据保存、HDF5/SQLite/JSON、授权
│   ├── ViewUi/                     #   Qt 界面：主窗口、配置、图像显示、表格、自定义控件
│   └── Tuning/                     #   算法调参层：在 DLL 之上提供可现场修改的源码入口
├── Translation/                    # 多语言翻译（.ts 源 / .qm 编译产物）
│
├── GlassInspection_v3.sln          # Visual Studio 解决方案
├── GlassInspection_v3.vcxproj      # VS 工程文件
├── GlassInspection_v3.vcxproj.filters
├── GlassInspection_v3.rc           # Windows 资源文件（图标、版本信息）
├── resource.h
└── main.cpp                        # 程序入口
```

---

## 技术栈

| 依赖 | 版本 | 用途 |
|:---:|:---:|:---|
| C++ | 17 | 开发语言标准 |
| Qt | 5.14.2 | GUI 框架（Widgets + 信号槽 + 多语言） |
| MSVC | v143（VS 2022） | 编译器 |
| OpenCV | 4.8.1 | 图像处理 |
| NVIDIA TensorRT | 8.x / 10.x | GPU 深度学习推理 |
| CUDA | 11.8 | GPU 计算后端 |
| spdlog | — | 日志 |
| QXlsx | — | Excel 报表导出 |
| HighFive / HDF5 | — | 大批量图像/特征持久化 |
| SQLite | — | 缺陷与统计数据存储 |
| HiKvision MVS SDK | — | 线阵相机采集 |
| 运行平台 | Windows x64 | 目标平台 |

---

## 模块说明

### `Src/Algorithm` — 算法调度层
将推理结果（`Object`）与图像处理工具（`GeneralMethod`）、缺陷分级（`defectLevel`）串接起来，负责：图像分块/合并、检测坐标到真实坐标的换算、虚拟大图绘制、二次过滤与排序等。核心文件：`AlgorithmProcess.cpp`、`Compute.cpp`、`showVirtualInfo.cpp`。

### `Src/Camera` — 相机抽象层
基于抽象工厂模式（`CameraFactory`）封装多品牌线阵相机：
- `LineScanCamera_Base`：抽象基类（启停、参数、回调、帧缓冲）
- `LineScanCamera_HK`：海康威视线阵实现
- `LineScanCamera_HR`：海泰/其它厂商实现
- `CameraManager`：多相机统一管理

### `Src/Project` — 业务子系统
- `MainProcess`：贯穿采图→检测→分级→保存的主流程编排
- `DataSave` / `ImageSaver`：图像与缺陷结果落盘
- `HDF5/`、`SQLiteUtils/`、`Json/`：三种数据持久化通道
- `License/`：基于机器指纹的授权管理（`LicenseManager` 单例）
- `Platform/`：与外部平台/PLC 通讯
- `CleanupFiles`：本地缓存清理

### `Src/ViewUi` — Qt 界面
- `MainWindow`：主界面（图像区 + 配置区 + 表格区 + 状态栏）
- `ConfigWidget`：参数配置面板
- `ImageView/DefectRectShow`：图像显示与缺陷框绘制
- `TableView/TableViewWidget`：缺陷实时表格
- `CustomWidget/`：自绘标题栏、工具按钮等

### `Src/Global` — 全局基础设施
日志（`LoggerManager` + spdlog 封装）、配置文件读写（`ConfigManager`）、全局常量与枚举（`Global.h`）、图像美化（`ImageBeautify`）、消息总线（`DisplayMessage`）等。

---

## 应用入口

`main.cpp` 的启动顺序：

1. 注册 Windows 未处理异常过滤器（生成 `full_crash.dmp`，便于现场崩溃定位）
2. 创建 `QApplication`，加载窗口图标
3. 根据配置读取语言项，安装 `QTranslator`（支持 中文 / English / Español / Português / Русский язык）
4. 初始化 spdlog（文件级 / 控制台级可分别配置）
5. **License 校验**：调用 `LicenseManager::validateLicenseStatus()`，未通过则直接退出
6. 创建 `MainWindow` 并最大化显示

> **说明**：仓库**不包含** `License.lic` 文件，未授权机器无法直接运行程序；这是预期的部署形态。

---

## 多语言

`Translation/` 目录提供 5 种语言的 `.ts`（源）与 `.qm`（编译）文件：

| 文件 | 语言 |
|:---:|:---:|
| `GlassInspection_zh_CN` | 简体中文 |
| `GlassInspection_en_US` | English |
| `GlassInspection_es_ES` | Español |
| `GlassInspection_pt_BR` | Português |
| `GlassInspection_ru_RU` | Русский язык |

通过 `Conifg/config.ini` 中 `[System] Language=...` 项切换。

---

## 配置说明

`Conifg/config.ini` 是主配置文件，关键段：

```ini
[System]
Language=中文                ; 中文 / English / Español / Português / Русский язык
LogFileLevel=2              ; spdlog 文件日志级别
LogConsoleLevel=2           ; spdlog 控制台日志级别
```

其它段落用于相机参数、算法阈值、分级规则、平台通讯等。

---

## 模型文件说明（重要）

`Detect/` 目录在运行时存放：
- `GlassAOI.onnx` — YOLO 权重（ONNX 格式）
- `GlassAOI.engine` — TensorRT 编译后的推理引擎

由于 ONNX 文件已超过 GitHub 单文件 100 MB 限制，且 `.engine` 在不同 TensorRT 版本/显卡型号之间不通用，本仓库通过 `.gitignore` **不追踪**这两类文件。运行时需在现场用 ONNX 重新编译 Engine。

---

## 构建说明

### 编译环境
- Windows 10/11 x64
- Visual Studio 2022（MSVC v143）
- Qt 5.14.2（msvc2017_64bit 或兼容版本）
- CUDA 11.8 + TensorRT 8.x（或 10.x）
- OpenCV 4.8.1

### 步骤
1. 用 VS 2022 打开 `GlassInspection_v3.sln`
2. 在项目属性中确认 Qt 版本、CUDA / TensorRT / OpenCV 路径正确
3. 选择 `Release | x64` 配置
4. 生成解决方案

> 应用层的算法核心以独立 DLL 形式提供（`InferTrt.dll`、`SegmentAlgorithm.dll`，分别对应推理引擎与图像处理/缺陷分级）。本项目通过 `Libraries/` 下的导入库与 `Include/` 下的头文件链接它们。

---

## 缺陷类型定义

| 枚举 | 缺陷类型 |
|:---:|:---:|
| `TYPE_POOR_COATING` | 镀膜不良 |
| `TYPE_SCRATCH` | 划伤 |
| `TYPE_CALCULUS` | 结石 |
| `TYPE_BUBBLE` | 气泡 |
| `TYPE_WATER_STAIN` | 水渍 |
| `TYPE_SMUDGE` | 脏污 |
| `TYPE_TRADEMARK` | 商标 |

分级体系：`NORMAL` → `MINOR`（轻微）→ `MEDIUM`（中等）→ `SERIOUS`（严重）。

---

## 许可证

本项目源码为私有，保留所有权利。未经授权不得复制、分发或商用。
