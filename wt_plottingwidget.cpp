/*
 * 文件名: wt_plottingwidget.cpp
 * 文件作用: 图表分析主界面实现
 * 功能描述:
 * 1. [核心修改] 实现了数据点的完整序列化与反序列化，支持保存至独立JSON文件。
 * 2. 实现了从文件恢复图表的功能 (loadProjectData)。
 * 3. 保持了原有的绘图、分析、交互逻辑。
 */

#include "wt_plottingwidget.h"
#include "ui_wt_plottingwidget.h"
#include "plottingdialog1.h"
#include "plottingdialog2.h"
#include "plottingdialog3.h"
#include "plottingdialog4.h"
#include "plottingsinglewidget.h"
#include "plottingstackwidget.h"
#include "chartsetting1.h"
#include "chartsetting2.h"
#include "modelparameter.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtMath>

// === JSON 转换辅助 ===
QJsonArray vectorToJson(const QVector<double>& vec) {
    QJsonArray arr;
    for(double v : vec) arr.append(v);
    return arr;
}

QVector<double> jsonToVector(const QJsonArray& arr) {
    QVector<double> vec;
    for(const auto& val : arr) vec.append(val.toDouble());
    return vec;
}

// === 序列化 ===
QJsonObject CurveInfo::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["legendName"] = legendName;
    obj["type"] = type;
    obj["xCol"] = xCol;
    obj["yCol"] = yCol;

    // 保存实际坐标数据
    obj["xData"] = vectorToJson(xData);
    obj["yData"] = vectorToJson(yData);

    obj["pointShape"] = (int)pointShape;
    obj["pointColor"] = pointColor.name();
    obj["lineStyle"] = (int)lineStyle;
    obj["lineColor"] = lineColor.name();

    if (type == 1) { // 压力产量
        obj["x2Col"] = x2Col;
        obj["y2Col"] = y2Col;
        obj["x2Data"] = vectorToJson(x2Data);
        obj["y2Data"] = vectorToJson(y2Data);
        obj["prodLegendName"] = prodLegendName;
        obj["prodGraphType"] = prodGraphType;
        obj["prodColor"] = prodColor.name();
    } else if (type == 2) { // 导数
        obj["isMeasuredP"] = isMeasuredP;
        obj["LSpacing"] = LSpacing;
        obj["isSmooth"] = isSmooth;
        obj["smoothFactor"] = smoothFactor;
        obj["derivData"] = vectorToJson(derivData);
        obj["derivShape"] = (int)derivShape;
        obj["derivPointColor"] = derivPointColor.name();
        obj["derivLineStyle"] = (int)derivLineStyle;
        obj["derivLineColor"] = derivLineColor.name();
        obj["prodLegendName"] = prodLegendName;
    }
    return obj;
}

// === 反序列化 ===
CurveInfo CurveInfo::fromJson(const QJsonObject& json) {
    CurveInfo info;
    info.name = json["name"].toString();
    info.legendName = json["legendName"].toString();
    info.type = json["type"].toInt();
    info.xCol = json["xCol"].toInt(-1);
    info.yCol = json["yCol"].toInt(-1);

    // 恢复坐标数据
    info.xData = jsonToVector(json["xData"].toArray());
    info.yData = jsonToVector(json["yData"].toArray());

    info.pointShape = (QCPScatterStyle::ScatterShape)json["pointShape"].toInt();
    info.pointColor = QColor(json["pointColor"].toString());
    info.lineStyle = (Qt::PenStyle)json["lineStyle"].toInt();
    info.lineColor = QColor(json["lineColor"].toString());

    if (info.type == 1) {
        info.x2Col = json["x2Col"].toInt(-1);
        info.y2Col = json["y2Col"].toInt(-1);
        info.x2Data = jsonToVector(json["x2Data"].toArray());
        info.y2Data = jsonToVector(json["y2Data"].toArray());
        info.prodLegendName = json["prodLegendName"].toString();
        info.prodGraphType = json["prodGraphType"].toInt();
        info.prodColor = QColor(json["prodColor"].toString());
    } else if (info.type == 2) {
        info.isMeasuredP = json["isMeasuredP"].toBool();
        info.LSpacing = json["LSpacing"].toDouble();
        info.isSmooth = json["isSmooth"].toBool();
        info.smoothFactor = json["smoothFactor"].toInt();
        info.derivData = jsonToVector(json["derivData"].toArray());
        info.derivShape = (QCPScatterStyle::ScatterShape)json["derivShape"].toInt();
        info.derivPointColor = QColor(json["derivPointColor"].toString());
        info.derivLineStyle = (Qt::PenStyle)json["derivLineStyle"].toInt();
        info.derivLineColor = QColor(json["derivLineColor"].toString());
        info.prodLegendName = json["prodLegendName"].toString();
    }
    return info;
}

