#ifndef PAGE3_CONTROLLER_H
#define PAGE3_CONTROLLER_H

#include <QObject>
#include <ros/ros.h>
#include <std_msgs/String.h>

namespace Ui {
class MainWindow;
}

class YamlModuleManager;

class Page3Controller : public QObject
{
    Q_OBJECT
public:
    explicit Page3Controller(Ui::MainWindow *ui, ros::NodeHandle &nh,
                             YamlModuleManager *yaml_mgr,
                             QObject *parent = nullptr);

    void init();

private slots:
    void onSubmitClicked();
    void onDeleteClicked();
    void onClearClicked();

private:
    void refreshTable();

    Ui::MainWindow *ui_;
    ros::NodeHandle &nh_;
    YamlModuleManager *yaml_mgr_;
    ros::Publisher modules_update_pub_;
};

#endif // PAGE3_CONTROLLER_H
