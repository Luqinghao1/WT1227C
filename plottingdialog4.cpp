/*
 * 文件名: plottingdialog4.cpp
 * 文件作用: 曲线属性编辑/管理对话框实现文件
 * 功能描述:
 * 1. 初始化界面，加载数据列和样式选项。
 * 2. 实现 setInitialData 用于回显旧值。
 * 3. 实现颜色选择交互。
 */

#include "plottingdialog4.h"
#include "ui_plottingdialog4.h"
#include <QColorDialog>

PlottingDialog4::PlottingDialog4(QStandardItemModel* model, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PlottingDialog4),
    m_dataModel(model),
    m_pointColor(Qt::red),
    m_lineColor(Qt::red)
{
    ui->setupUi(this);
    populateComboBoxes();
    setupStyleOptions();
}

PlottingDialog4::~PlottingDialog4()
{
    delete ui;
}

void PlottingDialog4::populateComboBoxes()
{
    if (!m_dataModel) return;
    QStringList headers;
    for(int i=0; i<m_dataModel->columnCount(); ++i) {
        QStandardItem* item = m_dataModel->horizontalHeaderItem(i);
        headers << (item ? item->text() : QString("Column %1").arg(i+1));
    }
    ui->comboXCol->addItems(headers);
    ui->comboYCol->addItems(headers);
}

void PlottingDialog4::setupStyleOptions()
{
    // 点形状
    ui->comboShape->addItem("实心圆 (Disc)", (int)QCPScatterStyle::ssDisc);
    ui->comboShape->addItem("空心圆 (Circle)", (int)QCPScatterStyle::ssCircle);
    ui->comboShape->addItem("正方形 (Square)", (int)QCPScatterStyle::ssSquare);
    ui->comboShape->addItem("三角形 (Triangle)", (int)QCPScatterStyle::ssTriangle);
    ui->comboShape->addItem("菱形 (Diamond)", (int)QCPScatterStyle::ssDiamond);
    ui->comboShape->addItem("十字 (Cross)", (int)QCPScatterStyle::ssCross);
    ui->comboShape->addItem("无 (None)", (int)QCPScatterStyle::ssNone);

    // 线型
    ui->comboLineStyle->addItem("实线 (Solid)", (int)Qt::SolidLine);
    ui->comboLineStyle->addItem("虚线 (Dash)", (int)Qt::DashLine);
    ui->comboLineStyle->addItem("点线 (Dot)", (int)Qt::DotLine);
    ui->comboLineStyle->addItem("无 (None)", (int)Qt::NoPen);
}

void PlottingDialog4::setInitialData(const QString& name, const QString& legend, int xCol, int yCol,
                                     QCPScatterStyle::ScatterShape shape, QColor pointColor,
                                     Qt::PenStyle lineStyle, QColor lineColor)
{
    ui->lineName->setText(name);
    ui->lineLegend->setText(legend);

    if (xCol < ui->comboXCol->count()) ui->comboXCol->setCurrentIndex(xCol);
    if (yCol < ui->comboYCol->count()) ui->comboYCol->setCurrentIndex(yCol);

    // 设置样式回显
    int shapeIdx = ui->comboShape->findData((int)shape);
    if (shapeIdx != -1) ui->comboShape->setCurrentIndex(shapeIdx);

    int lineStyleIdx = ui->comboLineStyle->findData((int)lineStyle);
    if (lineStyleIdx != -1) ui->comboLineStyle->setCurrentIndex(lineStyleIdx);

    m_pointColor = pointColor;
    m_lineColor = lineColor;
    updateColorButton(ui->btnPointColor, m_pointColor);
    updateColorButton(ui->btnLineColor, m_lineColor);
}

void PlottingDialog4::on_btnPointColor_clicked()
{
    QColor c = QColorDialog::getColor(m_pointColor, this, "选择点颜色");
    if(c.isValid()) {
        m_pointColor = c;
        updateColorButton(ui->btnPointColor, c);
    }
}

void PlottingDialog4::on_btnLineColor_clicked()
{
    QColor c = QColorDialog::getColor(m_lineColor, this, "选择线颜色");
    if(c.isValid()) {
        m_lineColor = c;
        updateColorButton(ui->btnLineColor, c);
    }
}

void PlottingDialog4::updateColorButton(QPushButton* btn, const QColor& color)
{
    btn->setStyleSheet(QString("background-color: %1; border: 1px solid #555; border-radius: 3px;").arg(color.name()));
}

// Getters
QString PlottingDialog4::getCurveName() const { return ui->lineName->text(); }
QString PlottingDialog4::getLegendName() const { return ui->lineLegend->text(); }
int PlottingDialog4::getXColumn() const { return ui->comboXCol->currentIndex(); }
int PlottingDialog4::getYColumn() const { return ui->comboYCol->currentIndex(); }
QCPScatterStyle::ScatterShape PlottingDialog4::getPointShape() const {
    return (QCPScatterStyle::ScatterShape)ui->comboShape->currentData().toInt();
}
QColor PlottingDialog4::getPointColor() const { return m_pointColor; }
Qt::PenStyle PlottingDialog4::getLineStyle() const {
    return (Qt::PenStyle)ui->comboLineStyle->currentData().toInt();
}
QColor PlottingDialog4::getLineColor() const { return m_lineColor; }