static void applyMessageBoxStyle(QMessageBox& msgBox) {
    msgBox.setStyleSheet(
        "QMessageBox { background-color: white; color: black; }"
        "QPushButton { color: black; background-color: #f0f0f0; border: 1px solid #555; padding: 5px; min-width: 60px; }"
        "QLabel { color: black; }"
        );
}

WT_PlottingWidget::WT_PlottingWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WT_PlottingWidget),
    m_dataModel(nullptr),
    m_isSelectingForExport(false),
    m_selectionStep(0),
    m_exportTargetGraph(nullptr),
    m_currentMode(Mode_Single),
    m_topRect(nullptr),
    m_bottomRect(nullptr),
    m_graphPress(nullptr),
    m_graphProd(nullptr)
{
    ui->setupUi(this);
    setupPlotStyle(Mode_Single);
    connect(ui->customPlot, &QCustomPlot::plottableClick, this, &WT_PlottingWidget::onGraphClicked);
}

WT_PlottingWidget::~WT_PlottingWidget()
{
    qDeleteAll(m_openedWindows);
    delete ui;
}

void WT_PlottingWidget::setDataModel(QStandardItemModel* model) { m_dataModel = model; }
void WT_PlottingWidget::setProjectPath(const QString& path) { m_projectPath = path; }

// [新增] 加载项目数据
void WT_PlottingWidget::loadProjectData()
{
    // 1. 清空当前状态
    m_curves.clear();
    ui->listWidget_Curves->clear();
    ui->customPlot->clearGraphs();
    ui->customPlot->replot();
    m_currentDisplayedCurve.clear();

    // 2. 从 ModelParameter 获取数据
    QJsonArray plots = ModelParameter::instance()->getPlottingData();
    if (plots.isEmpty()) return;

    // 3. 恢复曲线
    for (const auto& val : plots) {
        CurveInfo info = CurveInfo::fromJson(val.toObject());
        m_curves.insert(info.name, info);
        ui->listWidget_Curves->addItem(info.name);
    }

    // 4. 默认显示第一条
    if (ui->listWidget_Curves->count() > 0) {
        on_listWidget_Curves_itemDoubleClicked(ui->listWidget_Curves->item(0));
    }
}

void WT_PlottingWidget::saveProjectData()
{
    if (!ModelParameter::instance()->hasLoadedProject()) {
        QMessageBox::warning(this, "错误", "未加载项目，无法保存。");
        return;
    }

    QJsonArray curvesArray;
    for(auto it = m_curves.begin(); it != m_curves.end(); ++it) {
        curvesArray.append(it.value().toJson());
    }

    ModelParameter::instance()->savePlottingData(curvesArray);
    QMessageBox::information(this, "保存", "绘图数据已保存（包含数据点）。");
}

