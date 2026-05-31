#ifndef PAGE0_CONTROLLER_H
#define PAGE0_CONTROLLER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <std_msgs/UInt8.h>
#include <std_msgs/UInt16.h>

class QDoubleSpinBox;
class QComboBox;
class QPushButton;

namespace Ui {
class MainWindow;
}

class RvizManager;
class YamlModuleManager;

class Page0Controller : public QObject
{
    Q_OBJECT
public:
    explicit Page0Controller(Ui::MainWindow *ui, ros::NodeHandle &nh,
                             RvizManager *rviz_mgr, YamlModuleManager *yaml_mgr,
                             QObject *parent = nullptr);

    void init();

    // 供 MainWindow 在页面切换时同步速度/角速度
    float currentSpeed() const { return speed_; }
    float currentAngleSpeed() const { return angle_speed_; }
    void setSpeed(float s) { speed_ = s; }
    void setAngleSpeed(float a) { angle_speed_ = a; }

private slots:
    void onForwardPressed();
    void onBackPressed();
    void onLeftPressed();
    void onRightPressed();
    void onStopPressed();
    void onVehicleChanged(const QString &car_name);
    void functionButton(int index);
    void onFuncName(int moduleId);
    void onFuncData(uint16_t value);
    void updateFunctionMappingUi(const QMap<QString, QString> &mapping);

signals:
    // 内部信号，用于 ROS 回调线程 -> Qt 主线程的 QueuedConnection 桥接
    void sigFuncData(uint16_t value);
    void sigFuncName(int moduleId);

private:
    // ROS 回调（运行在 AsyncSpinner 线程中）
    void funcNameCallback(const std_msgs::UInt8::ConstPtr &msg);
    void funcDataCallback(const std_msgs::UInt16::ConstPtr &msg);

    Ui::MainWindow *ui_;
    ros::NodeHandle &nh_;
    RvizManager *rviz_mgr_;
    YamlModuleManager *yaml_mgr_;

    // ROS 通信
    ros::Publisher cmd_vel_pub_;
    ros::Subscriber func_name_sub_;
    ros::Subscriber func_data_sub_;
    ros::Publisher func_send_pub_;

    // 状态
    geometry_msgs::Twist twist_;
    float speed_{0.2};
    float speed_step_{0.1};
    float angle_speed_{1.0};
    float angle_speed_max_{2.0};
    float speed_max_{1.4};
    QMap<QString, QString> currentFuncMapping_;  // "功能1" -> 功能名
};

#endif // PAGE0_CONTROLLER_H
