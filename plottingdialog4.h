/*
 * 文件名: plottingdialog4.h
 * 文件作用: 曲线属性编辑/管理对话框头文件
 * 功能描述:
 * 1. 用于修改已存在曲线的属性。
 * 2. 支持修改：曲线内部ID、图例名称、X/Y数据源列。
 * 3. 支持修改样式：数据点形状、颜色；线条样式、颜色。
 * 4. 提供颜色选择按钮（调用调色盘）。
 */

#ifndef PLOTTINGDIALOG4_H
#define PLOTTINGDIALOG4_H

#include <QDialog>
#include <QStandardItemModel>
#include <QColor>
#include "qcustomplot.h"

namespace Ui {
class PlottingDialog4;
}

class PlottingDialog4 : public QDialog
{
    Q_OBJECT

public:
    explicit PlottingDialog4(QStandardItemModel* model, QWidget *parent = nullptr);
    ~PlottingDialog4();

    // 设置初始数据（回显当前属性）
    void setInitialData(const QString& name, const QString& legend, int xCol, int yCol,
                        QCPScatterStyle::ScatterShape shape, QColor pointColor,
                        Qt::PenStyle lineStyle, QColor lineColor);

    // 获取修改后的数据
    QString getCurveName() const;
    QString getLegendName() const;
    int getXColumn() const;
    int getYColumn() const;
    QCPScatterStyle::ScatterShape getPointShape() const;
    QColor getPointColor() const;
    Qt::PenStyle getLineStyle() const;
    QColor getLineColor() const;

private slots:
    void on_btnPointColor_clicked();
    void on_btnLineColor_clicked();

private:
    Ui::PlottingDialog4 *ui;
    QStandardItemModel* m_dataModel;
    QColor m_pointColor;
    QColor m_lineColor;

    void populateComboBoxes(); // 填充下列列表
    void setupStyleOptions();  // 填充样式选项
    void updateColorButton(QPushButton* btn, const QColor& color); // 更新按钮颜色显示
};

#endif // PLOTTINGDIALOG4_H
