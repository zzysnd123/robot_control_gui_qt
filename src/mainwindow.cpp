#include <robot_control/mainwindow.h>
#include <robot_control/rviz_manager.h>
#include <robot_control/yaml_module_manager.h>
#include <robot_control/page0_controller.h>
#include <robot_control/page1_controller.h>
#include <robot_control/page2_controller.h>
#include <robot_control/page3_controller.h>
#include "ui_mainwindow.h"
#include <QButtonGroup>
#include <QStackedWidget>
#include <QMetaType>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(QString("多模块无人车协同平台"));
    resize(600, 500);

    // 注册跨线程信号槽必需的元类型
    qRegisterMetaType<robot_control::robot_interface::ConstPtr>(
        "robot_control::robot_interface::ConstPtr");

    // 顺序很重要：先创建数据/基础设施管理者，再创建页面控制器
    createManagers();
    createControllers();
    setupNavigation();

    // 默认显示 page 0 并加载 RViz 配置
    rviz_mgr_->loadConfig(RvizSlot::SLOT_1,
                          "/home/zhou/桌面/rviz_robot1.rviz");
    ui->stackedWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createManagers()
{
    // RViz 管理器：嵌入到两个页面各自的 rviz 占位 Widget
    rviz_mgr_ = new RvizManager(ui->rviz_widget, ui->rviz_2, this);

    // YAML 模块管理器：自动加载配置文件
    QString yaml_path =
        "/home/zhou/QTROSworkspaces/catkin_ws/src/robot_control/function_config/modules.yaml";
    yaml_mgr_ = new YamlModuleManager(yaml_path, this);
}

void MainWindow::createControllers()
{
    // 创建4个页面控制器，注入共享的依赖
    page0_ctrl_ = new Page0Controller(ui, nh, rviz_mgr_, yaml_mgr_, this);
    page1_ctrl_ = new Page1Controller(ui, nh, rviz_mgr_, yaml_mgr_, this);
    page2_ctrl_ = new Page2Controller(ui, nh, this);
    page3_ctrl_ = new Page3Controller(ui, nh, yaml_mgr_, this);

    // 初始化每个页面的信号槽连接和ROS订阅
    page0_ctrl_->init();
    page1_ctrl_->init();
    page2_ctrl_->init();
    page3_ctrl_->init();
}

void MainWindow::setupNavigation()
{
    nav_btn_group_ = new QButtonGroup(this);
    nav_btn_group_->addButton(ui->one_car, 0);
    nav_btn_group_->addButton(ui->maney_car, 1);
    nav_btn_group_->addButton(ui->Vehicle_Status, 2);
    nav_btn_group_->addButton(ui->add_functional, 3);

    void (QButtonGroup::*idClicked)(int) = &QButtonGroup::buttonClicked;
    connect(nav_btn_group_, idClicked, ui->stackedWidget, &QStackedWidget::setCurrentIndex);
    connect(nav_btn_group_, idClicked, this, &MainWindow::onPageChanged);
}

void MainWindow::onPageChanged(int index)
{
    if (index == 0)
    {
        // 切换到单车控制页：清空 page1 的 RViz，加载 page0 的 RViz
        rviz_mgr_->clearRviz(RvizSlot::SLOT_2);
        rviz_mgr_->loadConfig(RvizSlot::SLOT_1,
                              "/home/zhou/桌面/rviz_robot1.rviz");
        // 同步速度/角速度显示
        ui->speed_show->setValue(page0_ctrl_->currentSpeed());
        ui->angle_speed_show->setValue(page0_ctrl_->currentAngleSpeed());
    }
    else if (index == 1)
    {
        // 切换到多车导航页：清空 page0 的 RViz，加载 page1 的 RViz
        rviz_mgr_->clearRviz(RvizSlot::SLOT_1);
        rviz_mgr_->loadConfig(RvizSlot::SLOT_2,
                              "/home/zhou/桌面/rviz2.rviz");
        // 同步速度/角速度显示
        ui->speed_show_2->setValue(page1_ctrl_->currentSpeed());
        ui->angle_speed_show_2->setValue(page1_ctrl_->currentAngleSpeed());
    }
}
