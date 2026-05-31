#ifndef PAGE2_CONTROLLER_H
#define PAGE2_CONTROLLER_H

#include <QObject>
#include <ros/ros.h>
#include <robot_control/robot_interface.h>

namespace Ui {
class MainWindow;
}

class Page2Controller : public QObject
{
    Q_OBJECT
public:
    explicit Page2Controller(Ui::MainWindow *ui, ros::NodeHandle &nh,
                             QObject *parent = nullptr);

    void init();

private slots:
    void onRobotInfoReceived(const robot_control::robot_interface::ConstPtr &msg);

signals:
    void sigRobotinfo(const robot_control::robot_interface::ConstPtr &msg);

private:
    void robotInfoCallback(const robot_control::robot_interface::ConstPtr &msg);

    Ui::MainWindow *ui_;
    ros::NodeHandle &nh_;
    ros::Subscriber robot_information_sub_;
};

// 元类型声明（跨线程信号槽必需）
Q_DECLARE_METATYPE(robot_control::robot_interface::ConstPtr)

#endif // PAGE2_CONTROLLER_H
