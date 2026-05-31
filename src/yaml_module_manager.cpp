#include <robot_control/yaml_module_manager.h>
#include <yaml-cpp/yaml.h>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QDebug>
#include <QMessageBox>
#include <fstream>

YamlModuleManager::YamlModuleManager(const QString &yaml_path, QObject *parent)
    : QObject(parent), yaml_path_(yaml_path)
{
    loadFromFile(yaml_path_);
}

void YamlModuleManager::loadFromFile(const QString &path)
{
    moduleMap_.clear();
    try
    {
        YAML::Node root = YAML::LoadFile(path.toStdString());
        YAML::Node modules = root["modules"];
        if (!modules || !modules.IsSequence())
        {
            qWarning() << "YAML中没有找到 modules 数组: " << path;
            return;
        }
        for (const auto &m : modules)
        {
            if (!m["id"]) continue;
            int id = m["id"].as<int>();
            ModuleInfo info;
            info.name = QString::fromStdString(m["name"].as<std::string>("未知模块"));
            info.desc = QString::fromStdString(m["description"].as<std::string>("无描述"));
            YAML::Node funcs = m["functions"];
            if (funcs.IsMap())
            {
                for (auto it = funcs.begin(); it != funcs.end(); ++it)
                {
                    QString funcName = QString::fromStdString(it->first.as<std::string>());
                    int code = it->second.as<int>();
                    info.functions[funcName] = code;
                    info.funcOrder.push_back(funcName);
                }
            }
            moduleMap_[id] = info;
        }
        qInfo() << "成功解析YAML: " << path << ", 加载模块数=" << moduleMap_.size();
    }
    catch (const std::exception &e)
    {
        qCritical() << "YAML解析失败: " << e.what();
    }
}

void YamlModuleManager::saveToFile(const QString &path) const
{
    try
    {
        YAML::Node root;
        root["modules"] = YAML::Node(YAML::NodeType::Sequence);

        for (const auto &pair : moduleMap_)
        {
            int id = pair.first;
            const ModuleInfo &info = pair.second;

            YAML::Node moduleNode;
            moduleNode["id"] = id;
            moduleNode["name"] = info.name.toStdString();
            moduleNode["description"] = info.desc.toStdString();

            YAML::Node funcs;
            for (const auto &funcPair : info.functions)
            {
                funcs[funcPair.first.toStdString()] = funcPair.second;
            }
            moduleNode["functions"] = funcs;

            root["modules"].push_back(moduleNode);
        }

        YAML::Emitter emitter;
        emitter << root;
        std::string yaml_str = emitter.c_str();

        std::ofstream fout(path.toStdString());
        if (!fout.is_open())
        {
            qCritical() << "无法打开 YAML 文件进行写入: " << path;
            return;
        }
        fout << yaml_str;
        fout.close();

        qInfo() << "成功保存配置到: " << path;
    }
    catch (const std::exception &e)
    {
        qCritical() << "YAML 写入失败: " << e.what();
    }
}

bool YamlModuleManager::findModule(int id, ModuleInfo &out) const
{
    auto it = moduleMap_.find(id);
    if (it != moduleMap_.end())
    {
        out = it->second;
        return true;
    }
    return false;
}

int YamlModuleManager::findFunctionCode(const QString &funcName) const
{
    for (const auto &kv : moduleMap_)
    {
        const ModuleInfo &m = kv.second;
        auto fit = m.functions.find(funcName);
        if (fit != m.functions.end())
        {
            return fit->second;
        }
    }
    return -1;
}

void YamlModuleManager::addOrUpdateModule(int id, const ModuleInfo &info)
{
    // 保护 ID=0 的"无模块"不被修改
    if (id == 0) return;
    moduleMap_[id] = info;
    emit modulesChanged();
}

bool YamlModuleManager::deleteModule(int id)
{
    // 保护 ID=0 的"无模块"不被删除
    if (id == 0) return false;
    auto it = moduleMap_.find(id);
    if (it == moduleMap_.end())
        return false;
    moduleMap_.erase(it);
    emit modulesChanged();
    return true;
}

void YamlModuleManager::populateTable(QTableWidget *table) const
{
    table->setRowCount(0);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    for (const auto &pair : moduleMap_)
    {
        int id = pair.first;
        // 隐藏 ID=0 的"无模块"占位项，不允许在表格中显示
        if (id == 0) continue;
        const ModuleInfo &info = pair.second;

        int row = table->rowCount();
        table->insertRow(row);

        // ID 列（十六进制显示）
        QTableWidgetItem *idItem = new QTableWidgetItem(
            QString("0x%1").arg(id, 2, 16, QChar('0')).toUpper());
        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(row, 0, idItem);

        // 功能名称列
        QTableWidgetItem *nameItem = new QTableWidgetItem(info.name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(row, 1, nameItem);

        // 功能介绍列
        QTableWidgetItem *descItem = new QTableWidgetItem(info.desc);
        descItem->setFlags(descItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(row, 2, descItem);
    }
}
