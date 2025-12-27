/*
 * 文件名: datecolumndialog.h
 * 文件作用: 列定义对话框头文件
 * 功能描述:
 * 1. 定义数据列属性设置的弹窗界面类 DataColumnDialog。
 * 2. 声明列定义的数据结构和交互槽函数。
 * 3. 优化交互：自定义单位时直接编辑下拉框。
 */

#ifndef DATECOLUMNDIALOG_H
#define DATECOLUMNDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QList>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include "dataeditorwidget.h" // 包含 WellTestColumnType 和 ColumnDefinition 定义

namespace Ui {
class DataColumnDialog;
}

class DataColumnDialog : public QDialog
{
    Q_OBJECT

public:
    // 构造函数：需要传入当前的列名列表，以及可选的已有定义
    explicit DataColumnDialog(const QStringList& columnNames,
                              const QList<ColumnDefinition>& definitions = QList<ColumnDefinition>(),
                              QWidget* parent = nullptr);

    // 析构函数
    ~DataColumnDialog();

    // 获取用户设置完成后的列定义列表
    QList<ColumnDefinition> getColumnDefinitions() const;

private slots:
    // 类型下拉框变化时触发，用于更新对应的单位选项
    void onTypeChanged(int index);

    // 单位下拉框变化时触发
    void onUnitChanged(int index);

    // 自定义类型或单位的文本发生变化时触发
    void onCustomTextChanged(const QString& text);

    // 自动识别按钮点击槽函数
    void onLoadPresetClicked();

    // 重置按钮点击槽函数
    void onResetClicked();

private:
    Ui::DataColumnDialog *ui;

    // 初始化界面控件，根据列数量动态生成设置行
    void setupColumnRows();

    // 根据选择的物理类型更新单位下拉框的内容
    void updateUnitsForType(WellTestColumnType type, QComboBox* unitCombo);

    // 更新预览标签的显示文本
    void updatePreviewLabel(int index);

    // 原始列名列表
    QStringList m_columnNames;
    // 列定义数据
    QList<ColumnDefinition> m_definitions;

    // 动态创建的控件列表
    QList<QComboBox*> m_typeComboBoxes;   // 类型选择框列表
    QList<QComboBox*> m_unitComboBoxes;   // 单位选择框列表
    QList<QCheckBox*> m_requiredChecks;   // 必需勾选框列表
    QList<QLabel*> m_previewLabels;       // 预览标签列表
};

#endif // DATECOLUMNDIALOG_H
