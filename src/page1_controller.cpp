#include <robot_control/page1_controller.h>
#include <robot_control/rviz_manager.h>
#include <robot_control/yaml_module_manager.h>
#include "ui_mainwindow.h"
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>
#include <QDebug>
#include <xmlrpcpp/XmlRpcValue.h>
#include <tf/tf.h>
#include <yaml-cpp/yaml.h>

Page1Controller::Page1Controller(Ui::MainWindow *ui, ros::NodeHandle &nh,
                                 RvizManager *rviz_mgr, YamlModuleManager *yaml_mgr,
                                 QObject *parent)
    : QObject(parent), ui_(ui), nh_(nh), rviz_mgr_(rviz_mgr), yaml_mgr_(yaml_mgr)
{
}

void Page1Controller::init()
{
    // 速度 spinbox 设置
    ui_->speed_show_2->setRange(0.0, speed_max_);
    ui_->speed_show_2->setDecimals(1);
    ui_->speed_show_2->setValue(speed_);
    ui_->speed_show_2->setSingleStep(speed_step_);
    ui_->angle_speed_show_2->setRange(0, angle_speed_max_);
    ui_->angle_speed_show_2->setDecimals(1);
    ui_->angle_speed_show_2->setValue(angle_speed_);
    ui_->angle_speed_show_2->setSingleStep(speed_step_);

    // 运动按钮：按下 = 运动，松开 = 停止
    connect(ui_->forword_2, &QPushButton::pressed, this, &Page1Controller::onForwardPressed);
    connect(ui_->back_2, &QPushButton::pressed, this, &Page1Controller::onBackPressed);
    connect(ui_->left_2, &QPushButton::pressed, this, &Page1Controller::onLeftPressed);
    connect(ui_->right_2, &QPushButton::pressed, this, &Page1Controller::onRightPressed);
    connect(ui_->forword_2, &QPushButton::released, this, &Page1Controller::onStopPressed);
    connect(ui_->back_2, &QPushButton::released, this, &Page1Controller::onStopPressed);
    connect(ui_->left_2, &QPushButton::released, this, &Page1Controller::onStopPressed);
    connect(ui_->right_2, &QPushButton::released, this, &Page1Controller::onStopPressed);

    // 速度值变更
    connect(ui_->speed_show_2,
            static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, [this](double v) { speed_ = v; });
    connect(ui_->angle_speed_show_2,
            static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, [this](double v) { angle_speed_ = v; });

    // 车辆切换（comboBox_2）
    connect(ui_->comboBox_2, &QComboBox::currentTextChanged,
            this, &Page1Controller::onVehicleChanged);
    connect(ui_->comboBox_2, &QComboBox::currentTextChanged,
            this, [this](const QString &name) { subscribeVehiclePose(name); });

    // 车队领航开关
    connect(ui_->open_close_lihang, &QPushButton::clicked,
            this, &Page1Controller::onFleetToggled);

    // 主车选择
    std::string master_robot;
    if (!nh_.getParam("/master_robot", master_robot))
    {
        master_robot = "robot1";
        nh_.setParam("/master_robot", master_robot);
    }
    int idx = ui_->comboBox_3->findText(QString::fromStdString(master_robot));
    if (idx >= 0)
        ui_->comboBox_3->setCurrentIndex(idx);
    connect(ui_->comboBox_3, &QComboBox::currentTextChanged,
            this, &Page1Controller::onMasterRobotChanged);

    // 主命令发布
    connect(ui_->pushButton, &QPushButton::clicked,
            this, &Page1Controller::onMasterCommandPublish);

    // 坐标点获取与保存
    connect(ui_->pushButton_3, &QPushButton::clicked,
            this, &Page1Controller::onGetPoseClicked);
    connect(ui_->pushButton_4, &QPushButton::clicked,
            this, &Page1Controller::onSaveGoalClicked);

    // 初始化 ROS 发布者
    linghang_pub_ = nh_.advertise<std_msgs::UInt8>("/open_close_linghang", 10);
    master_command_pub_ = nh_.advertise<std_msgs::String>("/master_command", 10);
    goal_param_update_pub_ = nh_.advertise<std_msgs::String>("/goal_param_update", 10);
    positions_reload_pub_ = nh_.advertise<std_msgs::Empty>("/positions_reload", 10);

    // 默认初始化为 robot1 的 cmd_vel
    onVehicleChanged(ui_->comboBox_2->currentText());

    // 默认订阅 robot1 位姿
    current_vehicle_name_ = ui_->comboBox_2->currentText();
    subscribeVehiclePose(current_vehicle_name_);
}

void Page1Controller::onForwardPressed()
{
    twist_.linear.x = speed_;
    twist_.angular.z = 0.0;
    cmd_vel_pub_.publish(twist_);
}

void Page1Controller::onBackPressed()
{
    twist_.linear.x = -speed_;
    twist_.angular.z = 0.0;
    cmd_vel_pub_.publish(twist_);
}

void Page1Controller::onLeftPressed()
{
    twist_.linear.x = 0.0;
    twist_.angular.z = angle_speed_;
    cmd_vel_pub_.publish(twist_);
}

void Page1Controller::onRightPressed()
{
    twist_.linear.x = 0.0;
    twist_.angular.z = -angle_speed_;
    cmd_vel_pub_.publish(twist_);
}