void WT_PlottingWidget::setupPlotStyle(ChartMode mode)
{
    m_currentMode = mode;
    ui->customPlot->plotLayout()->clear();
    ui->customPlot->clearGraphs();

    ui->customPlot->plotLayout()->insertRow(0);
    QCPTextElement* title = new QCPTextElement(ui->customPlot, "试井分析图表", QFont("Microsoft YaHei", 12, QFont::Bold));
    title->setTextColor(Qt::black);
    ui->customPlot->plotLayout()->addElement(0, 0, title);

    if (mode == Mode_Single) {
        QCPAxisRect* rect = new QCPAxisRect(ui->customPlot);
        ui->customPlot->plotLayout()->addElement(1, 0, rect);
        QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
        rect->axis(QCPAxis::atBottom)->setScaleType(QCPAxis::stLogarithmic);
        rect->axis(QCPAxis::atBottom)->setTicker(logTicker);
        rect->axis(QCPAxis::atLeft)->setScaleType(QCPAxis::stLogarithmic);
        rect->axis(QCPAxis::atLeft)->setTicker(logTicker);
        rect->axis(QCPAxis::atBottom)->setNumberFormat("eb");
        rect->axis(QCPAxis::atBottom)->setNumberPrecision(0);
        rect->axis(QCPAxis::atLeft)->setNumberFormat("eb");
        rect->axis(QCPAxis::atLeft)->setNumberPrecision(0);
        rect->axis(QCPAxis::atBottom)->grid()->setSubGridVisible(true);
        rect->axis(QCPAxis::atLeft)->grid()->setSubGridVisible(true);
        rect->axis(QCPAxis::atBottom)->setLabel("Time");
        rect->axis(QCPAxis::atLeft)->setLabel("Pressure");
    } else if (mode == Mode_Stacked) {
        m_topRect = new QCPAxisRect(ui->customPlot);
        m_bottomRect = new QCPAxisRect(ui->customPlot);
        ui->customPlot->plotLayout()->addElement(1, 0, m_topRect);
        ui->customPlot->plotLayout()->addElement(2, 0, m_bottomRect);
        QCPMarginGroup *group = new QCPMarginGroup(ui->customPlot);
        m_topRect->setMarginGroup(QCP::msLeft | QCP::msRight, group);
        m_bottomRect->setMarginGroup(QCP::msLeft | QCP::msRight, group);
        connect(m_topRect->axis(QCPAxis::atBottom), SIGNAL(rangeChanged(QCPRange)), m_bottomRect->axis(QCPAxis::atBottom), SLOT(setRange(QCPRange)));
        connect(m_bottomRect->axis(QCPAxis::atBottom), SIGNAL(rangeChanged(QCPRange)), m_topRect->axis(QCPAxis::atBottom), SLOT(setRange(QCPRange)));
        m_topRect->axis(QCPAxis::atLeft)->setLabel("Pressure (MPa)");
        m_bottomRect->axis(QCPAxis::atLeft)->setLabel("Production (m3/d)");
        m_bottomRect->axis(QCPAxis::atBottom)->setLabel("Time (h)");
        m_topRect->axis(QCPAxis::atBottom)->setTickLabels(false);
        m_graphPress = ui->customPlot->addGraph(m_topRect->axis(QCPAxis::atBottom), m_topRect->axis(QCPAxis::atLeft));
        m_graphProd = ui->customPlot->addGraph(m_bottomRect->axis(QCPAxis::atBottom), m_bottomRect->axis(QCPAxis::atLeft));
    }
    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui->customPlot->replot();
}

// ---------------- 按钮逻辑 ----------------

void WT_PlottingWidget::on_btn_NewCurve_clicked()
{
    if(!m_dataModel) return;
    PlottingDialog1 dlg(m_dataModel, this);
    if(dlg.exec() == QDialog::Accepted) {
        CurveInfo info;
        info.name = dlg.getCurveName();
        info.legendName = dlg.getLegendName();
        info.xCol = dlg.getXColumn(); info.yCol = dlg.getYColumn();
        info.pointShape = dlg.getPointShape(); info.pointColor = dlg.getPointColor();
        info.lineStyle = dlg.getLineStyle(); info.lineColor = dlg.getLineColor();
        info.type = 0;

        for(int i=0; i<m_dataModel->rowCount(); ++i) {
            info.xData.append(m_dataModel->item(i, info.xCol)->text().toDouble());
            info.yData.append(m_dataModel->item(i, info.yCol)->text().toDouble());
        }

        m_curves.insert(info.name, info);
        ui->listWidget_Curves->addItem(info.name);

        if(dlg.isNewWindow()) {
            PlottingSingleWidget* w = new PlottingSingleWidget();
            w->setProjectPath(m_projectPath);
            w->setWindowTitle(info.name);
            w->addCurve(info.legendName, info.xData, info.yData, info.pointShape, info.pointColor, info.lineStyle, info.lineColor, dlg.getXLabel(), dlg.getYLabel());
            w->show();
            m_openedWindows.append(w);
        } else {
            setupPlotStyle(Mode_Single);
            addCurveToPlot(info);
            m_currentDisplayedCurve = info.name;
        }
    }
}

