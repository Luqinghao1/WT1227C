/*
 * 文件名: dataeditorwidget.cpp
 * 文件作用: 数据编辑器主窗口实现文件
 * 功能描述:
 * 1. 实现了表格的增删改查。
 * 2. 实现了 CSV/TXT/XLS(文本型) 的通用读取，解决了乱码和无法打开问题。
 * 3. 实现了数据的可靠保存与恢复（修复了打开工程文件数据丢失问题）。
 * 4. 彻底修复了右键菜单交互：
 * - 删除了编辑状态下的英文菜单。
 * - 修复了浏览状态下增删行无效的问题。
 */

#include "dataeditorwidget.h"
#include "ui_dataeditorwidget.h"
#include "datecolumndialog.h"
#include "datacalculate.h"
#include "modelparameter.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QTextCodec>
#include <QLineEdit>
#include <QEvent>

// ============================================================================
// 内部类：NoContextMenuDelegate 实现
// 用于拦截单元格编辑器(QLineEdit)的右键事件，删除英文菜单
// ============================================================================
class EditorEventFilter : public QObject {
public:
    EditorEventFilter(QObject *parent) : QObject(parent) {}
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::ContextMenu) {
            // 返回 true 表示吃掉该事件，不让 QLineEdit 处理，从而不弹出菜单
            return true;
        }
        return QObject::eventFilter(obj, event);
    }
};

QWidget *NoContextMenuDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                             const QModelIndex &index) const
{
    QWidget *editor = QStyledItemDelegate::createEditor(parent, option, index);
    if (editor) {
        // 为编辑器安装事件过滤器
        editor->installEventFilter(new EditorEventFilter(editor));
    }
    return editor;
}

// ============================================================================
// DataEditorWidget 实现
// ============================================================================

DataEditorWidget::DataEditorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataEditorWidget),
    m_dataModel(new QStandardItemModel(this)),
    m_proxyModel(new QSortFilterProxyModel(this)),
    m_undoStack(new QUndoStack(this))
{
    ui->setupUi(this);
    initUI();
    setupModel();
    setupConnections();

    // 初始化搜索防抖定时器
    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(300);
    connect(m_searchTimer, &QTimer::timeout, this, [this](){
        m_proxyModel->setFilterWildcard(ui->searchLineEdit->text());
    });
}

DataEditorWidget::~DataEditorWidget()
{
    delete ui;
}

void DataEditorWidget::initUI()
{
    // 设置右键策略，响应表格视图态的右键
    ui->dataTableView->setContextMenuPolicy(Qt::CustomContextMenu);

    // 应用自定义委托，屏蔽编辑态的右键菜单
    ui->dataTableView->setItemDelegate(new NoContextMenuDelegate(this));

    updateButtonsState();
}

