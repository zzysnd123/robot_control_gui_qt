# 🤖 robot_control — 多模块无人车协同平台

<p align="center">
  <img src="https://img.shields.io/badge/ROS-Noetic-brightgreen?style=for-the-badge&logo=ros" alt="ROS Noetic">
  <img src="https://img.shields.io/badge/Qt-5.x-41CD52?style=for-the-badge&logo=qt" alt="Qt5">
  <img src="https://img.shields.io/badge/C++-11/14-00599C?style=for-the-badge&logo=c%2B%2B" alt="C++11/14">
  <img src="https://img.shields.io/badge/RViz-Embedded-9cf?style=for-the-badge" alt="RViz">
  <img src="https://img.shields.io/badge/License-Apache%202.0-blue?style=for-the-badge" alt="Apache 2.0">
</p>

<p align="center">
  <b>基于 ROS Noetic + Qt5 + 嵌入式 RViz 的多机器人协同控制 GUI 应用程序</b>
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/zzysnd123/robot_control_gui_qt/main/docs/screenshot.png" width="800" alt="GUI Screenshot" />
</p>

---

## 📖 目录

- [功能特性](#-功能特性)
- [界面预览](#-界面预览)
- [工程结构](#-工程结构)
- [架构设计](#-架构设计)
- [ROS 通信接口](#-ros-通信接口)
- [自定义消息](#-自定义消息)
- [模块配置 (YAML)](#-模块配置-yaml)
- [快速开始](#-快速开始)
- [使用指南](#-使用指南)
- [样式主题](#-样式主题)
- [依赖项](#-依赖项)

---

## ✨ 功能特性

### 🎮 四大功能页面

| 页面 | 图标 | 核心功能 |
|------|:----:|----------|
| **单车操作** | 🚗 | 车辆选择、直线/角速度控制、9 功能按钮（动态映射）、嵌入式 RViz、实时数据反馈 |
| **多车导航** | 🚛 | 车队领航开关、主车选择、协同命令发布、航点获取/保存、双 RViz 窗口 |
| **车辆状态** | 📊 | 全部车辆实时表格监控（名称/状态/CPU/温度/内存/角色/功能模块） |
| **模块管理** | ⚙️ | 功能模块动态 CRUD、9 子功能定义、YAML 持久化 + ROS 网络同步 |

### 🔧 技术亮点

- **模块化架构** — 主窗口协调器 + 4 页面控制器 + RViz 管理器 + YAML 数据管理器，职责清晰
- **线程安全** — `ros::AsyncSpinner(4)` + `Qt::QueuedConnection` 跨线程桥接
- **动态话题切换** — 运行时切换车辆 (robot1/2/3)，自动重订阅对应 ROS 话题
- **数据层分离** — `YamlModuleManager` 纯数据操作，零 ROS 依赖，可独立测试
- **双 RViz 嵌入** — 两个独立 `VisualizationFrame` 嵌入不同页面，各自加载 `.rviz` 配置
- **科技感暗色 UI** — 霓虹青色高亮、发光边框、赛博朋克风格 QSS 主题

---

## 🖥️ 界面预览

```
┌─────────────────────────────────────────────────────┐
│  [单车操作] [多车导航] [车辆状态] [模块管理]          │  ← 左侧导航栏
├─────────────────────────────────────────────────────┤
│                                                     │
│  ┌──────────────────────┐  ┌──────────────────────┐ │
│  │                      │  │  功能名称: 运输模块   │ │
│  │                      │  │  功能介绍: ...        │ │
│  │    RViz 3D 视图      │  │  测量值: 0           │ │
│  │                      │  ├──────────────────────┤ │
│  │                      │  │ [卸货] [开舱门] [重置]│ │  ← 功能按钮 (3×3)
│  │                      │  │ [  ]   [  ]    [  ]  │ │
│  │                      │  │ [  ]   [  ]    [  ]  │ │
│  └──────────────────────┘  ├──────────────────────┤ │
│                            │ 车辆: [robot1 ▼]     │ │
│                            │ 速度: [0.2] m/s      │ │
│                            │ 角速度: [1.0] rad/s  │ │
│                            │ [前进][后退][左][右]  │ │
│                            │      [停止]          │ │
│                            └──────────────────────┘ │
└─────────────────────────────────────────────────────┘
```

---

## 📁 工程结构

```
robot_control/
├── CMakeLists.txt                          # Catkin 构建配置
├── package.xml                             # ROS 包清单
├── res.qrc                                 # Qt 资源索引
├── README.md                               # 本文档
├── .gitignore
├── res/
│   └── style.qss                           # 科技感暗色 QSS 主题
├── function_config/
│   └── modules.yaml                        # 功能模块定义（运行时加载/保存）
├── msg/
│   └── robot_interface.msg                 # 自定义 ROS 消息：车辆状态
├── include/
│   └── robot_control/
│       ├── mainwindow.h                    # 主窗口（精简协调器）
│       ├── rviz_manager.h                  # RViz 双实例生命周期管理
│       ├── yaml_module_manager.h           # YAML 数据管理（纯数据层，无 ROS 依赖）
│       ├── page0_controller.h              # 单车控制页控制器
│       ├── page1_controller.h              # 多车导航页控制器
│       ├── page2_controller.h              # 车辆状态页控制器
│       └── page3_controller.h              # 模块管理页控制器
└── src/
    ├── car_control.cpp                     # 程序入口
    ├── mainwindow.cpp                      # 协调器实现
    ├── mainwindow.ui                       # Qt Designer 布局文件（4 页 QStackedWidget）
    ├── rviz_manager.cpp                    # RViz 配置加载 / 清空 / 初始化
    ├── yaml_module_manager.cpp             # YAML 读写 / 模块 CRUD / 表格填充
    ├── page0_controller.cpp                # 单车运动控制 / 功能按钮 / 车辆切换
    ├── page1_controller.cpp                # 多车导航 / 车队 / 航点 / 位姿
    ├── page2_controller.cpp                # 车辆状态表格（/car_interface）
    └── page3_controller.cpp                # 模块增删改 / YAML 同步
```

---

## 🏗️ 架构设计

### 类关系图

```
car_control.cpp (main)
  │
  └── MainWindow (协调器)
        │
        ├── RvizManager          — RViz 双实例生命周期管理（无 ROS 依赖）
        ├── YamlModuleManager    — 模块数据 CRUD + YAML I/O（无 ROS 依赖）
        │
        ├── Page0Controller      — 单车控制（cmd_vel / function / 车辆切换）
        ├── Page1Controller      — 多车导航（fleet / waypoint / pose / master）
        ├── Page2Controller      — 车辆状态（/car_interface 订阅 → 表格更新）
        └── Page3Controller      — 模块管理（YAML CRUD → /modules_yaml_update）
```

### 设计原则

| 原则 | 说明 |
|------|------|
| **单一职责** | 每个 Controller 仅负责一个页面 |
| **依赖注入** | 构造函数注入 `Ui*`, `NodeHandle&`, `RvizManager*`, `YamlModuleManager*` |
| **数据层分离** | `YamlModuleManager` 零 ROS 依赖，可独立单测 |
| **线程安全** | `AsyncSpinner(4)` → 内部信号 (`Qt::QueuedConnection`) → UI 主线程 |
| **开闭原则** | 新增页面只需添加 Controller，无需修改 MainWindow 核心逻辑 |

### 页面切换流程

```
用户点击导航按钮 (0–3)
  → QButtonGroup::buttonClicked(int)
    → QStackedWidget::setCurrentIndex
    → MainWindow::onPageChanged(index)
      ├─ 切换 RViz 配置（清空旧页 / 加载新页 .rviz）
      └─ 同步速度 / 角速度 spinbox 值
```

---

## 📡 ROS 通信接口

### 全局话题

| 话题 | 类型 | 方向 | 用途 |
|------|------|:--:|------|
| `/open_close_linghang` | `std_msgs/UInt8` | 📤 | 车队领航开关 (1=开启, 0=关闭) |
| `/master_command` | `std_msgs/String` | 📤 | 主车协同命令 |
| `/goal_param_update` | `std_msgs/String` | 📤 | 航点 YAML 更新 |
| `/positions_reload` | `std_msgs/Empty` | 📤 | 通知重载位置列表 |
| `/modules_yaml_update` | `std_msgs/String` | 📤 | 模块配置同步 |
| `/car_interface` | `robot_interface` | 📥 | 所有车辆状态 |

### 车辆动态话题 (`/<car>`)

| 话题模板 | 类型 | 方向 | 用途 |
|----------|------|:--:|------|
| `/<car>/cmd_vel` | `geometry_msgs/Twist` | 📤 | 速度指令 |
| `/<car>_function_name` | `std_msgs/UInt8` | 📥 | 当前功能模块编码 |
| `/<car>_function_data` | `std_msgs/UInt16` | 📥 | 功能返回数据 |
| `/<car>_function_send` | `std_msgs/UInt8` | 📤 | 功能指令下发 |
| `/<car>/amcl_pose` | `PoseWithCovarianceStamped` | 📥 | AMCL 位姿 |

### ROS 参数

| 参数 | 类型 | 默认值 | 用途 |
|------|------|--------|------|
| `/master_robot` | `string` | `"robot1"` | 当前主车名称 |
| `/positions` | `struct` | `{}` | 已保存航点集合 |

---

## 📨 自定义消息

### `robot_interface.msg`

```msg
string   robot_name        # 机器人名称（如 "robot1"）
string   robot_statue       # 状态描述
float32  dianya             # CPU 频率 (V)
float32  wendu              # 温度 (°C)
float32  shidu              # 内存使用率 (%)
uint8    robot_master       # 是否主车 (1=是, 0=否)
string   robot_function     # 搭载功能模块名称
```

---

## 📦 模块配置 (YAML)

配置文件：`function_config/modules.yaml`

```yaml
modules:
  - id: 1
    name: 运输模块
    description: 负责货物运输、重量检测和舱门控制
    functions:
      卸货: 4
      打开舱门: 3
      显示重量: 2
      重置: 5
  - id: 2
    name: 叉车模块
    description: 负责将卸载的货物放在指定的货架上
    functions:
      停止叉子移动: 7
      叉子上升: 4
      叉子下降: 5
      回到最低位置: 6
```

> **注意：** `id: 0` (无模块) 为系统保留项，不在界面显示且不可修改。

---

## 🚀 快速开始

### 环境要求

| 组件 | 版本 / 说明 |
|------|-------------|
| Ubuntu | 20.04 |
| ROS | Noetic (Desktop-Full) |
| Qt | 5.x (Widgets, Core, Gui) |
| yaml-cpp | `libyaml-cpp-dev` |
| 编译器 | GCC 9+, C++11 |

### 安装依赖

```bash
# 系统依赖
sudo apt install libyaml-cpp-dev

# 加载 ROS 环境
source /opt/ros/noetic/setup.bash
```

### 克隆与构建

```bash
# 克隆仓库
cd ~/QTROSworkspaces/catkin_ws/src
git clone git@github.com:zzysnd123/robot_control_gui_qt.git

# 构建
cd ~/QTROSworkspaces/catkin_ws
catkin build robot_control

# 加载环境
source devel/setup.bash
```

### 运行

```bash
# 终端 1：启动 ROS Master
roscore

# 终端 2：启动控制界面
rosrun robot_control car_control
```

### RViz 配置准备

将 `.rviz` 配置文件放置于 `~/桌面/` 目录：

| 文件 | 用途 |
|------|------|
| `rviz_robot1.rviz` | 单车控制页默认 RViz |
| `rviz_robot2.rviz` | robot2 切换时加载 |
| `rviz_robot3.rviz` | robot3 切换时加载 |
| `rviz2.rviz` | 多车导航页默认 RViz |

---

## 📘 使用指南

### 🚗 单车操作页

| 操作 | 方法 |
|------|------|
| 选择车辆 | 下拉框切换 robot1 / robot2 / robot3 |
| 运动控制 | **按住** 方向按钮运动，**松开** 自动停止 |
| 速度调节 | `直线速度` / `角速度` spinbox，步长 0.1，最大 1.4 m/s |
| 功能执行 | 9 个按钮自动映射当前模块子功能，点击发送指令 |
| 数据监控 | `功能名称` / `功能介绍` / `测量值` 区域实时更新 |

### 🚛 多车导航页

| 操作 | 方法 |
|------|------|
| 车队领航 | 点击 `开启车队` 按钮切换模式，状态实时显示 |
| 主车指派 | `当前主车` 下拉框选择，自动更新 ROS 参数 |
| 协同命令 | 输入文本 → 点击 `发布` → 发送至 `/master_command` |
| 获取坐标 | 点击 `获取坐标点` — 读取当前 AMCL 位姿 (X/Y/朝向) |
| 保存航点 | 输入名称 → 点击 `保存坐标点` → 持久化 + 网络同步 |

### 📊 车辆状态页

- 7 列表格实时展示所有在线车辆信息
- 新车上线自动新增行，已有车辆自动更新
- 列：名称 | 状态 | CPU频率 | 温度 | 内存 | 是否主车 | 搭载功能

### ⚙️ 模块管理页

**添加 / 更新模块：**
1. 输入模块 ID（十六进制，如 `03`）
2. 填写模块名称和功能介绍
3. 在 9 行中填写 `功能名称 → 指令编码`（编码为十六进制）
4. 点击 `提交` — 写入 YAML 并发布到 ROS 网络

**删除模块：**
1. 输入要删除的模块 ID
2. 点击 `删除` → 确认 → 从 YAML 移除 → 网络同步

**重置：** 点击 `重置` 清空所有输入字段

---

## 🎨 样式主题

采用 **赛博朋克 / 科技感暗色主题**，通过 Qt 资源系统加载：

```
  主背景:  #0A0A14  深黑蓝       ┌──────────────────────────┐
  卡片:    #12122A  暗蓝灰       │  ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓  │
  边框:    #1AFFE0  霓虹青       │  └─ 发光边框 0.5px     │
  主文字:  #E0F0FF  冰白蓝       └──────────────────────────┘
  高亮:    #00E5FF  电光青
  按钮:    深色渐变 + 青色悬停发光
  输入框:  深色背景 + 聚焦时青色边框 + 微发光
```

---

## 📋 依赖项

### ROS 包

| 包 | 用途 |
|----|------|
| `roscpp` | ROS C++ 客户端 |
| `std_msgs` | 标准消息类型 |
| `rviz` | 3D 可视化嵌入 |
| `amcl` | 自适应蒙特卡洛定位 |
| `gmapping` | SLAM 地图构建 |
| `map_server` | 地图服务 |
| `move_base` | 导航栈 |
| `message_generation` | 自定义消息生成 |
| `message_runtime` | 消息运行时 |

### 系统库

| 库 | 用途 |
|----|------|
| Qt5 (Widgets/Core/Gui) | GUI 框架 |
| yaml-cpp | YAML 解析与生成 |
| tf | 坐标变换 (四元数→欧拉角) |

---

<p align="center">
  <b>Made with ❤️ by zzysnd123</b><br>
  <sub>Licensed under Apache 2.0</sub>
</p>
