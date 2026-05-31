#include <robot_control/page0_controller.h>
#include <robot_control/rviz_manager.h>
#include <robot_control/yaml_module_manager.h>
#include "ui_mainwindow.h"
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QDebug>
#include <QMetaType>

Page0Controller::Page0Controller(Ui::MainWindow *ui, ros::NodeHandle &nh,
                                 RvizManager *rviz_mgr, YamlModuleManager *yaml_mgr,
                                 QObject *parent)
    : QObject(parent), ui_(ui), nh_(nh), rviz_mgr_(rviz_mgr), yaml_mgr_(yaml_mgr)
{
}

void Page0Controller::init()
{
    // 速度 spinbox 设置
    ui_->speed_show->setRange(0.0, speed_max_);
    ui_->speed_show->setDecimals(1);
    ui_->speed_show->setValue(speed_);
    ui_->speed_show->setSingleStep(speed_step_);
    ui_->angle_speed_show->setRange(0, angle_speed_max_);
    ui_->angle_speed_show->setDecimals(1);
    ui_->angle_speed_show->setValue(angle_speed_);
    ui_->angle_speed_show->setSingleStep(speed_step_);

    // 运动按钮：按下 = 运动，松开 = 停止
    connect(ui_->forword, &QPushButton::pressed, this, &Page0Controller::onForwardPressed);
    connect(ui_->back, &QPushButton::pressed, this, &Page0Controller::onBackPressed);
    connect(ui_->left, &QPushButton::pressed, this, &Page0Controller::onLeftPressed);
    connect(ui_->right, &QPushButton::pressed, this, &Page0Controller::onRightPressed);
    connect(ui_->forword, &QPushButton::released, this, &Page0Controller::onStopPressed);
    connect(ui_->back, &QPushButton::released, this, &Page0Controller::onStopPressed);
    connect(ui_->left, &QPushButton::released, this, &Page0Controller::onStopPressed);
    connect(ui_->right, &QPushButton::released, this, &Page0Controller::onStopPressed);

    // 速度值变更
    connect(ui_->speed_show,
            static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, [this](double v) { speed_ = v; });
    connect(ui_->angle_speed_show,
            static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, [this](double v) { angle_speed_ = v; });

    // ROS线程 -> Qt主线程 跨线程桥接
    connect(this, &Page0Controller::sigFuncData, this, &Page0Controller::onFuncData,
            Qt::QueuedConnection);
    connect(this, &Page0Controller::sigFuncName, this, &Page0Controller::onFuncName,
            Qt::QueuedConnection);

    // 车辆切换
    connect(ui_->comboBox, &QComboBox::currentTextChanged,
            this, &Page0Controller::onVehicleChanged);

    // 功能按钮 1-9
    connect(ui_->function_1, &QPushButton::clicked, this, [this]() { functionButton(1); });
    connect(ui_->function_2, &QPushButton::clicked, this, [this]() { functionButton(2); });
    connect(ui_->function_3, &QPushButton::clicked, this, [this]() { functionButton(3); });
    connect(ui_->function_4, &QPushButton::clicked, this, [this]() { functionButton(4); });
    connect(ui_->function_5, &QPushButton::clicked, this, [this]() { functionButton(5); });
    connect(ui_->function_6, &QPushButton::clicked, this, [this]() { functionButton(6); });
    connect(ui_->function_7, &QPushButton::clicked, this, [this]() { functionButton(7); });
    connect(ui_->function_8, &QPushButton::clicked, this, [this]() { functionButton(8); });
    connect(ui_->function_9, &QPushButton::clicked, this, [this]() { functionButton(9); });

    // 默认初始化话题为 robot1
    onVehicleChanged("robot1");
    onFuncName(0x00);
}

void Page0Controller::onForwardPressed()
{
    twist_.linear.x = speed_;
    twist_.angular.z = 0.0;
    cmd_vel_pub_.publish(twist_);
}

void Page0Controller::onBackPressed()
{
    twist_.linear.x = -speed_;
    twist_.angular.z = 0.0;
    cmd_vel_pub_.publish(twist_);
}

void Page0Controller::onLeftPressed()
{
    twist_.linear.x = 0.0;
    twist_.angular.z = angle_speed_;
    cmd_vel_pub_.publish(twist_);
}