void WT_PlottingWidget::on_btn_PressureRate_clicked()
{
    if(!m_dataModel) return;
    PlottingDialog2 dlg(m_dataModel, this);
    if(dlg.exec() == QDialog::Accepted) {
        CurveInfo info;
        info.name = dlg.getChartName();
        info.legendName = dlg.getPressLegend();
        info.type = 1;
        info.xCol = dlg.getPressXCol(); info.yCol = dlg.getPressYCol();
        info.x2Col = dlg.getProdXCol(); info.y2Col = dlg.getProdYCol();

        for(int i=0; i<m_dataModel->rowCount(); ++i) {
            info.xData.append(m_dataModel->item(i, info.xCol)->text().toDouble());
            info.yData.append(m_dataModel->item(i, info.yCol)->text().toDouble());
            info.x2Data.append(m_dataModel->item(i, info.x2Col)->text().toDouble());
            info.y2Data.append(m_dataModel->item(i, info.y2Col)->text().toDouble());
        }

        info.pointShape = dlg.getPressShape(); info.pointColor = dlg.getPressPointColor();
        info.lineStyle = dlg.getPressLineStyle(); info.lineColor = dlg.getPressLineColor();
        info.prodLegendName = dlg.getProdLegend();
        info.prodGraphType = dlg.getProdGraphType();
        info.prodColor = dlg.getProdColor();

        m_curves.insert(info.name, info);
        ui->listWidget_Curves->addItem(info.name);

        if(dlg.isNewWindow()) {
            PlottingStackWidget* w = new PlottingStackWidget();
            w->setProjectPath(m_projectPath);
            w->setWindowTitle(info.name);
            w->setData(info.xData, info.yData, info.x2Data, info.y2Data,
                       info.legendName, info.pointShape, info.pointColor, info.lineStyle, info.lineColor,
                       info.prodLegendName, info.prodGraphType, info.prodColor);
            w->show();
            m_openedWindows.append(w);
        } else {
            setupPlotStyle(Mode_Stacked);
            drawStackedPlot(info);
            m_currentDisplayedCurve = info.name;
        }
    }
}

void WT_PlottingWidget::on_btn_Derivative_clicked()
{
    if(!m_dataModel) return;
    PlottingDialog3 dlg(m_dataModel, this);
    if(dlg.exec() == QDialog::Accepted) {
        CurveInfo info;
        info.name = dlg.getCurveName();
        info.legendName = dlg.getPressLegend();
        info.type = 2;

        info.xCol = dlg.getTimeColumn(); info.yCol = dlg.getPressureColumn();
        info.isMeasuredP = dlg.isMeasuredPressure();
        info.LSpacing = dlg.getLSpacing();
        info.isSmooth = dlg.isSmoothEnabled();
        info.smoothFactor = dlg.getSmoothFactor();

        double initialP = 0; bool first = true;
        for(int i=0; i<m_dataModel->rowCount(); ++i) {
            double t = m_dataModel->item(i, info.xCol)->text().toDouble();
            double p = m_dataModel->item(i, info.yCol)->text().toDouble();
            if(first) { initialP = p; first = false; }
            double dp = info.isMeasuredP ? std::abs(p - initialP) : p;
            if(t > 0 && dp > 0) { info.xData.append(t); info.yData.append(dp); }
        }

        if(info.xData.size() < 3) { QMessageBox::warning(this, "错误", "数据点不足"); return; }

        QVector<double> derData;
        for (int i = 0; i < info.xData.size(); ++i) {
            double t = info.xData[i];
            double logT = std::log(t);
            int l=i, r=i;
            while(l>0 && std::log(info.xData[l]) > logT - info.LSpacing) l--;
            while(r<info.xData.size()-1 && std::log(info.xData[r]) < logT + info.LSpacing) r++;
            double num = info.yData[r] - info.yData[l];
            double den = std::log(info.xData[r]) - std::log(info.xData[l]);
            derData.append(std::abs(den)>1e-6 ? num/den : 0);
        }

        if(info.isSmooth && info.smoothFactor > 1) {
            QVector<double> smoothed;
            int half = info.smoothFactor/2;
            for (int i = 0; i < derData.size(); ++i) {
                double sum = 0; int cnt = 0;
                for (int j = i - half; j <= i + half; ++j) {
                    if (j >= 0 && j < derData.size()) { sum += derData[j]; cnt++; }
                }
                smoothed.append(sum / cnt);
            }
            info.derivData = smoothed;
        } else {
            info.derivData = derData;
        }

        info.pointShape = dlg.getPressShape(); info.pointColor = dlg.getPressPointColor();
        info.lineStyle = dlg.getPressLineStyle(); info.lineColor = dlg.getPressLineColor();
        info.derivShape = dlg.getDerivShape(); info.derivPointColor = dlg.getDerivPointColor();
        info.derivLineStyle = dlg.getDerivLineStyle(); info.derivLineColor = dlg.getDerivLineColor();
        info.prodLegendName = dlg.getDerivLegend();

        m_curves.insert(info.name, info);
        ui->listWidget_Curves->addItem(info.name);

        if(dlg.isNewWindow()) {
            PlottingSingleWidget* w = new PlottingSingleWidget();
            w->setProjectPath(m_projectPath);
            w->setWindowTitle(info.name);
            w->addCurve(info.legendName, info.xData, info.yData, info.pointShape, info.pointColor, info.lineStyle, info.lineColor, dlg.getXLabel(), dlg.getYLabel());
            w->addCurve(info.prodLegendName, info.xData, info.derivData, info.derivShape, info.derivPointColor, info.derivLineStyle, info.derivLineColor, dlg.getXLabel(), dlg.getYLabel());
            w->show();
            m_openedWindows.append(w);
        } else {
            setupPlotStyle(Mode_Single);
            drawDerivativePlot(info);
            m_currentDisplayedCurve = info.name;
        }
    }
}

