/*
 * 文件名: datecolumndialog.cpp
 * 文件作用: 列定义对话框实现文件
 * 功能描述:
 * 1. 动态生成每一列的配置行。
 * 2. 类型为“自定义”时，允许用户直接在下拉框中输入类型名称。
 * 3. 单位为“自定义”时，允许用户直接在下拉框中输入单位名称。
 */

#include "datecolumndialog.h"
#include "ui_datecolumndialog.h"
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDebug>

DataColumnDialog::DataColumnDialog(const QStringList& columnNames,
                                   const QList<ColumnDefinition>& definitions,
                                   QWidget* parent)
    : QDialog(parent), ui(new Ui::DataColumnDialog), m_columnNames(columnNames), m_definitions(definitions)
{
    ui->setupUi(this);
    // ... 连接信号槽，初始化行 (参考之前的实现) ...
    connect(ui->btnOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->btnPreset, &QPushButton::clicked, this, &DataColumnDialog::onLoadPresetClicked);
    connect(ui->btnReset, &QPushButton::clicked, this, &DataColumnDialog::onResetClicked);

    if (m_definitions.isEmpty()) {
        for (const QString& name : columnNames) {
            ColumnDefinition def; def.name = name; def.type = WellTestColumnType::Custom;
            m_definitions.append(def);
        }
    }
    setupColumnRows();
}

DataColumnDialog::~DataColumnDialog()
{
    delete ui;
}

void DataColumnDialog::setupColumnRows()
{
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(ui->scrollContent->layout());
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);

    QStringList typeNames = {
        "序号", "日期", "时刻", "时间", "压力", "温度", "流量", "深度", "粘度", "密度",
        "渗透率", "孔隙度", "井半径", "表皮系数", "距离", "体积", "压降", "自定义"
    };

    for (int i = 0; i < m_columnNames.size(); ++i) {
        QWidget* rowWidget = new QWidget;
        QHBoxLayout* rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0, 0, 0, 0);

        // 1. 原列名
        QLabel* originalNameLabel = new QLabel(QString("原列名: %1").arg(m_columnNames[i]));
        originalNameLabel->setFixedWidth(150);
        originalNameLabel->setStyleSheet("font-weight: bold; color: black;");
        rowLayout->addWidget(originalNameLabel);

        // 2. 类型选择
        QComboBox* typeCombo = new QComboBox;
        typeCombo->addItems(typeNames);
        typeCombo->setFixedWidth(120);

        // 恢复设置
        if (i < m_definitions.size()) {
            typeCombo->setCurrentIndex(static_cast<int>(m_definitions[i].type));
            if (m_definitions[i].type == WellTestColumnType::Custom) {
                typeCombo->setEditable(true); // 自定义类型可编辑
                typeCombo->setCurrentText(m_definitions[i].name.split('\\').first());
            } else {
                typeCombo->setEditable(false);
            }
        } else {
            typeCombo->setCurrentIndex(17); // 默认自定义
            typeCombo->setEditable(true);
        }

        rowLayout->addWidget(typeCombo);
        m_typeComboBoxes.append(typeCombo);

        connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DataColumnDialog::onTypeChanged);
        connect(typeCombo, &QComboBox::editTextChanged, this, &DataColumnDialog::onCustomTextChanged);

        // 3. 单位选择
        QComboBox* unitCombo = new QComboBox;
        unitCombo->setFixedWidth(100);
        // 单位框的可编辑状态由类型决定，但如果选了"自定义"单位，它自己也会变可编辑
        rowLayout->addWidget(unitCombo);
        m_unitComboBoxes.append(unitCombo);

        connect(unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DataColumnDialog::onUnitChanged);
        connect(unitCombo, &QComboBox::editTextChanged, this, &DataColumnDialog::onCustomTextChanged);

        // 4. 必需
        QCheckBox* requiredCheck = new QCheckBox("必需");
        requiredCheck->setStyleSheet("color: black;");
        if (i < m_definitions.size()) {
            requiredCheck->setChecked(m_definitions[i].isRequired);
        }
        rowLayout->addWidget(requiredCheck);
        m_requiredChecks.append(requiredCheck);

        // 5. 预览
        QLabel* previewLabel = new QLabel;
        previewLabel->setStyleSheet("color: #28a745; font-weight: bold;");
        rowLayout->addWidget(previewLabel);
        m_previewLabels.append(previewLabel);

        layout->addWidget(rowWidget);

        // 初始化
        WellTestColumnType currentType = static_cast<WellTestColumnType>(typeCombo->currentIndex());
        updateUnitsForType(currentType, unitCombo);

        // 恢复单位
        if (i < m_definitions.size()) {
            int unitIdx = unitCombo->findText(m_definitions[i].unit);
            if (unitIdx >= 0) {
                unitCombo->setCurrentIndex(unitIdx);
            } else if (!m_definitions[i].unit.isEmpty()) {
                // 自定义单位，不在列表中
                unitCombo->setEditable(true);
                unitCombo->setCurrentText(m_definitions[i].unit);
            }
        }

        updatePreviewLabel(i);
    }

    layout->addStretch();
}

