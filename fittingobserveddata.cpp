#include "fittingobserveddata.h"
#include "pressurederivativecalculator.h"

#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QRegularExpression>
#include <cmath>

// ===========================================================================
// FittingDataLoadDialog 实现
// ===========================================================================
FittingDataLoadDialog::FittingDataLoadDialog(const QList<QStringList>& previewData, QWidget *parent) : QDialog(parent) {
    setWindowTitle("数据列映射配置"); resize(800, 550);

    this->setStyleSheet(
        "QDialog { background-color: #ffffff; color: #000000; font-family: 'Microsoft YaHei'; }"
        "QLabel, QComboBox, QTableWidget, QGroupBox { color: #000000; }"
        "QTableWidget { gridline-color: #d0d0d0; border: 1px solid #c0c0c0; }"
        "QHeaderView::section { background-color: #f0f0f0; border: 1px solid #d0d0d0; color: #000000; }"
        "QPushButton { background-color: #ffffff; border: 1px solid #c0c0c0; border-radius: 4px; padding: 5px 15px; color: #333333; }"
        "QPushButton:hover { background-color: #f2f2f2; border-color: #a0a0a0; color: #000000; }"
        "QPushButton:pressed { background-color: #e0e0e0; }"
        );

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("请指定数据列含义 (时间必选):", this));

    m_previewTable = new QTableWidget(this);
    if(!previewData.isEmpty()) {
        int rows = qMin(previewData.size(), 50); int cols = previewData[0].size();
        m_previewTable->setRowCount(rows); m_previewTable->setColumnCount(cols);
        QStringList headers; for(int i=0;i<cols;++i) headers<<QString("Col %1").arg(i+1);
        m_previewTable->setHorizontalHeaderLabels(headers);
        for(int i=0;i<rows;++i) for(int j=0;j<cols && j<previewData[i].size();++j)
                m_previewTable->setItem(i,j,new QTableWidgetItem(previewData[i][j]));
    }
    m_previewTable->setAlternatingRowColors(true); layout->addWidget(m_previewTable);

    QGroupBox* grp = new QGroupBox("列映射与设置", this);
    QGridLayout* grid = new QGridLayout(grp);
    QStringList opts; for(int i=0;i<m_previewTable->columnCount();++i) opts<<QString("Col %1").arg(i+1);

    grid->addWidget(new QLabel("时间列 *:",this), 0, 0); m_comboTime = new QComboBox(this); m_comboTime->addItems(opts); grid->addWidget(m_comboTime, 0, 1);
    grid->addWidget(new QLabel("压力列:",this), 0, 2); m_comboPressure = new QComboBox(this); m_comboPressure->addItem("不导入",-1); m_comboPressure->addItems(opts); if(opts.size()>1) m_comboPressure->setCurrentIndex(2); grid->addWidget(m_comboPressure, 0, 3);
    grid->addWidget(new QLabel("导数列:",this), 1, 0); m_comboDeriv = new QComboBox(this); m_comboDeriv->addItem("自动计算 (Bourdet)",-1); m_comboDeriv->addItems(opts); grid->addWidget(m_comboDeriv, 1, 1);
    grid->addWidget(new QLabel("跳过首行数:",this), 1, 2); m_comboSkipRows = new QComboBox(this); for(int i=0;i<=20;++i) m_comboSkipRows->addItem(QString::number(i),i); m_comboSkipRows->setCurrentIndex(1); grid->addWidget(m_comboSkipRows, 1, 3);
    grid->addWidget(new QLabel("压力数据类型:",this), 2, 0); m_comboPressureType = new QComboBox(this); m_comboPressureType->addItem("原始压力 (自动计算压差 |P-Pi|)", 0); m_comboPressureType->addItem("压差数据 (直接使用 ΔP)", 1); grid->addWidget(m_comboPressureType, 2, 1, 1, 3);

    layout->addWidget(grp);
    QHBoxLayout* btns = new QHBoxLayout; QPushButton* ok = new QPushButton("确定",this); QPushButton* cancel = new QPushButton("取消",this);
    connect(ok, &QPushButton::clicked, this, &FittingDataLoadDialog::validateSelection); connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    btns->addStretch(); btns->addWidget(ok); btns->addWidget(cancel); layout->addLayout(btns);
}
void FittingDataLoadDialog::validateSelection() { if(m_comboTime->currentIndex()<0) return; accept(); }
int FittingDataLoadDialog::getTimeColumnIndex() const { return m_comboTime->currentIndex(); }
int FittingDataLoadDialog::getPressureColumnIndex() const { return m_comboPressure->currentIndex()-1; }
int FittingDataLoadDialog::getDerivativeColumnIndex() const { return m_comboDeriv->currentIndex()-1; }
int FittingDataLoadDialog::getSkipRows() const { return m_comboSkipRows->currentData().toInt(); }
int FittingDataLoadDialog::getPressureDataType() const { return m_comboPressureType->currentData().toInt(); }

