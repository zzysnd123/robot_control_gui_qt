#include <robot_control/page3_controller.h>
#include <robot_control/yaml_module_manager.h>
#include "ui_mainwindow.h"
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QDebug>
#include <vector>
#include <yaml-cpp/yaml.h>

Page3Controller::Page3Controller(Ui::MainWindow *ui, ros::NodeHandle &nh,
                                 YamlModuleManager *yaml_mgr, QObject *parent)
    : QObject(parent), ui_(ui), nh_(nh), yaml_mgr_(yaml_mgr)
{
}

void Page3Controller::init()
{
    ui_->fun_show->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui_->fun_show->setRowCount(0);

    // 连接提交、重置、删除按钮
    connect(ui_->push, &QPushButton::clicked, this, &Page3Controller::onSubmitClicked);
    connect(ui_->clear, &QPushButton::clicked, this, &Page3Controller::onClearClicked);
    connect(ui_->pushButton_2, &QPushButton::clicked, this, &Page3Controller::onDeleteClicked);

    // 初始化发布者
    modules_update_pub_ = nh_.advertise<std_msgs::String>("/modules_yaml_update", 10);

    // 首次加载表格
    refreshTable();
}

void Page3Controller::refreshTable()
{
    yaml_mgr_->populateTable(ui_->fun_show);
}

void Page3Controller::onSubmitClicked()
{
    QString idStr = ui_->fun_id->text().trimmed();
    QString name = ui_->fun_name->text().trimmed();
    QString desc = ui_->fun_int->toPlainText().trimmed();

    if (idStr.isEmpty() || name.isEmpty())
    {
        QMessageBox::warning(nullptr, "警告", "模块ID和名称不能为空！");
        return;
    }

    bool ok = false;
    int moduleId = idStr.toInt(&ok, 16);
    if (!ok || moduleId < 0)
    {
        QMessageBox::warning(nullptr, "警告", "模块ID必须为有效的十六进制正整数！");
        return;
    }
    if (moduleId == 0)
    {
        QMessageBox::warning(nullptr, "警告", "ID 0x00（无模块）为系统保留项，不可修改！");
        return;
    }

    // 收集9个功能名和指令编码
    std::vector<QLineEdit *> funcNameEdits = {
        ui_->fun_name_1, ui_->fun_name_2, ui_->fun_name_3,
        ui_->fun_name_4, ui_->fun_name_5, ui_->fun_name_6,
        ui_->fun_name_7, ui_->fun_name_8, ui_->fun_name_9};
    std::vector<QLineEdit *> funcCmdEdits = {
        ui_->fun_zhiling_1, ui_->fun_zhiling_2, ui_->fun_zhiling_3,
        ui_->fun_zhiling_4, ui_->fun_zhiling_5, ui_->fun_zhiling_6,
        ui_->fun_zhiling_7, ui_->fun_zhiling_8, ui_->fun_zhiling_9};

    ModuleInfo info;
    info.name = name;
    info.desc = desc;

    for (int i = 0; i < 9; ++i)
    {
        QString funcName = funcNameEdits[i]->text().trimmed();
        QString codeStr = funcCmdEdits[i]->text().trimmed();

        if (!funcName.isEmpty() && !codeStr.isEmpty())
        {
            bool codeOk = false;
            int code = codeStr.toInt(&codeOk, 16);
            if (!codeOk)
            {
                QMessageBox::warning(nullptr, "输入错误",
                    QString("功能%1的指令编码不是有效的十六进制数！").arg(i + 1));
                return;
            }
            info.functions[funcName] = code;
            info.funcOrder.push_back(funcName);
        }
    }

    // 存入内存
    yaml_mgr_->addOrUpdateModule(moduleId, info);

    // 持久化
    yaml_mgr_->saveToFile(yaml_mgr_->yamlPath());

    // 发布到无人车同步话题
    // 重新生成 YAML 字符串用于发布
    try
    {
        YAML::Node root;
        root["modules"] = YAML::Node(YAML::NodeType::Sequence);
        for (const auto &pair : yaml_mgr_->allModules())
        {
            int id = pair.first;
            const ModuleInfo &m = pair.second;
            YAML::Node moduleNode;
            moduleNode["id"] = id;
            moduleNode["name"] = m.name.toStdString();
            moduleNode["description"] = m.desc.toStdString();
            YAML::Node funcs;
            for (const auto &fp : m.functions)
                funcs[fp.first.toStdString()] = fp.second;
            moduleNode["functions"] = funcs;
            root["modules"].push_back(moduleNode);
        }
        YAML::Emitter emitter;
        emitter << root;

        std_msgs::String msg;
        msg.data = emitter.c_str();
        modules_update_pub_.publish(msg);
    }
    catch (const std::exception &e)
    {
        qCritical() << "生成YAML发布消息失败: " << e.what();
    }

    refreshTable();
    QMessageBox::information(nullptr, "提示", "模块信息已添加/更新！");
}

