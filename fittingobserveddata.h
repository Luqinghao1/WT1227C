#ifndef FITTINGOBSERVEDDATA_H
#define FITTINGOBSERVEDDATA_H

#include <QDialog>
#include <QObject>
#include <QVector>
#include <QTableWidget>
#include <QComboBox>

// ===========================================================================
// 数据加载对话框 (从 fittingwidget.h 移动至此)
// ===========================================================================
class FittingDataLoadDialog : public QDialog {
    Q_OBJECT
public:
    explicit FittingDataLoadDialog(const QList<QStringList>& previewData, QWidget *parent = nullptr);
    int getTimeColumnIndex() const;
    int getPressureColumnIndex() const;
    int getDerivativeColumnIndex() const;
    int getSkipRows() const;
    int getPressureDataType() const;
private:
    QTableWidget* m_previewTable;
    QComboBox *m_comboTime, *m_comboPressure, *m_comboDeriv, *m_comboSkipRows, *m_comboPressureType;
    void validateSelection();
};

/**
 * @brief FittingObservedData 类
 * 负责处理观测数据的加载、解析、计算导数等功能。
 */
class FittingObservedData : public QObject
{
    Q_OBJECT
public:
    explicit FittingObservedData(QObject *parent = nullptr);

    // 打开文件对话框加载数据，成功返回 true
    bool loadDataFromFile(QWidget* parentWidget);

    // 获取加载后的数据
    QVector<double> getTime() const;
    QVector<double> getPressure() const;
    QVector<double> getDerivative() const;

private:
    QVector<double> m_obsTime;
    QVector<double> m_obsPressure;
    QVector<double> m_obsDerivative;

    // 辅助函数：解析文本行
    QStringList parseLine(const QString& line);
};

#endif // FITTINGOBSERVEDDATA_H