// ===========================================================================
// FittingObservedData 实现
// ===========================================================================

FittingObservedData::FittingObservedData(QObject *parent) : QObject(parent)
{
}

QStringList FittingObservedData::parseLine(const QString& line) {
    return line.split(QRegularExpression("[,\\s\\t]+"), Qt::SkipEmptyParts);
}

bool FittingObservedData::loadDataFromFile(QWidget* parentWidget)
{
    QString path = QFileDialog::getOpenFileName(parentWidget, "加载试井数据", "", "文本文件 (*.txt *.csv)");
    if(path.isEmpty()) return false;

    QFile f(path);
    if(!f.open(QIODevice::ReadOnly)) return false;

    QTextStream in(&f);
    QList<QStringList> data;
    while(!in.atEnd()) {
        QString l=in.readLine().trimmed();
        if(!l.isEmpty()) data<<parseLine(l);
    }
    f.close();

    // 弹出列映射对话框
    FittingDataLoadDialog dlg(data, parentWidget);
    if(dlg.exec()!=QDialog::Accepted) return false;

    int tCol=dlg.getTimeColumnIndex();
    int pCol=dlg.getPressureColumnIndex();
    int dCol=dlg.getDerivativeColumnIndex();
    int pressureType = dlg.getPressureDataType();

    m_obsTime.clear();
    m_obsPressure.clear();
    m_obsDerivative.clear();

    double p_init = 0;
    // 如果是原始压力模式且指定了压力列，尝试获取初始压力（假设在跳过行后的第一行）
    if(pressureType == 0 && pCol>=0) {
        for(int i=dlg.getSkipRows(); i<data.size(); ++i) {
            if(pCol<data[i].size()) { p_init = data[i][pCol].toDouble(); break; }
        }
    }

    // 解析数据
    for(int i=dlg.getSkipRows(); i<data.size(); ++i) {
        if(tCol<data[i].size()) {
            double tv = data[i][tCol].toDouble();
            double pv = 0;
            if (pCol>=0 && pCol<data[i].size()) {
                double val = data[i][pCol].toDouble();
                // 如果是原始压力，减去初始压力取绝对值；如果是压差，直接使用
                pv = (pressureType == 0) ? std::abs(val - p_init) : val;
            }
            // 过滤无效时间点
            if(tv>0) {
                m_obsTime << tv;
                m_obsPressure << pv;
            }
        }
    }

    // 处理导数：如果文件中指定了导数列，直接读取；否则计算 Bourdet 导数
    if (dCol >= 0) {
        for(int i=dlg.getSkipRows(); i<data.size(); ++i) {
            if(tCol<data[i].size() && data[i][tCol].toDouble() > 0 && dCol<data[i].size()) {
                m_obsDerivative << data[i][dCol].toDouble();
            }
        }
    } else {
        m_obsDerivative = PressureDerivativeCalculator::calculateBourdetDerivative(m_obsTime, m_obsPressure, 0.15);
    }

    return true;
}

QVector<double> FittingObservedData::getTime() const { return m_obsTime; }
QVector<double> FittingObservedData::getPressure() const { return m_obsPressure; }
QVector<double> FittingObservedData::getDerivative() const { return m_obsDerivative; }
