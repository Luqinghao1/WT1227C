/*
 * 文件名: dataeditorwidget.h
 * 文件作用: 数据编辑器主窗口头文件
 * 功能描述:
 * 1. 管理数据表格的显示、编辑、导入导出。
 * 2. 负责与 ModelParameter 进行数据同步（保存/加载）。
 * 3. 实现了右键菜单的定制和编辑状态下的菜单屏蔽。
 */

#ifndef DATAEDITORWIDGET_H
#define DATAEDITORWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QUndoStack>
#include <QMenu>
#include <QJsonArray>
#include <QStyledItemDelegate>
#include <QTimer>

// 定义列的枚举和结构体
enum class WellTestColumnType {
    SerialNumber, Date, Time, TimeOfDay, Pressure, Temperature, FlowRate,
    Depth, Viscosity, Density, Permeability, Porosity, WellRadius,
    SkinFactor, Distance, Volume, PressureDrop, Custom
};

struct ColumnDefinition {
    QString name;
    WellTestColumnType type;
    QString unit;
    bool isRequired;
    int decimalPlaces;

    ColumnDefinition() : type(WellTestColumnType::Custom), isRequired(false), decimalPlaces(3) {}
};

namespace Ui {
class DataEditorWidget;
}

// ----------------------------------------------------------------------------
// 自定义委托：用于屏蔽编辑状态下 QLineEdit 的默认右键菜单
// ----------------------------------------------------------------------------
class NoContextMenuDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit NoContextMenuDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
};

// ----------------------------------------------------------------------------
// 主编辑器类
// ----------------------------------------------------------------------------
class DataEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataEditorWidget(QWidget *parent = nullptr);
    ~DataEditorWidget();

    // 外部调用加载项目数据接口（从 ModelParameter 加载）
    // 修复：确保项目打开时数据能恢复
    void loadFromProjectData();

    // 获取当前的数据模型
    QStandardItemModel* getDataModel() const;

    // 加载指定文件
    void loadData(const QString& filePath, const QString& fileType = "auto");

    // 获取当前打开的文件名
    QString getCurrentFileName() const;

    // 判断当前是否有数据
    bool hasData() const;

    // 获取当前的列定义
    QList<ColumnDefinition> getColumnDefinitions() const { return m_columnDefinitions; }

signals:
    void dataChanged();
    void fileChanged(const QString& filePath, const QString& fileType);

private slots:
    void onOpenFile();
    void onSave();
    void onDefineColumns();
    void onTimeConvert();
    void onPressureDropCalc();

    void onSearchTextChanged();

    // 右键菜单相关
    void onCustomContextMenu(const QPoint& pos);
    void onAddRow();
    void onDeleteRow();
    void onAddCol();
    void onDeleteCol();

    void onModelDataChanged();

private:
    Ui::DataEditorWidget *ui;

    QStandardItemModel* m_dataModel;
    QSortFilterProxyModel* m_proxyModel;
    QUndoStack* m_undoStack;

    QList<ColumnDefinition> m_columnDefinitions;
    QString m_currentFilePath;
    QMenu* m_contextMenu;
    QTimer* m_searchTimer;

    void initUI();
    void setupConnections();
    void setupModel();
    void updateButtonsState();

    // 文件加载逻辑：核心修复部分
    bool loadFileInternal(const QString& path);
    // 统一使用文本流读取，支持CSV和"伪"xls
    bool loadTextBasedFile(const QString& path);
    bool loadJson(const QString& path);

    // 探测编码
    QString detectAndReadText(const QString& path);

    // 序列化与反序列化（修复数据丢失问题）
    QJsonArray serializeModelToJson() const;
    void deserializeJsonToModel(const QJsonArray& array);
};

#endif // DATAEDITORWIDGET_H
