#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <ros/ros.h>

namespace Ui {
class MainWindow;
}

class RvizManager;
class YamlModuleManager;
class Page0Controller;
class Page1Controller;
class Page2Controller;
class Page3Controller;
class QButtonGroup;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onPageChanged(int index);

private:
    void createManagers();
    void createControllers();
    void setupNavigation();

    Ui::MainWindow *ui;
    ros::NodeHandle nh;

    // 基础设施管理者
    RvizManager *rviz_mgr_{nullptr};
    YamlModuleManager *yaml_mgr_{nullptr};

    // 页面控制器
    Page0Controller *page0_ctrl_{nullptr};
    Page1Controller *page1_ctrl_{nullptr};
    Page2Controller *page2_ctrl_{nullptr};
    Page3Controller *page3_ctrl_{nullptr};

    QButtonGroup *nav_btn_group_{nullptr};
};

#endif // MAINWINDOW_H
