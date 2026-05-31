#ifndef PAGE1_CONTROLLER_H
#define PAGE1_CONTROLLER_H

#include <QObject>
#include <QString>
#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <std_msgs/UInt8.h>
#include <std_msgs/String.h>
#include <std_msgs/Empty.h>

namespace Ui {
class MainWindow;
}

class RvizManager;
class YamlModuleManager;

class Page1Controller : public QObject
{
    Q_OBJECT
public:
    explicit Page1Controller(Ui::MainWindow *ui, ros::NodeHandle &nh,
                             RvizManager *rviz_mgr, YamlModuleManager *yaml_mgr,
                             QObject *parent = nullptr);

    void init();

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
    void onFleetToggled();
    void onVehicleChanged(const QString &car_name);
    void onMasterRobotChanged(const QString &car_name);
    void onMasterCommandPublish();
    void onGetPoseClicked();
    void onSaveGoalClicked();

private:
    void subscribeVehiclePose(const QString &car_name);

    Ui::MainWindow *ui_;
    ros::NodeHandle &nh_;
    RvizManager *rviz_mgr_;
    YamlModuleManager *yaml_mgr_;

    // ROS 通信
    ros::Publisher cmd_vel_pub_;
    ros::Publisher linghang_pub_;
    ros::Publisher master_command_pub_;
    ros::Publisher goal_param_update_pub_;
    ros::Publisher positions_reload_pub_;
    ros::Subscriber pose_sub_;

    // 状态
    geometry_msgs::Twist twist_;
    std_msgs::UInt8 open_close_;
    float speed_{0.2};
    float speed_step_{0.1};
    float angle_speed_{1.0};
    float angle_speed_max_{2.0};
    float speed_max_{1.4};
    geometry_msgs::Pose current_pose_;
    bool pose_received_{false};
    QString current_vehicle_name_;
};

#endif // PAGE1_CONTROLLER_H