void Page1Controller::onStopPressed()
{
    twist_.linear.x = 0.0;
    twist_.angular.z = 0.0;
    cmd_vel_pub_.publish(twist_);
}

void Page1Controller::onVehicleChanged(const QString &car_name)
{
    // Page1 自己的 cmd_vel publisher，根据 comboBox_2 的车辆切换话题
    QString car_vel = "/" + car_name + "/cmd_vel";
    cmd_vel_pub_ = nh_.advertise<geometry_msgs::Twist>(car_vel.toStdString(), 10);
}

void Page1Controller::onMasterRobotChanged(const QString &car_name)
{
    nh_.setParam("/master_robot", car_name.toStdString());
    qInfo() << "主车已切换为:" << car_name;
}

void Page1Controller::onMasterCommandPublish()
{
    QString text = ui_->lineEdit->text().trimmed();
    if (text.isEmpty())
    {
        QMessageBox::information(nullptr, "提示", "请输入协同命令内容");
        return;
    }

    std_msgs::String msg;
    msg.data = text.toStdString();
    master_command_pub_.publish(msg);
    qInfo() << "已发布主命令:" << text;
    ui_->lineEdit->clear();
}

void Page1Controller::onFleetToggled()
{
    if (open_close_.data == 0)
    {
        open_close_.data = 1;
        linghang_pub_.publish(open_close_);
        ui_->lihang_statue->setText(QString("开启"));
    }
    else
    {
        open_close_.data = 0;
        linghang_pub_.publish(open_close_);
        ui_->lihang_statue->setText(QString("关闭"));
    }
}

void Page1Controller::onGetPoseClicked()
{
    if (!pose_received_)
    {
        QMessageBox::warning(nullptr, "提示", "尚未收到车辆位姿，请稍后再试");
        return;
    }
    double x = current_pose_.position.x;
    double y = current_pose_.position.y;
    tf::Quaternion q(
        current_pose_.orientation.x,
        current_pose_.orientation.y,
        current_pose_.orientation.z,
        current_pose_.orientation.w);
    double roll, pitch, yaw;
    tf::Matrix3x3(q).getRPY(roll, pitch, yaw);

    ui_->lineEdit_3->setText(QString::number(x, 'f', 3));
    ui_->lineEdit_4->setText(QString::number(y, 'f', 3));
    ui_->lineEdit_5->setText(QString::number(yaw, 'f', 3));
    qInfo() << "Got pose:" << x << y << yaw;
}

void Page1Controller::onSaveGoalClicked()
{
    QString name = ui_->lineEdit_2->text().trimmed();
    if (name.isEmpty())
    {
        QMessageBox::warning(nullptr, "警告", "坐标点名称不能为空");
        return;
    }

    bool okX, okY, okYaw;
    double x = ui_->lineEdit_3->text().toDouble(&okX);
    double y = ui_->lineEdit_4->text().toDouble(&okY);
    double yaw = ui_->lineEdit_5->text().toDouble(&okYaw);
    if (!okX || !okY || !okYaw)
    {
        QMessageBox::warning(nullptr, "警告", "X、Y、朝向必须为有效数字");
        return;
    }

    XmlRpc::XmlRpcValue positions;
    if (!nh_.getParam("/positions", positions) ||
        positions.getType() != XmlRpc::XmlRpcValue::TypeStruct)
    {
        positions = XmlRpc::XmlRpcValue();
    }

    XmlRpc::XmlRpcValue coord;
    coord.setSize(3);
    coord[0] = x;
    coord[1] = y;
    coord[2] = yaw;

    positions[name.toStdString()] = coord;
    nh_.setParam("/positions", positions);
    qInfo() << "Updated /positions with point:" << name << x << y << yaw;

    // 生成 goal_param.yaml 内容
    YAML::Node root;
    root["positions"] = YAML::Node(YAML::NodeType::Map);
    for (const auto &pair : positions)
    {
        std::string key = pair.first;
        XmlRpc::XmlRpcValue val = pair.second;
        if (val.getType() == XmlRpc::XmlRpcValue::TypeArray && val.size() == 3)
        {
            root["positions"][key].push_back(static_cast<double>(val[0]));
            root["positions"][key].push_back(static_cast<double>(val[1]));
            root["positions"][key].push_back(static_cast<double>(val[2]));
        }
    }

    YAML::Emitter emitter;
    emitter << root;
    std::string yaml_str = emitter.c_str();

    std_msgs::String update_msg;
    update_msg.data = yaml_str;
    goal_param_update_pub_.publish(update_msg);

    std_msgs::Empty empty_msg;
    positions_reload_pub_.publish(empty_msg);
    qInfo() << "Published /positions_reload to notify master node";

    QMessageBox::information(nullptr, "提示",
                             QString("坐标点 '%1' 已保存并同步").arg(name));
}

void Page1Controller::subscribeVehiclePose(const QString &car_name)
{
    current_vehicle_name_ = car_name;
    pose_received_ = false;
    pose_sub_.shutdown();
    std::string topic = "/" + car_name.toStdString() + "/amcl_pose";
    pose_sub_ = nh_.subscribe<geometry_msgs::PoseWithCovarianceStamped>(
        topic, 10,
        [this](const geometry_msgs::PoseWithCovarianceStamped::ConstPtr &msg) {
            current_pose_ = msg->pose.pose;
            pose_received_ = true;
        });
    qInfo() << "Subscribed to vehicle pose: " << QString::fromStdString(topic);
}