void Page0Controller::onRightPressed()
{
    twist_.linear.x = 0.0;
    twist_.angular.z = -angle_speed_;
    cmd_vel_pub_.publish(twist_);
}

void Page0Controller::onStopPressed()
{
    twist_.linear.x = 0.0;
    twist_.angular.z = 0.0;
    cmd_vel_pub_.publish(twist_);
}

void Page0Controller::onVehicleChanged(const QString &car_name)
{
    QString car_vel = "/" + car_name + "/cmd_vel";
    QString car_function_name = "/" + car_name + "_function_name";
    QString car_function_data = "/" + car_name + "_function_data";
    QString car_function_send = "/" + car_name + "_function_send";
    QString car_rviz = "/home/zhou/桌面/rviz_" + car_name + ".rviz";

    // 重新创建 publisher / subscriber 以切换车辆
    cmd_vel_pub_ = nh_.advertise<geometry_msgs::Twist>(car_vel.toStdString(), 10);
    func_name_sub_ = nh_.subscribe<std_msgs::UInt8>(
        car_function_name.toStdString(), 10, &Page0Controller::funcNameCallback, this);
    func_data_sub_ = nh_.subscribe<std_msgs::UInt16>(
        car_function_data.toStdString(), 10, &Page0Controller::funcDataCallback, this);
    func_send_pub_ = nh_.advertise<std_msgs::UInt8>(car_function_send.toStdString(), 10);

    // 切换 RViz 配置
    rviz_mgr_->loadConfig(RvizSlot::SLOT_1, car_rviz);

    // 刷新功能按钮显示
    onFuncName(0x00);
}

void Page0Controller::functionButton(int index)
{
    QString funcKey = QString("功能%1").arg(index);
    QString funcName = currentFuncMapping_.value(funcKey, "未定义");

    if (funcName == "未定义")
    {
        qWarning() << funcKey << " 未绑定实际功能";
        return;
    }

    int code = yaml_mgr_->findFunctionCode(funcName);
    if (code < 0)
    {
        qWarning() << "未找到功能编码: " << funcName;
        return;
    }

    std_msgs::UInt8 out;
    out.data = static_cast<uint8_t>(code);
    func_send_pub_.publish(out);

    qInfo() << QString("发布 %1: %2 (0x%3)")
                   .arg(funcKey)
                   .arg(funcName)
                   .arg(code, 2, 16, QChar('0'))
                   .toUpper();
}

void Page0Controller::onFuncName(int moduleId)
{
    const auto &moduleMap = yaml_mgr_->allModules();
    auto it = moduleMap.find(moduleId);
    ModuleInfo info;
    if (it != moduleMap.end())
    {
        info = it->second;
    }
    else
    {
        info.name = "未知模块";
        info.desc = "无描述";
    }

    // 映射功能到按钮
    QMap<QString, QString> mapping;
    for (int i = 0; i < 9; ++i)
    {
        QString key = QString("功能%1").arg(i + 1);
        if (i < (int)info.funcOrder.size())
            mapping[key] = info.funcOrder[i];
        else
            mapping[key] = "未定义";
    }
    currentFuncMapping_ = mapping;

    ui_->function_name->setText(info.name);
    ui_->function_Introduction->setText(info.desc);
    updateFunctionMappingUi(mapping);
}

void Page0Controller::onFuncData(uint16_t value)
{
    ui_->function_value->setText(QString::number(value));
}

void Page0Controller::updateFunctionMappingUi(const QMap<QString, QString> &mapping)
{
    ui_->function_1->setText(mapping.value("功能1"));
    ui_->function_2->setText(mapping.value("功能2"));
    ui_->function_3->setText(mapping.value("功能3"));
    ui_->function_4->setText(mapping.value("功能4"));
    ui_->function_5->setText(mapping.value("功能5"));
    ui_->function_6->setText(mapping.value("功能6"));
    ui_->function_7->setText(mapping.value("功能7"));
    ui_->function_8->setText(mapping.value("功能8"));
    ui_->function_9->setText(mapping.value("功能9"));
}

// ---- ROS 回调（运行在 AsyncSpinner 线程） ----
void Page0Controller::funcNameCallback(const std_msgs::UInt8::ConstPtr &msg)
{
    emit sigFuncName(msg->data);
}

void Page0Controller::funcDataCallback(const std_msgs::UInt16::ConstPtr &msg)
{
    emit sigFuncData(msg->data);
}