void DataEditorWidget::setupModel()
{
    m_proxyModel->setSourceModel(m_dataModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    ui->dataTableView->setModel(m_proxyModel);

    // 允许单选或多选
    ui->dataTableView->setSelectionBehavior(QAbstractItemView::SelectItems);
    ui->dataTableView->setSelectionMode(QAbstractItemView::ContiguousSelection);
}

void DataEditorWidget::setupConnections()
{
    connect(ui->btnOpenFile, &QPushButton::clicked, this, &DataEditorWidget::onOpenFile);
    connect(ui->btnSave, &QPushButton::clicked, this, &DataEditorWidget::onSave);
    connect(ui->btnDefineColumns, &QPushButton::clicked, this, &DataEditorWidget::onDefineColumns);
    connect(ui->btnTimeConvert, &QPushButton::clicked, this, &DataEditorWidget::onTimeConvert);
    connect(ui->btnPressureDropCalc, &QPushButton::clicked, this, &DataEditorWidget::onPressureDropCalc);

    connect(ui->searchLineEdit, &QLineEdit::textChanged, this, &DataEditorWidget::onSearchTextChanged);

    // 连接表格右键信号
    connect(ui->dataTableView, &QTableView::customContextMenuRequested, this, &DataEditorWidget::onCustomContextMenu);

    connect(m_dataModel, &QStandardItemModel::itemChanged, this, &DataEditorWidget::onModelDataChanged);
}

void DataEditorWidget::updateButtonsState()
{
    bool hasData = m_dataModel->rowCount() > 0 && m_dataModel->columnCount() > 0;
    ui->btnSave->setEnabled(hasData);
    ui->btnDefineColumns->setEnabled(hasData);
    ui->btnTimeConvert->setEnabled(hasData);
    ui->btnPressureDropCalc->setEnabled(hasData);
}

// ============================================================================
// 公共接口
// ============================================================================

QStandardItemModel* DataEditorWidget::getDataModel() const { return m_dataModel; }
QString DataEditorWidget::getCurrentFileName() const { return m_currentFilePath; }
bool DataEditorWidget::hasData() const { return m_dataModel->rowCount() > 0; }

void DataEditorWidget::loadData(const QString& filePath, const QString& fileType)
{
    if (loadFileInternal(filePath)) {
        emit fileChanged(filePath, fileType);
    }
}

// ============================================================================
// 文件加载 (通用文本读取，解决Excel/乱码问题)
// ============================================================================

void DataEditorWidget::onOpenFile()
{
    QString filter = "支持的文件 (*.csv *.txt *.json *.xls *.xlsx);;文本数据 (*.csv *.txt *.xls *.xlsx);;JSON (*.json)";
    QString path = QFileDialog::getOpenFileName(this, "打开数据文件", "", filter);
    if (path.isEmpty()) return;

    loadData(path, "auto");
}

bool DataEditorWidget::loadFileInternal(const QString& path)
{
    m_currentFilePath = path;
    ui->filePathLabel->setText("当前文件: " + path);

    bool success = false;
    m_dataModel->clear();
    m_columnDefinitions.clear();

    if (path.endsWith(".json", Qt::CaseInsensitive)) {
        success = loadJson(path);
    } else {
        // 对于 xls, xlsx, csv, txt，统一尝试用智能文本加载器读取
        // 这能兼容 "另存为xls的文本文件"，也是最通用的方式
        success = loadTextBasedFile(path);
    }

    if (success) {
        ui->statusLabel->setText("加载成功");
        updateButtonsState();
        emit dataChanged();
    } else {
        QMessageBox::critical(this, "错误", "文件加载失败。\n请确认文件格式正确，若是Excel文件请尝试另存为CSV。");
        ui->statusLabel->setText("加载失败");
    }

    return success;
}

// 智能探测编码并读取文本
QString DataEditorWidget::detectAndReadText(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();

    QByteArray data = file.readAll();
    file.close();

    // 1. 尝试 UTF-8
    QTextCodec::ConverterState state;
    QTextCodec* codecUtf8 = QTextCodec::codecForName("UTF-8");
    QString text = codecUtf8->toUnicode(data.constData(), data.size(), &state);

    // 2. 如果 UTF-8 转换出现无效字符，或者看起来乱码，尝试本地编码 (通常是 GBK)
    if (state.invalidChars > 0) {
        QTextCodec* codecLocal = QTextCodec::codecForLocale();
        // 强制回退到 GBK 防止 System 是其他编码
        if (!codecLocal) codecLocal = QTextCodec::codecForName("GBK");
        if (codecLocal) {
            return codecLocal->toUnicode(data);
        }
    }
    return text;
}

bool DataEditorWidget::loadTextBasedFile(const QString& path)
{
    QString content = detectAndReadText(path);
    if (content.isEmpty()) return false;

    QTextStream in(&content);
    bool isHeader = true;

    // 智能探测分隔符：读第一行，看逗号还是制表符多
    QChar separator = ',';
    if (!in.atEnd()) {
        QString firstLine = content.section('\n', 0, 0);
        if (firstLine.count('\t') > firstLine.count(',')) {
            separator = '\t'; // 很多 .xls 其实是制表符分隔的文本
        }
    }

    in.seek(0);

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;

        QStringList fields = line.split(separator);

        // 去除引号
        for(int i=0; i<fields.size(); ++i) {
            QString f = fields[i].trimmed();
            if (f.startsWith('"') && f.endsWith('"')) f = f.mid(1, f.length()-2);
            fields[i] = f;
        }

        if (isHeader) {
            m_dataModel->setHorizontalHeaderLabels(fields);
            for(const QString& h : fields) {
                ColumnDefinition def;
                def.name = h;
                m_columnDefinitions.append(def);
            }
            isHeader = false;
        } else {
            QList<QStandardItem*> items;
            for(const QString& field : fields) {
                items.append(new QStandardItem(field));
            }
            m_dataModel->appendRow(items);
        }
    }
    return true;
}

