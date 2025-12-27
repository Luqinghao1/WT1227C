#ifndef WT_PLOTTINGWIDGET_H
#define WT_PLOTTINGWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QMap>
#include <QListWidgetItem>
#include <QJsonObject>
#include "mousezoom.h"
#include "plottingstackwidget.h"

namespace Ui {
class WT_PlottingWidget;
}

// 曲线信息结构体
struct CurveInfo {
    QString name;
    QString legendName;
    int xCol, yCol;

    // [重要] 实际数据点缓存，保存到文件
    QVector<double> xData, yData;

    QCPScatterStyle::ScatterShape pointShape;
    QColor pointColor;
    Qt::PenStyle lineStyle;
    QColor lineColor;

    int type; // 0=普通, 1=压力产量, 2=导数

    // 扩展数据
    QString prodLegendName;
    int x2Col, y2Col;
    QVector<double> x2Data, y2Data; // 缓存
    int prodGraphType;
    QColor prodColor;

    bool isMeasuredP;
    double LSpacing;
    bool isSmooth;
    int smoothFactor;

    QVector<double> derivData; // 缓存
    QCPScatterStyle::ScatterShape derivShape;
    QColor derivPointColor;
    Qt::PenStyle derivLineStyle;
    QColor derivLineColor;

    CurveInfo() : xCol(-1), yCol(-1), x2Col(-1), y2Col(-1),
        pointShape(QCPScatterStyle::ssDisc), type(0), prodGraphType(0),
        isMeasuredP(true), LSpacing(0.1), isSmooth(false), smoothFactor(3) {}

    QJsonObject toJson() const;
    static CurveInfo fromJson(const QJsonObject& json);
};

class WT_PlottingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WT_PlottingWidget(QWidget *parent = nullptr);
    ~WT_PlottingWidget();

    void setDataModel(QStandardItemModel* model);
    void setProjectPath(const QString& path);

    // [新增] 加载并恢复图表数据
    // 应在 MainWindow 打开项目后调用此函数
    void loadProjectData();

private slots:
    void on_btn_NewCurve_clicked();
    void on_btn_PressureRate_clicked();
    void on_btn_Derivative_clicked();
    void on_btn_Manage_clicked();
    void on_btn_Delete_clicked();
    void on_btn_Save_clicked();

    void on_listWidget_Curves_itemDoubleClicked(QListWidgetItem *item);
    void on_check_ShowLines_toggled(bool checked);
    void on_btn_ExportData_clicked();
    void on_btn_ChartSettings_clicked();
    void on_btn_ExportImg_clicked();
    void on_btn_FitToData_clicked();
    void onGraphClicked(QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event);

private:
    Ui::WT_PlottingWidget *ui;
    QStandardItemModel* m_dataModel;
    QString m_projectPath;

    QMap<QString, CurveInfo> m_curves;
    QString m_currentDisplayedCurve;
    QList<QWidget*> m_openedWindows;

    bool m_isSelectingForExport;
    int m_selectionStep;
    double m_exportStartIndex;
    double m_exportEndIndex;
    QCPGraph* m_exportTargetGraph;

    enum ChartMode { Mode_Single, Mode_Stacked };
    ChartMode m_currentMode;

    QCPAxisRect* m_topRect;
    QCPAxisRect* m_bottomRect;
    QCPGraph* m_graphPress;
    QCPGraph* m_graphProd;

    void setupPlotStyle(ChartMode mode);
    void addCurveToPlot(const CurveInfo& info);
    void drawStackedPlot(const CurveInfo& info);
    void drawDerivativePlot(const CurveInfo& info);

    QListWidgetItem* getCurrentSelectedItem();
    void executeExport(bool fullRange, double startKey = 0, double endKey = 0);
    double getProductionValueAt(double t, const CurveInfo& info);

    void saveProjectData();
};

#endif // WT_PLOTTINGWIDGET_H
