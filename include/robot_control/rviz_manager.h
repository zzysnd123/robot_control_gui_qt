#ifndef RVIZ_MANAGER_H
#define RVIZ_MANAGER_H

#include <QObject>
#include <QString>

namespace rviz
{
class VisualizationFrame;
class VisualizationManager;
}

// 枚举区分两个 RViz 槽位
enum class RvizSlot
{
    SLOT_1,   // 对应 page 0 (单车控制页)
    SLOT_2    // 对应 page 1 (多车导航页)
};

class RvizManager : public QObject
{
    Q_OBJECT
public:
    // 构造器：接收两个父 QWidget，自动创建 VisualizationFrame 并嵌入布局
    explicit RvizManager(QWidget *parent_widget_1, QWidget *parent_widget_2,
                         QObject *parent = nullptr);
    ~RvizManager() override;

    // 加载 .rviz 配置文件到指定槽位
    void loadConfig(RvizSlot slot, const QString &config_path);
    // 清空指定槽位的所有显示
    void clearRviz(RvizSlot slot);
    // 获取指定槽位的 VisualizationManager（用于外部需要直接操作 manager 的场景）
    rviz::VisualizationManager* manager(RvizSlot slot) const;

private:
    rviz::VisualizationFrame *rviz_frame_1_{nullptr};
    rviz::VisualizationFrame *rviz_frame_2_{nullptr};
    rviz::VisualizationManager *manager_1_{nullptr};
    rviz::VisualizationManager *manager_2_{nullptr};
};

#endif // RVIZ_MANAGER_H