bool DataEditorWidget::loadJson(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isArray()) {
        deserializeJsonToModel(doc.array());
        return true;
    }
    return false;
}

// ============================================================================
// 数据保存与恢复 (关键修复：数据丢失问题)
// ============================================================================

void DataEditorWidget::onSave()
{
    // 1. 序列化当前表格数据
    QJsonArray data = serializeModelToJson();

    // 2. 存入 ModelParameter (写入 _date.json)
    ModelParameter::instance()->saveTableData(data);

    // 3. 同时触发项目主文件保存
    ModelParameter::instance()->saveProject();

    QMessageBox::information(this, "保存", "数据已成功保存至项目文件。");
}

// [被 MainWindow 调用] 从项目加载数据
void DataEditorWidget::loadFromProjectData()
{
    qDebug() << "DataEditorWidget: 开始从项目恢复数据...";
    QJsonArray data = ModelParameter::instance()->getTableData();

    if (!data.isEmpty()) {
        deserializeJsonToModel(data);
        ui->statusLabel->setText("已恢复项目数据");
        updateButtonsState();
        qDebug() << "DataEditorWidget: 数据恢复成功，行数:" << m_dataModel->rowCount();
    } else {
        qDebug() << "DataEditorWidget: 项目中无表格数据";
        m_dataModel->clear();
        m_columnDefinitions.clear();
        ui->statusLabel->setText("无数据");
        updateButtonsState();
    }
}

QJsonArray DataEditorWidget::serializeModelToJson() const
{
    QJsonArray array;
    // 第一项存表头
    QJsonObject headerObj;
    QJsonArray headers;
    for(int i=0; i<m_dataModel->columnCount(); ++i) {
        headers.append(m_dataModel->headerData(i, Qt::Horizontal).toString());
    }
    headerObj["headers"] = headers;
    array.append(headerObj);

    // 后续存数据
    for(int i=0; i<m_dataModel->rowCount(); ++i) {
        QJsonArray rowArr;
        for(int j=0; j<m_dataModel->columnCount(); ++j) {
            rowArr.append(m_dataModel->item(i, j)->text());
        }
        QJsonObject rowObj;
        rowObj["row_data"] = rowArr;
        array.append(rowObj);
    }
    return array;
}

void DataEditorWidget::deserializeJsonToModel(const QJsonArray& array)
{
    m_dataModel->clear();
    m_columnDefinitions.clear();

    if (array.isEmpty()) return;

    // 1. 恢复表头
    QJsonObject headerObj = array.first().toObject();
    if (headerObj.contains("headers")) {
        QJsonArray headers = headerObj["headers"].toArray();
        QStringList headerLabels;
        for(const auto& h : headers) headerLabels << h.toString();
        m_dataModel->setHorizontalHeaderLabels(headerLabels);

        // 重建列定义
        for(const QString& h : headerLabels) {
            ColumnDefinition def;
            def.name = h;
            m_columnDefinitions.append(def);
        }
    }

    // 2. 恢复数据
    for(int i=1; i<array.size(); ++i) {
        QJsonObject rowObj = array[i].toObject();
        if (rowObj.contains("row_data")) {
            QJsonArray rowArr = rowObj["row_data"].toArray();
            QList<QStandardItem*> items;
            for(const auto& val : rowArr) {
                items.append(new QStandardItem(val.toString()));
            }
            m_dataModel->appendRow(items);
        }
    }
}

// ============================================================================
// 功能模块
// ============================================================================

void DataEditorWidget::onDefineColumns()
{
    QStringList currentHeaders;
    for(int i=0; i<m_dataModel->columnCount(); ++i)
        currentHeaders << m_dataModel->headerData(i, Qt::Horizontal).toString();

    DataColumnDialog dlg(currentHeaders, m_columnDefinitions, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_columnDefinitions = dlg.getColumnDefinitions();
        for(int i=0; i<m_columnDefinitions.size(); ++i) {
            if (i < m_dataModel->columnCount()) {
                m_dataModel->setHeaderData(i, Qt::Horizontal, m_columnDefinitions[i].name);
            }
        }
        emit dataChanged();
    }
}