void DataColumnDialog::onTypeChanged(int index)
{
    QComboBox* senderCombo = qobject_cast<QComboBox*>(sender());
    if (!senderCombo) return;

    int rowIdx = -1;
    for(int i=0; i<m_typeComboBoxes.size(); ++i) {
        if(m_typeComboBoxes[i] == senderCombo) {
            rowIdx = i;
            break;
        }
    }

    if (rowIdx != -1) {
        WellTestColumnType type = static_cast<WellTestColumnType>(index);

        // 类型为"自定义" (索引17) 时可编辑
        bool isCustomType = (index == 17);
        senderCombo->setEditable(isCustomType);

        // 更新单位列表
        updateUnitsForType(type, m_unitComboBoxes[rowIdx]);
        updatePreviewLabel(rowIdx);
    }
}

void DataColumnDialog::onUnitChanged(int index)
{
    QComboBox* senderCombo = qobject_cast<QComboBox*>(sender());
    if (!senderCombo) return;

    int rowIdx = -1;
    for(int i=0; i<m_unitComboBoxes.size(); ++i) {
        if(m_unitComboBoxes[i] == senderCombo) {
            rowIdx = i;
            break;
        }
    }

    if (rowIdx != -1) {
        QString currentText = senderCombo->currentText();
        // 如果选中了"自定义"选项，让下拉框变为可编辑
        if (currentText == "自定义") {
            senderCombo->setEditable(true);
            senderCombo->clearEditText(); // 清空让用户输入
        } else {
            // 如果不是"自定义"，通常不可编辑，除非类型本身是Custom且我们想允许任意单位
            // 这里简化逻辑：只要不是"自定义"选项，就禁止编辑，规避混淆
            senderCombo->setEditable(false);
        }
        updatePreviewLabel(rowIdx);
    }
}

void DataColumnDialog::onCustomTextChanged(const QString& text)
{
    Q_UNUSED(text);
    QWidget* senderWidget = qobject_cast<QWidget*>(sender());
    for(int i=0; i<m_typeComboBoxes.size(); ++i) {
        if (m_typeComboBoxes[i] == senderWidget || m_unitComboBoxes[i] == senderWidget) {
            updatePreviewLabel(i);
            break;
        }
    }
}