// ---------------- 绘图函数 ----------------

void WT_PlottingWidget::addCurveToPlot(const CurveInfo& info)
{
    QCPGraph* graph = ui->customPlot->addGraph();
    graph->setName(info.legendName);
    graph->setData(info.xData, info.yData);
    graph->setScatterStyle(QCPScatterStyle(info.pointShape, info.pointColor, info.pointColor, 6));
    QPen pen(info.lineColor, 2, info.lineStyle);
    graph->setPen(pen);
    if(info.lineStyle == Qt::NoPen) graph->setLineStyle(QCPGraph::lsNone);
    if(ui->check_ShowLines->isChecked()) graph->setLineStyle(QCPGraph::lsLine);
    ui->customPlot->rescaleAxes();
    ui->customPlot->replot();
}

void WT_PlottingWidget::drawStackedPlot(const CurveInfo& info)
{
    if(!m_graphPress || !m_graphProd) return;

    m_graphPress->setData(info.xData, info.yData);
    m_graphPress->setName(info.legendName);
    m_graphPress->setScatterStyle(QCPScatterStyle(info.pointShape, info.pointColor, info.pointColor, 6));
    QPen pPen(info.lineColor, 2, info.lineStyle);
    m_graphPress->setPen(pPen);
    if(info.lineStyle == Qt::NoPen) m_graphPress->setLineStyle(QCPGraph::lsNone);

    QVector<double> px, py;
    if(info.prodGraphType == 0) { // Step
        double t_cum = 0;
        if(!info.x2Data.isEmpty()) { px.append(0); py.append(info.y2Data[0]); }
        for(int i=0; i<info.x2Data.size(); ++i) {
            t_cum += info.x2Data[i];
            if(i+1 < info.y2Data.size()) { px.append(t_cum); py.append(info.y2Data[i+1]); }
            else { px.append(t_cum); py.append(info.y2Data[i]); }
        }
        m_graphProd->setLineStyle(QCPGraph::lsStepLeft);
        m_graphProd->setScatterStyle(QCPScatterStyle::ssNone);
        m_graphProd->setBrush(QBrush(info.prodColor.lighter(170)));
    } else {
        px = info.x2Data; py = info.y2Data;
        m_graphProd->setLineStyle(info.prodGraphType==1 ? QCPGraph::lsNone : QCPGraph::lsLine);
        m_graphProd->setScatterStyle(info.prodGraphType==1 ? QCPScatterStyle(QCPScatterStyle::ssCircle, 6) : QCPScatterStyle::ssNone);
        m_graphProd->setBrush(Qt::NoBrush);
    }
    m_graphProd->setData(px, py);
    m_graphProd->setName(info.prodLegendName);
    m_graphProd->setPen(QPen(info.prodColor, 2));

    m_graphPress->rescaleAxes();
    m_graphProd->rescaleAxes();
    ui->customPlot->replot();
}