void DataEditorWidget::onTimeConvert()
{
    DataCalculate calculator;
    QStringList headers;
    for(int i=0; i<m_dataModel->columnCount(); ++i)
        headers << m_dataModel->headerData(i, Qt::Horizontal).toString();

    TimeConversionDialog dlg(headers, this);
    if (dlg.exec() == QDialog::Accepted) {
        TimeConversionConfig config = dlg.getConversionConfig();
        TimeConversionResult res = calculator.convertTimeColumn(m_dataModel, m_columnDefinitions, config);

        if (res.success) QMessageBox::information(this, "成功", "时间转换完成");
        else QMessageBox::warning(this, "失败", res.errorMessage);
    }
}

void DataEditorWidget::onPressureDropCalc()
{
    DataCalculate calculator;
    PressureDropResult res = calculator.calculatePressureDrop(m_dataModel, m_columnDefinitions);

    if (res.success) QMessageBox::information(this, "成功", "压降计算完成");
    else QMessageBox::warning(this, "失败", res.errorMessage);
}

// ============================================================================
// 右键菜单与编辑 (核心修复：点击操作无效问题)
// ============================================================================

void DataEditorWidget::onSearchTextChanged()
{
    m_searchTimer->start();
}

void DataEditorWidget::onCustomContextMenu(const QPoint& pos)
{
    QMenu menu(this);
    // 强制样式：白底黑字
    menu.setStyleSheet("QMenu { background-color: white; color: black; border: 1px solid #ccc; }"
                       "QMenu::item { padding: 5px 20px; }"
                       "QMenu::item:selected { background-color: #e0e0e0; }");

    menu.addAction("添加行", this, &DataEditorWidget::onAddRow);
    menu.addAction("删除选中行", this, &DataEditorWidget::onDeleteRow);
    menu.addSeparator();
    menu.addAction("添加列", this, &DataEditorWidget::onAddCol);
    menu.addAction("删除选中列", this, &DataEditorWidget::onDeleteCol);

    menu.exec(ui->dataTableView->mapToGlobal(pos));
}

void DataEditorWidget::onAddRow()
{
    // 获取当前焦点行，如果没有焦点则加到末尾
    QModelIndex currIdx = ui->dataTableView->currentIndex();
    int row = (currIdx.isValid()) ? currIdx.row() + 1 : m_dataModel->rowCount();

    int colCount = m_dataModel->columnCount() > 0 ? m_dataModel->columnCount() : 1;
    QList<QStandardItem*> items;
    for(int i=0; i<colCount; ++i) items << new QStandardItem("");

    m_dataModel->insertRow(row, items);
    updateButtonsState();
}

void DataEditorWidget::onDeleteRow()
{
    // 获取所有选中的行号
    QModelIndexList idxs = ui->dataTableView->selectionModel()->selectedRows();
    if (idxs.isEmpty()) {
        // 如果只选了单元格没选整行，也尝试删除
        QModelIndex curr = ui->dataTableView->currentIndex();
        if (curr.isValid()) {
            m_dataModel->removeRow(curr.row());
            updateButtonsState();
        }
        return;
    }

    // 从后往前删，防止索引错乱
    QList<int> rows;
    for(auto idx : idxs) rows << m_proxyModel->mapToSource(idx).row();
    std::sort(rows.begin(), rows.end(), std::greater<int>());

    for(int r : rows) m_dataModel->removeRow(r);
    updateButtonsState();
}

void DataEditorWidget::onAddCol()
{
    m_dataModel->insertColumn(m_dataModel->columnCount());
    ColumnDefinition def;
    def.name = "新列";
    m_columnDefinitions.append(def);
    m_dataModel->setHeaderData(m_dataModel->columnCount()-1, Qt::Horizontal, "新列");
}

void DataEditorWidget::onDeleteCol()
{
    QModelIndexList idxs = ui->dataTableView->selectionModel()->selectedColumns();
    if (idxs.isEmpty()) return;

    QList<int> cols;
    for(auto idx : idxs) cols << m_proxyModel->mapToSource(idx).column();
    std::sort(cols.begin(), cols.end(), std::greater<int>());

    for(int c : cols) {
        m_dataModel->removeColumn(c);
        if(c < m_columnDefinitions.size()) m_columnDefinitions.removeAt(c);
    }
    updateButtonsState();
}

void DataEditorWidget::onModelDataChanged()
{
    // 数据变更处理
}
