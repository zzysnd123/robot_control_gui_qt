# robot_control — 多模块无人车协同平台

基于 **ROS Noetic + Qt5 + RViz** 的多机器人协同控制 GUI 应用程序。支持单车运动控制、多车协同导航、车辆状态监控以及功能模块动态配置。

---

## 目录

- [功能概览](#功能概览)
- [工程结构](#工程结构)
- [架构设计](#架构设计)
- [ROS 通信接口](#ros-通信接口)
- [自定义消息](#自定义消息)
- [功能模块配置 (YAML)](#功能模块配置-yaml)
- [构建与运行](#构建与运行)
- [使用指南](#使用指南)
- [主题样式](#主题样式)
- [依赖项](#依赖项)

---

## 功能概览

| 页面 | 功能 |
|------|------|
| **单车操作页** | 车辆选择 (robot1/2/3)、直线/角速度控制、9 个功能按钮（动态映射）、RViz 可视化、实时功能数据反馈 |
| **多车导航页** | 车队领航开关、主车选择、协同命令发布、航点获取与保存、第二位姿 RViz 显示、运动控制 |
| **车辆状态页** | 全部车辆实时状态表格：名称、状态、CPU 频率、温度、内存、是否主车、搭载功能 |
| **模块管理页** | 功能模块的增删改查，支持 9 个子功能定义，数据持久化到 YAML 并同步至无人车 |

---

## 工程结构

```
robot_control/
├── CMakeLists.txt                          # Catkin 构建配置
├── package.xml                             # ROS 包清单
├── res.qrc                                 # Qt 资源文件
├── res/
│   └── style.qss                           # Qt 暗色主题样式表
├── function_config/
│   └── modules.yaml                        # 功能模块定义（运行时加载/保存）
├── msg/
│   └── robot_interface.msg                 # 自定义 ROS 消息：车辆状态
├── include/
│   └── robot_control/
│       ├── mainwindow.h                    # 主窗口（精简协调器）
│       ├── rviz_manager.h                  # RViz 双实例管理
│       ├── yaml_module_manager.h           # YAML 数据管理（纯数据层，无 ROS 依赖）
│       ├── page0_controller.h              # 单车控制页
│       ├── page1_controller.h              # 多车导航页
│       ├── page2_controller.h              # 车辆状态页
│       └── page3_controller.h              # 模块管理页
└── src/
    ├── car_control.cpp                     # 程序入口（ROS 初始化 + Qt 事件循环）
    ├── mainwindow.cpp                      # 协调器实现（创建管理者 + 控制器，页面导航）
    ├── rviz_manager.cpp                    # RViz Frame 创建 / .rviz 配置加载 / 清空
    ├── yaml_module_manager.cpp             # YAML 文件读写 / 模块 CRUD / 表格填充
    ├── page0_controller.cpp                # 单车运动控制 / 功能按钮 / 车辆动态切换
    ├── page1_controller.cpp                # 多车导航 / 车队开关 / 航点 / 位姿订阅
    ├── page2_controller.cpp                # 车辆状态表格实时更新（/car_interface 订阅）
    ├── page3_controller.cpp                # 模块增删改 / YAML 持久化 / ROS 话题同步
    └── mainwindow.ui                       # Qt Designer UI 布局文件（4 页 QStackedWidget）
```

---

## 架构设计

### 类关系图

```
car_control.cpp (main)
  └── MainWindow (协调器)
        ├── RvizManager          — RViz 双实例生命周期管理（无 ROS 依赖）
        ├── YamlModuleManager    — 模块数据 CRUD + YAML I/O（无 ROS 依赖）
        ├── Page0Controller      — 单车控制页（cmd_vel / function 话题）
        ├── Page1Controller      — 多车导航页（fleet / waypoint / pose 话题）
        ├── Page2Controller      — 车辆状态页（/car_interface 订阅）
        └── Page3Controller      — 模块管理页（/modules_yaml_update 发布）
```

### 设计原则

| 原则 | 说明 |
|------|------|
| **单一职责** | 每个 Controller 只负责一个页面的业务逻辑 |
| **依赖注入** | Controller 通过构造函数接收 `Ui::MainWindow*`、`ros::NodeHandle&`、`RvizManager*`、`YamlModuleManager*` |
| **数据层分离** | `YamlModuleManager` 纯数据操作，不含 ROS 依赖，可独立测试 |
| **线程安全** | ROS 回调 (AsyncSpinner) → 内部信号 (QueuedConnection) → Qt 主线程更新 UI |

### 页面切换流程

1. 用户点击左侧导航按钮 (0-3)
2. `QButtonGroup::buttonClicked(int)` → `QStackedWidget::setCurrentIndex`
3. `MainWindow::onPageChanged(int)` 触发：
   - 切换 RViz 配置（清空另一页 RViz，加载当前页 .rviz）
   - 同步速度/角速度 spinbox 显示值

---

## ROS 通信接口

### 全局话题（由 MainWindow / Page1 / Page3 管理）

| 话题 | 类型 | 方向 | 用途 |
|------|------|------|------|
| `/open_close_linghang` | `std_msgs/UInt8` | 发布 | 车队领航开关：1=开启, 0=关闭 |
| `/master_command` | `std_msgs/String` | 发布 | 主车协同命令文本 |
| `/master_robot` | ROS Param (`string`) | 读写 | 当前主车名称，默认 `"robot1"` |
| `/positions` | ROS Param (`struct`) | 读写 | 航点坐标集合 |
| `/goal_param_update` | `std_msgs/String` | 发布 | 航点 YAML 更新字符串 |
| `/positions_reload` | `std_msgs/Empty` | 发布 | 通知主节点重新加载位置列表 |
| `/modules_yaml_update` | `std_msgs/String` | 发布 | 模块配置 YAML 字符串，同步给所有车辆 |

### 车辆动态话题（按选中车辆切换，例如 `robot1`）

| 话题 | 类型 | 方向 | 用途 |
|------|------|------|------|
| `/<car>/cmd_vel` | `geometry_msgs/Twist` | 发布 | 车辆运动控制 |
| `/<car>_function_name` | `std_msgs/UInt8` | 订阅 | 当前功能模块名称（编码） |
| `/<car>_function_data` | `std_msgs/UInt16` | 订阅 | 当前功能返回的测量数据 |
| `/<car>_function_send` | `std_msgs/UInt8` | 发布 | 发送功能指令编码给车辆 |
| `/<car>/amcl_pose` | `geometry_msgs/PoseWithCovarianceStamped` | 订阅 | AMCL 定位位姿（用于航点获取） |
| `/car_interface` | `robot_control/robot_interface` | 订阅 | 所有车辆状态信息 |

---

## 自定义消息

### `robot_interface.msg`

车辆状态消息，由各无人车节点发布到 `/car_interface` 话题：

```
string   robot_name       # 机器人名称（如 "robot1"）
string   robot_statue      # 机器人状态描述
float32  dianya            # 电压 / CPU 频率（V）
float32  wendu             # 温度（°C）
float32  shidu             # 内存使用率（%）
uint8    robot_master      # 是否为主车（1=是, 0=否）
string   robot_function    # 当前搭载的功能模块名称
```

---

## 功能模块配置 (YAML)

配置文件路径：`function_config/modules.yaml`

### 格式说明

```yaml
modules:
  - id: 0                      # 模块 ID（十六进制整数）
    name: 无模块                # 模块中文名称
    description: 无             # 模块功能描述
    functions:                  # 子功能映射：功能名 -> 指令编码（十六进制）
      无功能: 0
```

### 预置模块

| ID | 名称 | 功能 |
|----|------|------|
| `0x00` | 无模块 | 空模块占位 |
| `0x01` | 运输模块 | 卸货、打开舱门、显示重量、重置 |
| `0x02` | 叉车模块 | 叉子上升/下降、停止移动、回到最低位置 |

### 运行时管理

- 在 **模块管理页** (Page 3) 可动态添加/修改/删除模块
- 提交后自动写入 `modules.yaml` 并发布到 `/modules_yaml_update` 话题
- 车辆节点收到更新后同步本地配置

---

## 构建与运行

### 前置条件

- **操作系统**: Ubuntu 20.04
- **ROS**: Noetic (完整桌面版)
- **Qt**: 5.x (Widgets, Core, Gui)
- **yaml-cpp**: 系统级或通过包管理器安装

### 安装依赖

```bash
# 安装 yaml-cpp
sudo apt install libyaml-cpp-dev

# 确保 ROS Noetic 已安装并 source
source /opt/ros/noetic/setup.bash
```

### 构建

```bash
cd ~/QTROSworkspaces/catkin_ws

# 使用 catkin build (推荐)
catkin build robot_control

# 或使用 catkin_make
# catkin_make
```

### 运行

```bash
# 终端 1：启动 ROS Master
roscore

# 终端 2：启动 robot_control GUI
source ~/QTROSworkspaces/catkin_ws/devel/setup.bash
rosrun robot_control car_control
```

> **注意**: RViz 配置文件（`.rviz`）需要放置在 `~/桌面/` 目录下，命名格式为：
> - `rviz_robot1.rviz` — 单车控制页默认 RViz 配置
> - `rviz_robot2.rviz` / `rviz_robot3.rviz` — 按车辆名称切换
> - `rviz2.rviz` — 多车导航页默认 RViz 配置

---

## 使用指南

### 1. 单车操作页

| 操作 | 方法 |
|------|------|
| 选择车辆 | 右上角 `当前车辆` 下拉框选择 robot1/2/3 |
| 运动控制 | 点击 前进/后退/左转/右转 按钮（按住=运动，松开=停止） |
| 速度调节 | `直线速度` / `角速度` spinbox 调整，步长 0.1 |
| 功能按钮 | 9 个按钮自动映射当前模块的子功能，点击发送对应指令编码 |
| 查看反馈 | `功能名称` / `功能介绍` / `测量值` 区域实时更新 |
| RViz 显示 | 左侧嵌入 RViz 窗口，加载对应车辆的 `.rviz` 配置 |

### 2. 多车导航页

| 操作 | 方法 |
|------|------|
| 车队控制 | 点击 `开启车队` 按钮切换车队领航模式 |
| 主车选择 | `当前主车` 下拉框选择，自动更新 `/master_robot` ROS 参数 |
| 协同命令 | 在输入框输入命令文本，点击 `发布` 发送到 `/master_command` |
| 获取坐标 | 点击 `获取坐标点` 读取当前车辆的 AMCL 位姿 (X/Y/朝向) |
| 保存航点 | 输入航点名称，点击 `保存坐标点` 持久化并同步 |
| 运动控制 | 同上（独立的速度和方向控制） |

### 3. 车辆状态页

- 实时表格显示所有在线车辆的信息
- 新车辆出现时自动新增行
- 已有车辆自动更新对应行
- 7 列：名称 / 状态 / CPU频率 / 温度 / 内存使用率 / 是否主车 / 搭载功能

### 4. 模块管理页

**添加/更新模块：**
1. 输入模块 ID（十六进制，如 `03`）
2. 填写模块名称和功能介绍
3. 在 9 行中填写至少一个 `功能名称 → 指令编码` 映射
4. 点击 `提交` — 写入 YAML 并同步到 ROS 网络

**删除模块：**
1. 输入模块 ID
2. 点击 `删除` → 确认 → 从 YAML 和内存中移除 → 同步

**重置：** 点击 `重置` 清空所有输入字段

---

## 主题样式

应用使用暗色主题 (`res/style.qss`)，通过 Qt 资源系统加载：

| 元素 | 样式特征 |
|------|----------|
| 主背景 | `#1E1E2E` 深紫灰 |
| 文字 | `#E0E0FF` 浅紫白 |
| 按钮 | 渐变紫灰底色，蓝色高亮悬停 (`#6A87E8`)，圆角 8px |
| 输入框 | 深色渐变底色，聚焦时蓝色边框 |
| 下拉框 | 匹配输入框风格，蓝色选中高亮 |

---

## 依赖项

### ROS 包

| 包 | 用途 |
|----|------|
| `roscpp` | ROS C++ 客户端库 |
| `std_msgs` | 标准消息类型 |
| `rviz` | 3D 可视化（嵌入式） |
| `amcl` | 自适应蒙特卡洛定位 |
| `gmapping` | SLAM 地图构建 |
| `map_server` | 地图服务 |
| `move_base` | 导航栈 |
| `message_generation` | 自定义消息生成 |
| `message_runtime` | 消息运行时支持 |

### 系统库

| 库 | 用途 |
|----|------|
| Qt5 (Widgets/Core/Gui) | GUI 框架 |
| yaml-cpp | YAML 配置文件解析与生成 |
| tf | 坐标变换（四元数→欧拉角） |
