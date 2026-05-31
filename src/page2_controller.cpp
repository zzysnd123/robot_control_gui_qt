#include <robot_control/page2_controller.h>
#include "ui_mainwindow.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QDebug>
#include <QMetaType>

Page2Controller::Page2Controller(Ui::MainWindow *ui, ros::NodeHandle &nh,
                                 QObject *parent)
    : QObject(parent), ui_(ui), nh_(nh)
{
}

void Page2Controller::init()
{
    ui_->robot_info_show->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 连接跨线程信号槽（ROS 回调线程 -> Qt 主线程）
    connect(this, &Page2Controller::sigRobotinfo, this, &Page2Controller::onRobotInfoReceived,
            Qt::QueuedConnection);

    // 订阅 /car_interface 话题
    robot_information_sub_ = nh_.subscribe<robot_control::robot_interface>(
        "/car_interface", 10, &Page2Controller::robotInfoCallback, this);
}

void Page2Controller::robotInfoCallback(const robot_control::robot_interface::ConstPtr &msg)
{
    emit sigRobotinfo(msg);
}

void Page2Controller::onRobotInfoReceived(const robot_control::robot_interface::ConstPtr &robot_info)
{
    qDebug() << "无人车名称:" << QString::fromStdString(robot_info->robot_name);
    qDebug() << "无人车状态:" << QString::fromStdString(robot_info->robot_statue);
    qDebug() << "无人车cpu频率:" << robot_info->dianya;
    qDebug() << "无人车温度:" << robot_info->wendu;
    qDebug() << "无人车内存使用率:" << robot_info->shidu;
    qDebug() << "无人车是否为主车:" << robot_info->robot_master;
    qDebug() << "无人车功能模块名称:" << QString::fromStdString(robot_info->robot_function);

    QString carname = QString::fromStdString(robot_info->robot_name);
    int target = -1;
    for (int i = 0; i < ui_->robot_info_show->rowCount(); i++)
    {
        QTableWidgetItem *item = ui_->robot_info_show->item(i, 0);
        if (item && item->text() == carname)
        {
            target = i;
            qDebug() << "更新行号：" << target;
            break;
        }
    }

    if (target == -1)
    {
        target = ui_->robot_info_show->rowCount();
        ui_->robot_info_show->insertRow(target);
    }

    ui_->robot_info_show->setItem(target, 0, new QTableWidgetItem(carname));
    ui_->robot_info_show->setItem(target, 1,
        new QTableWidgetItem(QString::fromStdString(robot_info->robot_statue)));
    ui_->robot_info_show->setItem(target, 2,
        new QTableWidgetItem(QString::number(robot_info->dianya, 'f', 1)));
    ui_->robot_info_show->setItem(target, 3,
        new QTableWidgetItem(QString::number(robot_info->wendu, 'f', 1)));
    ui_->robot_info_show->setItem(target, 4,
        new QTableWidgetItem(QString::number(robot_info->shidu, 'f', 1)));

    if (robot_info->robot_master == 1)
        ui_->robot_info_show->setItem(target, 5, new QTableWidgetItem("是"));
    else
        ui_->robot_info_show->setItem(target, 5, new QTableWidgetItem("否"));

    ui_->robot_info_show->setItem(target, 6,
        new QTableWidgetItem(QString::fromStdString(robot_info->robot_function)));
}