void WT_PlottingWidget::drawDerivativePlot(const CurveInfo& info)
{
    // 在同一幅单坐标系图中绘制
    QCPGraph* g1 = ui->customPlot->addGraph();
    g1->setName(info.legendName);
    g1->setData(info.xData, info.yData);
    g1->setScatterStyle(QCPScatterStyle(info.pointShape, info.pointColor, info.pointColor, 6));
    QPen p1(info.lineColor, 2, info.lineStyle);
    g1->setPen(p1);
    if(info.lineStyle == Qt::NoPen) g1->setLineStyle(QCPGraph::lsNone);

    QCPGraph* g2 = ui->customPlot->addGraph();
    g2->setName(info.prodLegendName);
    g2->setData(info.xData, info.derivData);
    g2->setScatterStyle(QCPScatterStyle(info.derivShape, info.derivPointColor, info.derivPointColor, 6));
    QPen p2(info.derivLineColor, 2, info.derivLineStyle);
    g2->setPen(p2);
    if(info.derivLineStyle == Qt::NoPen) g2->setLineStyle(QCPGraph::lsNone);

    ui->customPlot->rescaleAxes();
    ui->customPlot->replot();
}

void WT_PlottingWidget::on_listWidget_Curves_itemDoubleClicked(QListWidgetItem *item)
{
    QString name = item->text();
    if(!m_curves.contains(name)) return;
    CurveInfo info = m_curves[name];
    m_currentDisplayedCurve = name;

    if (info.type == 1) { setupPlotStyle(Mode_Stacked); drawStackedPlot(info); }
    else if (info.type == 2) { setupPlotStyle(Mode_Single); drawDerivativePlot(info); }
    else { setupPlotStyle(Mode_Single); addCurveToPlot(info); }
}

void WT_PlottingWidget::on_btn_Save_clicked()
{
    saveProjectData();
}

void WT_PlottingWidget::on_btn_ExportData_clicked()
{
    if(m_currentDisplayedCurve.isEmpty()) return;
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("导出");
    msgBox.setText("请选择导出范围：");
    applyMessageBoxStyle(msgBox);
    QPushButton* btnAll = msgBox.addButton("全部数据", QMessageBox::ActionRole);
    QPushButton* btnPart = msgBox.addButton("部分数据", QMessageBox::ActionRole);
    msgBox.addButton("取消", QMessageBox::RejectRole);
    msgBox.exec();

    if(msgBox.clickedButton() == btnAll) executeExport(true);
    else if(msgBox.clickedButton() == btnPart) {
        m_isSelectingForExport = true;
        m_selectionStep = 1;
        ui->customPlot->setCursor(Qt::CrossCursor);
        QMessageBox::information(this, "提示", "请在曲线上点击起始点。");
    }
}

void WT_PlottingWidget::onGraphClicked(QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event)
{
    if(!m_isSelectingForExport) return;
    QCPGraph* graph = qobject_cast<QCPGraph*>(plottable);
    if(!graph) return;

    double key = graph->dataMainKey(dataIndex);

    if(m_selectionStep == 1) {
        m_exportStartIndex = key; m_selectionStep = 2;
        QMessageBox::information(this, "提示", "请点击结束点。");
    } else {
        m_exportEndIndex = key;
        if(m_exportStartIndex > m_exportEndIndex) std::swap(m_exportStartIndex, m_exportEndIndex);
        m_isSelectingForExport = false;
        ui->customPlot->setCursor(Qt::ArrowCursor);
        executeExport(false, m_exportStartIndex, m_exportEndIndex);
    }
}

void WT_PlottingWidget::executeExport(bool fullRange, double start, double end)
{
    QString name = m_projectPath + "/export.csv";
    QString file = QFileDialog::getSaveFileName(this, "保存", name, "CSV Files (*.csv);;Excel Files (*.xls);;Text Files (*.txt)");
    if(file.isEmpty()) return;
    QFile f(file);
    if(!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&f);
    QString sep = ",";
    if(file.endsWith(".txt") || file.endsWith(".xls")) sep = "\t";

    CurveInfo& info = m_curves[m_currentDisplayedCurve];
    if(m_currentMode == Mode_Stacked) {
        out << (fullRange ? "Time,P,Q\n" : "AdjTime,P,Q,OrigTime\n");
        for(int i=0; i<info.xData.size(); ++i) {
            double t = info.xData[i];
            if(!fullRange && (t < start || t > end)) continue;
            double p = info.yData[i];
            double q = getProductionValueAt(t, info);
            if(fullRange) out << t << sep << p << sep << q << "\n";
            else out << (t-start) << sep << p << sep << q << sep << t << "\n";
        }
    } else {
        out << (fullRange ? "Time,Value\n" : "AdjTime,Value,OrigTime\n");
        for(int i=0; i<info.xData.size(); ++i) {
            double t = info.xData[i];
            if(!fullRange && (t < start || t > end)) continue;
            double val = info.yData[i];
            if(fullRange) out << t << sep << val << "\n";
            else out << (t-start) << sep << val << sep << t << "\n";
        }
    }
    f.close();
    QMessageBox::information(this, "成功", "导出完成。");
}

