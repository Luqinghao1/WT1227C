#ifndef FITTINGPAGE_H
#define FITTINGPAGE_H

#include <QWidget>
#include <QJsonObject>
#include <QTabWidget> // 显式包含，防止报错
#include "modelmanager.h"

// 前置声明
class FittingWidget;

namespace Ui {
class FittingPage;
}

class FittingPage : public QWidget
{
    Q_OBJECT

public:
    explicit FittingPage(QWidget *parent = nullptr);
    ~FittingPage();

    // 设置模型管理器（传递给子页面）
    void setModelManager(ModelManager* m);

    // 接收来自 MainWindow 的数据，传递给当前激活的 FittingWidget
    void setObservedDataToCurrent(const QVector<double>& t, const QVector<double>& p, const QVector<double>& d);

    // 初始化/重置基本参数
    void updateBasicParameters();

    // 从项目文件加载所有拟合分析的状态
    void loadAllFittingStates();

    // 保存所有拟合分析的状态到项目文件
    void saveAllFittingStates();

private slots:
    void on_btnNewAnalysis_clicked();
    void on_btnRenameAnalysis_clicked();
    void on_btnDeleteAnalysis_clicked();
    void onChildRequestSave();

private:
    Ui::FittingPage *ui;
    ModelManager* m_modelManager;

    // 内部函数：创建新页签
    FittingWidget* createNewTab(const QString& name, const QJsonObject& initData = QJsonObject());
    QString generateUniqueName(const QString& baseName);
};

#endif // FITTINGPAGE_H
