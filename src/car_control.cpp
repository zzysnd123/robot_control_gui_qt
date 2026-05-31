#include "ros/ros.h"
#include <robot_control/mainwindow.h>
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    // 先让 ROS 处理并移除它自己的参数，避免 Qt 解析到 ROS remap 参数
    ros::init(argc, argv, "car_control");

    // 必须先有 QApplication，后面才能 new QWidget / MainWindow / RenderPanel
    QApplication app(argc, argv);

    // 加载 Qt 样式表
    QFile styleFile(":/style/res/style.qss");
    if (styleFile.open(QFile::ReadOnly))
    {
        app.setStyleSheet(styleFile.readAll());
        styleFile.close();
    }

    // ROS 回调线程（不阻塞 Qt 事件循环）
    ros::AsyncSpinner spinner(4);
    spinner.start();

    MainWindow w;
    w.show();

    int ret = app.exec();

    ros::shutdown();
    return ret;
}
