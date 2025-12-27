/*
 * 文件名: plottingdialog3.cpp
 * 文件作用: 压力导数曲线配置对话框实现
 * 功能描述:
 * 1. 修复了SpinBox按钮无效的问题（设置了合理的step和range）。
 * 2. 实现了调色盘颜色选择。
 */

#include "plottingdialog3.h"
#include "ui_plottingdialog3.h"
#include <QColorDialog>

int PlottingDialog3::s_counter = 1;

PlottingDialog3::PlottingDialog3(QStandardItemModel* model, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PlottingDialog3),
    m_dataModel(model),
    m_pressPointColor(Qt::red),
    m_pressLineColor(Qt::red),
    m_derivPointColor(Qt::blue),
    m_derivLineColor(Qt::blue)
{
    ui->setupUi(this);

    // 默认值
    ui->lineName->setText(QString("导数分析 %1").arg(s_counter++));
    ui->linePressLegend->setText("Delta P");
    ui->lineDerivLegend->setText("Derivative");
    ui->lineXLabel->setText("Time (h)");
    ui->lineYLabel->setText("Pressure / Derivative (MPa)");

    populateComboBoxes();
    setupStyleOptions();

    // 信号连接
    connect(ui->checkSmooth, &QCheckBox::toggled, this, &PlottingDialog3::onSmoothToggled);
    onSmoothToggled(ui->checkSmooth->isChecked());

    connect(ui->btnPressPointColor, &QPushButton::clicked, this, &PlottingDialog3::selectPressPointColor);
    connect(ui->btnPressLineColor, &QPushButton::clicked, this, &PlottingDialog3::selectPressLineColor);
    connect(ui->btnDerivPointColor, &QPushButton::clicked, this, &PlottingDialog3::selectDerivPointColor);
    connect(ui->btnDerivLineColor, &QPushButton::clicked, this, &PlottingDialog3::selectDerivLineColor);
}

PlottingDialog3::~PlottingDialog3()
{
    delete ui;
}

void PlottingDialog3::populateComboBoxes()
{
    if (!m_dataModel) return;
    QStringList headers;
    for(int i=0; i<m_dataModel->columnCount(); ++i) {
        QStandardItem* item = m_dataModel->horizontalHeaderItem(i);
        headers << (item ? item->text() : QString("列 %1").arg(i+1));
    }
    ui->comboTime->addItems(headers);
    ui->comboPress->addItems(headers);
}

void PlottingDialog3::setupStyleOptions()
{
    auto addShapes = [](QComboBox* box) {
        box->addItem("实心圆 (Disc)", (int)QCPScatterStyle::ssDisc);
        box->addItem("空心圆 (Circle)", (int)QCPScatterStyle::ssCircle);
        box->addItem("三角形 (Triangle)", (int)QCPScatterStyle::ssTriangle);
        box->addItem("菱形 (Diamond)", (int)QCPScatterStyle::ssDiamond);
        box->addItem("无 (None)", (int)QCPScatterStyle::ssNone);
    };
    auto addLines = [](QComboBox* box) {
        box->addItem("实线 (Solid)", (int)Qt::SolidLine);
        box->addItem("虚线 (Dash)", (int)Qt::DashLine);
        box->addItem("无 (None)", (int)Qt::NoPen);
    };

    addShapes(ui->comboPressShape);
    addLines(ui->comboPressLine);
    addShapes(ui->comboDerivShape);
    addLines(ui->comboDerivLine);

    // 默认样式
    ui->comboPressLine->setCurrentIndex(2); // NoPen
    ui->comboDerivShape->setCurrentIndex(2); // Triangle
    ui->comboDerivLine->setCurrentIndex(2); // NoPen

    updateColorButton(ui->btnPressPointColor, m_pressPointColor);
    updateColorButton(ui->btnPressLineColor, m_pressLineColor);
    updateColorButton(ui->btnDerivPointColor, m_derivPointColor);
    updateColorButton(ui->btnDerivLineColor, m_derivLineColor);
}

void PlottingDialog3::onSmoothToggled(bool checked)
{
    ui->spinSmooth->setEnabled(checked);
}

void PlottingDialog3::updateColorButton(QPushButton* btn, const QColor& color) {
    btn->setStyleSheet(QString("background-color: %1; border: 1px solid #555; border-radius: 3px;").arg(color.name()));
}

void PlottingDialog3::selectPressPointColor() {
    QColor c = QColorDialog::getColor(m_pressPointColor, this);
    if(c.isValid()) { m_pressPointColor = c; updateColorButton(ui->btnPressPointColor, c); }
}
void PlottingDialog3::selectPressLineColor() {
    QColor c = QColorDialog::getColor(m_pressLineColor, this);
    if(c.isValid()) { m_pressLineColor = c; updateColorButton(ui->btnPressLineColor, c); }
}
void PlottingDialog3::selectDerivPointColor() {
    QColor c = QColorDialog::getColor(m_derivPointColor, this);
    if(c.isValid()) { m_derivPointColor = c; updateColorButton(ui->btnDerivPointColor, c); }
}
void PlottingDialog3::selectDerivLineColor() {
    QColor c = QColorDialog::getColor(m_derivLineColor, this);
    if(c.isValid()) { m_derivLineColor = c; updateColorButton(ui->btnDerivLineColor, c); }
}

// Getters
QString PlottingDialog3::getCurveName() const { return ui->lineName->text(); }
QString PlottingDialog3::getPressLegend() const { return ui->linePressLegend->text(); }
QString PlottingDialog3::getDerivLegend() const { return ui->lineDerivLegend->text(); }
int PlottingDialog3::getTimeColumn() const { return ui->comboTime->currentIndex(); }
int PlottingDialog3::getPressureColumn() const { return ui->comboPress->currentIndex(); }
bool PlottingDialog3::isMeasuredPressure() const { return ui->radioMeasured->isChecked(); }
double PlottingDialog3::getLSpacing() const { return ui->spinL->value(); }
bool PlottingDialog3::isSmoothEnabled() const { return ui->checkSmooth->isChecked(); }
int PlottingDialog3::getSmoothFactor() const { return ui->spinSmooth->value(); }
QString PlottingDialog3::getXLabel() const { return ui->lineXLabel->text(); }
QString PlottingDialog3::getYLabel() const { return ui->lineYLabel->text(); }

QCPScatterStyle::ScatterShape PlottingDialog3::getPressShape() const { return (QCPScatterStyle::ScatterShape)ui->comboPressShape->currentData().toInt(); }
QColor PlottingDialog3::getPressPointColor() const { return m_pressPointColor; }
Qt::PenStyle PlottingDialog3::getPressLineStyle() const { return (Qt::PenStyle)ui->comboPressLine->currentData().toInt(); }
QColor PlottingDialog3::getPressLineColor() const { return m_pressLineColor; }

QCPScatterStyle::ScatterShape PlottingDialog3::getDerivShape() const { return (QCPScatterStyle::ScatterShape)ui->comboDerivShape->currentData().toInt(); }
QColor PlottingDialog3::getDerivPointColor() const { return m_derivPointColor; }
Qt::PenStyle PlottingDialog3::getDerivLineStyle() const { return (Qt::PenStyle)ui->comboDerivLine->currentData().toInt(); }
QColor PlottingDialog3::getDerivLineColor() const { return m_derivLineColor; }

bool PlottingDialog3::isNewWindow() const { return ui->checkNewWindow->isChecked(); }
