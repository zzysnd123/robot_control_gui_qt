#include <robot_control/rviz_manager.h>
#include <rviz/visualization_frame.h>
#include <rviz/visualization_manager.h>
#include <rviz/yaml_config_reader.h>
#include <rviz/config.h>
#include <QVBoxLayout>
#include <QDebug>
#include <QMessageBox>

RvizManager::RvizManager(QWidget *parent_widget_1, QWidget *parent_widget_2,
                         QObject *parent)
    : QObject(parent)
{
    // 创建两个 RViz VisualizationFrame
    rviz_frame_1_ = new rviz::VisualizationFrame;
    rviz_frame_2_ = new rviz::VisualizationFrame;

    // 初始化（必须调用，会自动创建内部布局和状态栏）
    rviz_frame_1_->initialize();
    rviz_frame_2_->initialize();

    // 嵌入到 parent_widget_1 的布局
    QVBoxLayout *layout1 = new QVBoxLayout(parent_widget_1);
    layout1->setContentsMargins(0, 0, 0, 0);
    layout1->addWidget(rviz_frame_1_);

    // 嵌入到 parent_widget_2 的布局
    QVBoxLayout *layout2 = new QVBoxLayout(parent_widget_2);
    layout2->setContentsMargins(0, 0, 0, 0);
    layout2->addWidget(rviz_frame_2_);

    // 获取管理器
    manager_1_ = rviz_frame_1_->getManager();
    manager_2_ = rviz_frame_2_->getManager();
}

RvizManager::~RvizManager() = default;

rviz::VisualizationManager* RvizManager::manager(RvizSlot slot) const
{
    return (slot == RvizSlot::SLOT_1) ? manager_1_ : manager_2_;
}

void RvizManager::loadConfig(RvizSlot slot, const QString &config_path)
{
    rviz::VisualizationManager *mgr = manager(slot);

    // 先清空默认显示项
    mgr->removeAllDisplays();

    if (config_path.isEmpty())
        return;

    if (mgr == nullptr)
    {
        QMessageBox::warning(nullptr, "警告", "RViz管理器未初始化！", QMessageBox::Ok);
        return;
    }

    // 使用 YamlConfigReader 读取配置文件
    rviz::YamlConfigReader yamlconfigreader;
    rviz::Config con;
    yamlconfigreader.readFile(con, config_path);

    if (yamlconfigreader.error())
    {
        qDebug() << "Failed to load RViz config file: " << config_path;
        mgr->initialize();
        mgr->startUpdate();
    }
    else
    {
        qDebug() << "Success to load RViz config file: " << config_path;
        rviz::Config visualization_manager_config =
            con.mapGetChild("Visualization Manager");
        mgr->load(visualization_manager_config);
        mgr->startUpdate();
    }
}

void RvizManager::clearRviz(RvizSlot slot)
{
    rviz::VisualizationManager *mgr = manager(slot);
    mgr->removeAllDisplays();
    mgr->initialize();
    mgr->startUpdate();
}
