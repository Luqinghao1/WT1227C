/*
 * 文件名: plottingdialog3.h
 * 文件作用: 压力导数曲线配置对话框头文件
 * 功能描述:
 * 1. 包含数据源选择（支持压差计算）、计算参数（L-Spacing, 平滑）。
 * 2. 独立的压力曲线和导数曲线样式设置（调色盘按钮）。
 * 3. 坐标轴标签设置。
 */

#ifndef PLOTTINGDIALOG3_H
#define PLOTTINGDIALOG3_H

#include <QDialog>
#include <QStandardItemModel>
#include <QColor>
#include "qcustomplot.h"

namespace Ui {
class PlottingDialog3;
}

class PlottingDialog3 : public QDialog
{
    Q_OBJECT

public:
    explicit PlottingDialog3(QStandardItemModel* model, QWidget *parent = nullptr);
    ~PlottingDialog3();

    // --- 基础信息 ---
    QString getCurveName() const;
    QString getPressLegend() const;
    QString getDerivLegend() const;

    // --- 数据源 ---
    int getTimeColumn() const;
    int getPressureColumn() const;
    bool isMeasuredPressure() const; // true=实测压力, false=压差

    // --- 计算参数 ---
    double getLSpacing() const;
    bool isSmoothEnabled() const;
    int getSmoothFactor() const;

    // --- 坐标轴 ---
    QString getXLabel() const;
    QString getYLabel() const;

    // --- 压力样式 ---
    QCPScatterStyle::ScatterShape getPressShape() const;
    QColor getPressPointColor() const;
    Qt::PenStyle getPressLineStyle() const;
    QColor getPressLineColor() const;

    // --- 导数样式 ---
    QCPScatterStyle::ScatterShape getDerivShape() const;
    QColor getDerivPointColor() const;
    Qt::PenStyle getDerivLineStyle() const;
    QColor getDerivLineColor() const;

    bool isNewWindow() const;

private slots:
    void onSmoothToggled(bool checked);
    // 颜色按钮槽
    void selectPressPointColor();
    void selectPressLineColor();
    void selectDerivPointColor();
    void selectDerivLineColor();

private:
    Ui::PlottingDialog3 *ui;
    QStandardItemModel* m_dataModel;
    static int s_counter;

    // 颜色存储
    QColor m_pressPointColor;
    QColor m_pressLineColor;
    QColor m_derivPointColor;
    QColor m_derivLineColor;

    void populateComboBoxes();
    void setupStyleOptions();
    void updateColorButton(QPushButton* btn, const QColor& color);
};

#endif // PLOTTINGDIALOG3_H
