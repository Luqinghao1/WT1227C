/*
 * wt_fittingwidget.h
 * 文件作用：试井拟合主界面类的头文件
 * 功能描述：
 * 1. 声明 FittingWidget 类，继承自 QWidget
 * 2. 定义界面控件指针、模型管理器、图表控件等成员变量
 * 3. 声明与参数配置、数据加载、拟合算法相关的信号和槽函数
 * 4. 提供外部调用的接口（如设置数据、加载状态等）
 */

#ifndef WT_FITTINGWIDGET_H
#define WT_FITTINGWIDGET_H

#include <QWidget>
#include <QMap>
#include <QVector>
#include <QFutureWatcher>
#include <QJsonObject>
#include "modelmanager.h"
#include "mousezoom.h"
#include "chartsetting1.h"

// 引入参数管理和数据加载模块头文件
#include "fittingparameterchart.h"
#include "fittingobserveddata.h"
#include "paramselectdialog.h"

namespace Ui { class FittingWidget; }

class FittingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FittingWidget(QWidget *parent = nullptr);
    ~FittingWidget();

    // 设置模型管理器
    void setModelManager(ModelManager* m);

    // 设置观测数据（时间、压力、导数）
    void setObservedData(const QVector<double>& t, const QVector<double>& p, const QVector<double>& d);

    // 基础参数更新接口（供外部调用）
    void updateBasicParameters();

    // 从 JSON 数据加载拟合状态（包含参数、视图范围、观测数据等）
    void loadFittingState(const QJsonObject& data = QJsonObject());

    // 获取当前拟合状态的 JSON 对象（用于保存到项目文件）
    QJsonObject getJsonState() const;

signals:
    // 拟合完成信号
    void fittingCompleted(ModelManager::ModelType modelType, const QMap<QString, double>& parameters);
    // 迭代更新信号（用于刷新曲线和误差显示）
    void sigIterationUpdated(double error, QMap<QString, double> currentParams, QVector<double> t, QVector<double> p, QVector<double> d);
    // 进度信号
    void sigProgress(int progress);
    // 请求保存信号
    void sigRequestSave();

private slots:
    // UI 按钮槽函数
    void on_btnLoadData_clicked();      // 加载数据
    void on_btnRunFit_clicked();        // 开始拟合
    void on_btnStop_clicked();          // 停止拟合
    void on_btnImportModel_clicked();   // 刷新曲线
    void on_btnExportData_clicked();    // 导出参数
    void on_btnExportChart_clicked();   // 导出图表
    void on_btnResetParams_clicked();   // 重置参数
    void on_btnResetView_clicked();     // 复位视图
    void on_btnChartSettings_clicked(); // 图表设置
    void on_btn_modelSelect_clicked();  // 选择模型
    void on_btnSelectParams_clicked();  // 打开参数选择对话框

    void on_btnSaveFit_clicked();       // 保存结果
    void on_btnExportReport_clicked();  // 导出报告

    // 内部逻辑槽函数
    void onIterationUpdate(double err, const QMap<QString,double>& p, const QVector<double>& t, const QVector<double>& p_curve, const QVector<double>& d_curve);
    void onFitFinished();
    void onSliderWeightChanged(int value); // 权重滑块改变

private:
    Ui::FittingWidget *ui;
    ModelManager* m_modelManager;
    MouseZoom* m_plot;
    QCPTextElement* m_plotTitle;
    ModelManager::ModelType m_currentModelType;

    // --- 核心模块实例 ---
    FittingParameterChart* m_paramChart; // 负责参数数据与表格的管理
    FittingObservedData* m_dataLoader;   // 负责数据文件的加载与解析

    // 本地缓存的观测数据
    QVector<double> m_obsTime;
    QVector<double> m_obsPressure;
    QVector<double> m_obsDerivative;

    // 拟合控制标志
    bool m_isFitting;
    bool m_stopRequested;
    QFutureWatcher<void> m_watcher;

    // 初始化绘图控件配置
    void setupPlot();
    // 初始化默认模型状态
    void initializeDefaultModel();
    // 根据当前参数更新理论曲线
    void updateModelCurve();

    // 优化算法相关函数 (Levenberg-Marquardt)
    void runOptimizationTask(ModelManager::ModelType modelType, QList<FitParameter> fitParams, double weight);
    void runLevenbergMarquardtOptimization(ModelManager::ModelType modelType, QList<FitParameter> params, double weight);

    // 计算残差
    QVector<double> calculateResiduals(const QMap<QString, double>& params, ModelManager::ModelType modelType, double weight);
    // 计算雅可比矩阵
    QVector<QVector<double>> computeJacobian(const QMap<QString, double>& params, const QVector<double>& residuals, const QVector<int>& fitIndices, ModelManager::ModelType modelType, const QList<FitParameter>& currentFitParams, double weight);
    // 求解线性方程组 (Eigen)
    QVector<double> solveLinearSystem(const QVector<QVector<double>>& A, const QVector<double>& b);
    // 计算平方误差和
    double calculateSumSquaredError(const QVector<double>& residuals);

    // 获取图表 Base64 字符串用于报告
    QString getPlotImageBase64();
    // 绘制曲线
    void plotCurves(const QVector<double>& t, const QVector<double>& p, const QVector<double>& d, bool isModel);
};

#endif // WT_FITTINGWIDGET_H
