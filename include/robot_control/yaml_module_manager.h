#ifndef YAML_MODULE_MANAGER_H
#define YAML_MODULE_MANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <map>
#include <vector>

class QTableWidget;

// 模块信息结构体（从 MainWindow 中提取，保持与 YAML 格式兼容）
struct ModuleInfo
{
    QString name;
    QString desc;
    std::map<QString, int> functions;   // 功能名 -> 编码
    std::vector<QString> funcOrder;       // 保留 YAML 中功能顺序
    ModuleInfo() = default;
};

class YamlModuleManager : public QObject
{
    Q_OBJECT
public:
    explicit YamlModuleManager(const QString &yaml_path, QObject *parent = nullptr);

    // YAML 文件 I/O
    void loadFromFile(const QString &path);
    void saveToFile(const QString &path) const;   // 仅写文件，不做 ROS 发布

    // 查询
    const std::map<int, ModuleInfo>& allModules() const { return moduleMap_; }
    bool findModule(int id, ModuleInfo &out) const;
    int findFunctionCode(const QString &funcName) const;  // 遍历所有模块查找功能编码，找不到返回 -1

    // 修改（内存数据，不自动写文件）
    void addOrUpdateModule(int id, const ModuleInfo &info);
    bool deleteModule(int id);

    // 获取路径
    QString yamlPath() const { return yaml_path_; }

    // UI 辅助 — 填充 QTableWidget（不依赖具体 UI 指针）
    void populateTable(QTableWidget *table) const;

signals:
    void modulesChanged();

private:
    std::map<int, ModuleInfo> moduleMap_;
    QString yaml_path_;
};

#endif // YAML_MODULE_MANAGER_H