double WT_PlottingWidget::getProductionValueAt(double t, const CurveInfo& info) {
    if(info.y2Data.isEmpty()) return 0;
    return info.y2Data.last();
}

void WT_PlottingWidget::on_btn_ExportImg_clicked() {
    QString fileName = QFileDialog::getSaveFileName(this, "导出图片", "chart.png", "Images (*.png *.jpg *.pdf)");
    if (!fileName.isEmpty()) ui->customPlot->savePng(fileName);
}

void WT_PlottingWidget::on_check_ShowLines_toggled(bool checked) {
    for(int i=0; i<ui->customPlot->graphCount(); ++i)
        ui->customPlot->graph(i)->setLineStyle(checked ? QCPGraph::lsLine : QCPGraph::lsNone);
    ui->customPlot->replot();
}

void WT_PlottingWidget::on_btn_FitToData_clicked() {
    ui->customPlot->rescaleAxes(); ui->customPlot->replot();
}

void WT_PlottingWidget::on_btn_Manage_clicked() {
    QListWidgetItem* item = getCurrentSelectedItem();
    if(!item) return;
    QString name = item->text();
    CurveInfo& info = m_curves[name];
    PlottingDialog4 dlg(m_dataModel, this);
    dlg.setInitialData(info.name, info.legendName, info.xCol, info.yCol,
                       info.pointShape, info.pointColor, info.lineStyle, info.lineColor);
    if(dlg.exec() == QDialog::Accepted) {
        info.legendName = dlg.getLegendName();
        info.xCol = dlg.getXColumn(); info.yCol = dlg.getYColumn();
        info.pointShape = dlg.getPointShape(); info.pointColor = dlg.getPointColor();
        info.lineStyle = dlg.getLineStyle(); info.lineColor = dlg.getLineColor();
        if(info.type == 0) {
            info.xData.clear(); info.yData.clear();
            for(int i=0; i<m_dataModel->rowCount(); ++i) {
                info.xData.append(m_dataModel->item(i, info.xCol)->text().toDouble());
                info.yData.append(m_dataModel->item(i, info.yCol)->text().toDouble());
            }
        }
        if(m_currentDisplayedCurve == name) on_listWidget_Curves_itemDoubleClicked(item);
    }
}

void WT_PlottingWidget::on_btn_Delete_clicked() {
    QListWidgetItem* item = getCurrentSelectedItem();
    if(!item) return;
    QString name = item->text();
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("确认删除");
    msgBox.setText("确定要删除曲线 \"" + name + "\" 吗？");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    applyMessageBoxStyle(msgBox);
    if(msgBox.exec() == QMessageBox::Yes) {
        m_curves.remove(name);
        delete item;
        if(m_currentDisplayedCurve == name) { ui->customPlot->clearGraphs(); ui->customPlot->replot(); m_currentDisplayedCurve.clear(); }
    }
}

void WT_PlottingWidget::on_btn_ChartSettings_clicked() {
    QCPLayoutElement* el = ui->customPlot->plotLayout()->element(0,0);
    QCPTextElement* titleElement = qobject_cast<QCPTextElement*>(el);
    if(m_currentMode == Mode_Stacked) {
        ChartSetting2 dlg(ui->customPlot, m_topRect, m_bottomRect, titleElement, this);
        dlg.exec();
    } else {
        ChartSetting1 dlg(ui->customPlot, titleElement, this);
        dlg.exec();
    }
}

QListWidgetItem* WT_PlottingWidget::getCurrentSelectedItem() {
    return ui->listWidget_Curves->currentItem();
}