void Page3Controller::onDeleteClicked()
{
    QString idStr = ui_->fun_id->text().trimmed();
    if (idStr.isEmpty())
    {
        QMessageBox::warning(nullptr, "警告", "请输入要删除的模块ID");
        return;
    }

    bool ok = false;
    int moduleId = idStr.toInt(&ok, 16);
    if (!ok || moduleId < 0)
    {
        QMessageBox::warning(nullptr, "警告", "模块ID必须为有效的十六进制正整数");
        return;
    }
    if (moduleId == 0)
    {
        QMessageBox::warning(nullptr, "警告", "ID 0x00（无模块）为系统保留项，不可删除！");
        return;
    }

    ModuleInfo foundInfo;
    if (!yaml_mgr_->findModule(moduleId, foundInfo))
    {
        QMessageBox::information(nullptr, "提示",
            QString("ID 0x%1 对应的模块不存在，无需删除")
                .arg(moduleId, 2, 16, QChar('0'))
                .toUpper());
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        nullptr, "确认删除",
        QString("确定要删除模块 \"%1\" (ID: 0x%2) 吗？")
            .arg(foundInfo.name)
            .arg(moduleId, 2, 16, QChar('0'))
            .toUpper(),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    yaml_mgr_->deleteModule(moduleId);
    refreshTable();
    yaml_mgr_->saveToFile(yaml_mgr_->yamlPath());

    // 发布更新
    try
    {
        YAML::Node root;
        root["modules"] = YAML::Node(YAML::NodeType::Sequence);
        for (const auto &pair : yaml_mgr_->allModules())
        {
            int id = pair.first;
            const ModuleInfo &m = pair.second;
            YAML::Node moduleNode;
            moduleNode["id"] = id;
            moduleNode["name"] = m.name.toStdString();
            moduleNode["description"] = m.desc.toStdString();
            YAML::Node funcs;
            for (const auto &fp : m.functions)
                funcs[fp.first.toStdString()] = fp.second;
            moduleNode["functions"] = funcs;
            root["modules"].push_back(moduleNode);
        }
        YAML::Emitter emitter;
        emitter << root;

        std_msgs::String msg;
        msg.data = emitter.c_str();
        modules_update_pub_.publish(msg);
    }
    catch (const std::exception &e)
    {
        qCritical() << "生成YAML发布消息失败: " << e.what();
    }

    ui_->fun_id->clear();
    QMessageBox::information(nullptr, "提示",
        QString("模块 0x%1 已删除并保存")
            .arg(moduleId, 2, 16, QChar('0'))
            .toUpper());
}

void Page3Controller::onClearClicked()
{
    ui_->fun_id->clear();
    ui_->fun_name->clear();
    ui_->fun_int->clear();

    QList<QLineEdit *> nameEdits = {
        ui_->fun_name_1, ui_->fun_name_2, ui_->fun_name_3,
        ui_->fun_name_4, ui_->fun_name_5, ui_->fun_name_6,
        ui_->fun_name_7, ui_->fun_name_8, ui_->fun_name_9};
    QList<QLineEdit *> cmdEdits = {
        ui_->fun_zhiling_1, ui_->fun_zhiling_2, ui_->fun_zhiling_3,
        ui_->fun_zhiling_4, ui_->fun_zhiling_5, ui_->fun_zhiling_6,
        ui_->fun_zhiling_7, ui_->fun_zhiling_8, ui_->fun_zhiling_9};
    for (auto e : nameEdits)
        e->clear();
    for (auto e : cmdEdits)
        e->clear();
}