void DataColumnDialog::updateUnitsForType(WellTestColumnType type, QComboBox* unitCombo)
{
    unitCombo->blockSignals(true);
    unitCombo->clear();
    unitCombo->setEditable(false); // 重置为不可编辑

    switch (type) {
    case WellTestColumnType::SerialNumber:
        unitCombo->addItems({"-", "自定义"}); break;
    case WellTestColumnType::Date:
        unitCombo->addItems({"-", "yyyy-MM-dd", "yyyy/MM/dd", "自定义"}); break;
    case WellTestColumnType::TimeOfDay:
        unitCombo->addItems({"-", "hh:mm:ss", "hh:mm", "自定义"}); break;
    case WellTestColumnType::Time:
        unitCombo->addItems({"h", "min", "s", "day", "自定义"}); break;
    case WellTestColumnType::Pressure:
    case WellTestColumnType::PressureDrop:
        unitCombo->addItems({"MPa", "kPa", "Pa", "psi", "bar", "atm", "自定义"}); break;
    case WellTestColumnType::Temperature:
        unitCombo->addItems({"°C", "°F", "K", "自定义"}); break;
    case WellTestColumnType::FlowRate:
        unitCombo->addItems({"m³/d", "m³/h", "L/s", "bbl/d", "自定义"}); break;
    case WellTestColumnType::Depth:
    case WellTestColumnType::Distance:
        unitCombo->addItems({"m", "ft", "km", "自定义"}); break;
    case WellTestColumnType::Viscosity:
        unitCombo->addItems({"mPa·s", "cP", "Pa·s", "自定义"}); break;
    case WellTestColumnType::Density:
        unitCombo->addItems({"kg/m³", "g/cm³", "lb/ft³", "自定义"}); break;
    case WellTestColumnType::Permeability:
        unitCombo->addItems({"mD", "D", "μm²", "自定义"}); break;
    case WellTestColumnType::Porosity:
        unitCombo->addItems({"%", "fraction", "自定义"}); break;
    case WellTestColumnType::WellRadius:
        unitCombo->addItems({"m", "ft", "cm", "in", "自定义"}); break;
    case WellTestColumnType::SkinFactor:
        unitCombo->addItems({"dimensionless", "自定义"}); break;
    case WellTestColumnType::Volume:
        unitCombo->addItems({"m³", "L", "bbl", "ft³", "自定义"}); break;
    default: // Custom
        unitCombo->addItems({"-", "自定义"}); break;
    }

    unitCombo->blockSignals(false);
}

void DataColumnDialog::updatePreviewLabel(int index)
{
    QString typeStr = m_typeComboBoxes[index]->currentText();
    QString unitStr = m_unitComboBoxes[index]->currentText();

    if (unitStr == "-" || unitStr.isEmpty() || unitStr == "自定义") {
        m_previewLabels[index]->setText(typeStr);
    } else {
        m_previewLabels[index]->setText(QString("%1\\%2").arg(typeStr).arg(unitStr));
    }
}

void DataColumnDialog::onLoadPresetClicked()
{
    for (int i = 0; i < m_columnNames.size(); ++i) {
        QString name = m_columnNames[i].toLower();
        int typeIdx = 17;
        QString unitToSel = "-";

        if (name.contains("序号") || name == "no") { typeIdx = 0; }
        else if (name.contains("日期") || name.contains("date")) { typeIdx = 1; unitToSel = "yyyy-MM-dd"; }
        else if (name.contains("时刻") || name.contains("time")) { typeIdx = 2; unitToSel = "hh:mm:ss"; }
        else if (name.contains("时间") || name == "t") { typeIdx = 3; unitToSel = "h"; }
        else if (name.contains("压力") || name.contains("pressure") || name == "p") { typeIdx = 4; unitToSel = "MPa"; }
        else if (name.contains("流量") || name.contains("flow") || name == "q") { typeIdx = 6; unitToSel = "m³/d"; }

        m_typeComboBoxes[i]->setCurrentIndex(typeIdx);
        updateUnitsForType(static_cast<WellTestColumnType>(typeIdx), m_unitComboBoxes[i]);

        int uIdx = m_unitComboBoxes[i]->findText(unitToSel);
        if (uIdx >= 0) m_unitComboBoxes[i]->setCurrentIndex(uIdx);
    }
}

void DataColumnDialog::onResetClicked()
{
    for (int i = 0; i < m_typeComboBoxes.size(); ++i) {
        m_typeComboBoxes[i]->setCurrentIndex(17);
        m_typeComboBoxes[i]->setEditable(true);
        m_requiredChecks[i]->setChecked(false);
    }
}

QList<ColumnDefinition> DataColumnDialog::getColumnDefinitions() const
{
    QList<ColumnDefinition> result;
    for (int i = 0; i < m_columnNames.size(); ++i) {
        ColumnDefinition def;
        QString typeStr = m_typeComboBoxes[i]->currentText();
        QString unitStr = m_unitComboBoxes[i]->currentText();

        // 只有当单位有效且不是占位符时才组合
        if (unitStr == "-" || unitStr.isEmpty() || unitStr == "自定义") {
            def.name = typeStr;
            def.unit = "";
        } else {
            def.name = QString("%1\\%2").arg(typeStr).arg(unitStr);
            def.unit = unitStr;
        }

        def.type = static_cast<WellTestColumnType>(m_typeComboBoxes[i]->currentIndex());
        def.isRequired = m_requiredChecks[i]->isChecked();

        result.append(def);
    }
    return result;
}
