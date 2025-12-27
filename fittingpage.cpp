#include "fittingpage.h"
#include "ui_fittingpage.h" // 【关键】必须包含这个由 uic 自动生成的头文件
#include "wt_fittingwidget.h" // 【关键】引用改名后的拟合控件头文件
#include "modelparameter.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QJsonArray>
#include <QDebug>

FittingPage::FittingPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FittingPage), // 如果 ui_fittingpage.h 生成成功，这里就不会报错
    m_modelManager(nullptr)
{
    ui->setupUi(this);
}

FittingPage::~FittingPage()
{
    delete ui;
}

void FittingPage::setModelManager(ModelManager *m)
{
    m_modelManager = m;
    for(int i = 0; i < ui->tabWidget->count(); ++i) {
        // 使用 FittingWidget 类（定义在 wt_fittingwidget.h 中）
        FittingWidget* w = qobject_cast<FittingWidget*>(ui->tabWidget->widget(i));
        if(w) w->setModelManager(m);
    }
}

void FittingPage::setObservedDataToCurrent(const QVector<double> &t, const QVector<double> &p, const QVector<double> &d)
{
    FittingWidget* current = qobject_cast<FittingWidget*>(ui->tabWidget->currentWidget());
    if (current) {
        current->setObservedData(t, p, d);
    } else {
        // 如果当前没有页签，先创建一个
        on_btnNewAnalysis_clicked();
        current = qobject_cast<FittingWidget*>(ui->tabWidget->currentWidget());
        if(current) current->setObservedData(t, p, d);
    }
}

void FittingPage::updateBasicParameters()
{
    for(int i = 0; i < ui->tabWidget->count(); ++i) {
        FittingWidget* w = qobject_cast<FittingWidget*>(ui->tabWidget->widget(i));
        if(w) w->updateBasicParameters();
    }
}

FittingWidget* FittingPage::createNewTab(const QString &name, const QJsonObject &initData)
{
    // 创建 FittingWidget 实例
    FittingWidget* w = new FittingWidget(this);
    if(m_modelManager) w->setModelManager(m_modelManager);

    connect(w, &FittingWidget::sigRequestSave, this, &FittingPage::onChildRequestSave);

    int index = ui->tabWidget->addTab(w, name);
    ui->tabWidget->setCurrentIndex(index);

    if(!initData.isEmpty()) {
        w->loadFittingState(initData);
    }

    return w;
}

QString FittingPage::generateUniqueName(const QString &baseName)
{
    QString name = baseName;
    int counter = 1;
    bool exists = true;
    while(exists) {
        exists = false;
        for(int i=0; i<ui->tabWidget->count(); ++i) {
            if(ui->tabWidget->tabText(i) == name) {
                exists = true;
                break;
            }
        }
        if(exists) {
            counter++;
            name = QString("%1 %2").arg(baseName).arg(counter);
        }
    }
    return name;
}

void FittingPage::on_btnNewAnalysis_clicked()
{
    QStringList items;
    items << "空白分析 (Blank)";
    for(int i=0; i<ui->tabWidget->count(); ++i) {
        items << "复制: " + ui->tabWidget->tabText(i);
    }

    bool ok;
    QString item = QInputDialog::getItem(this, "新建分析", "请选择创建方式:", items, 0, false, &ok);
    if (!ok || item.isEmpty()) return;

    QString newName = generateUniqueName("Analysis");

    if (item == "空白分析 (Blank)") {
        createNewTab(newName);
    } else {
        int indexToCopy = items.indexOf(item) - 1;
        FittingWidget* source = qobject_cast<FittingWidget*>(ui->tabWidget->widget(indexToCopy));
        if(source) {
            QJsonObject state = source->getJsonState();
            createNewTab(newName, state);
        }
    }
}

void FittingPage::on_btnRenameAnalysis_clicked()
{
    int idx = ui->tabWidget->currentIndex();
    if(idx < 0) return;

    QString oldName = ui->tabWidget->tabText(idx);
    bool ok;
    QString newName = QInputDialog::getText(this, "重命名", "请输入新的分析名称:", QLineEdit::Normal, oldName, &ok);
    if(ok && !newName.isEmpty()) {
        ui->tabWidget->setTabText(idx, newName);
    }
}

void FittingPage::on_btnDeleteAnalysis_clicked()
{
    int idx = ui->tabWidget->currentIndex();
    if(idx < 0) return;

    if(ui->tabWidget->count() == 1) {
        QMessageBox::warning(this, "警告", "至少需要保留一个分析页面！");
        return;
    }

    if(QMessageBox::question(this, "确认", "确定要删除当前分析页吗？\n此操作不可恢复。") == QMessageBox::Yes) {
        QWidget* w = ui->tabWidget->widget(idx);
        ui->tabWidget->removeTab(idx);
        delete w;
    }
}

void FittingPage::saveAllFittingStates()
{
    QJsonArray analysesArray;
    for(int i=0; i<ui->tabWidget->count(); ++i) {
        FittingWidget* w = qobject_cast<FittingWidget*>(ui->tabWidget->widget(i));
        if(w) {
            QJsonObject pageObj = w->getJsonState();
            pageObj["_tabName"] = ui->tabWidget->tabText(i);
            analysesArray.append(pageObj);
        }
    }

    QJsonObject root;
    root["version"] = "2.0";
    root["analyses"] = analysesArray;

    ModelParameter::instance()->saveFittingResult(root);
}

void FittingPage::loadAllFittingStates()
{
    QJsonObject root = ModelParameter::instance()->getFittingResult();
    if(root.isEmpty()) {
        if(ui->tabWidget->count() == 0) createNewTab("Analysis 1");
        return;
    }

    ui->tabWidget->clear();

    if(root.contains("analyses") && root["analyses"].isArray()) {
        QJsonArray arr = root["analyses"].toArray();
        for(int i=0; i<arr.size(); ++i) {
            QJsonObject pageObj = arr[i].toObject();
            QString name = pageObj.contains("_tabName") ? pageObj["_tabName"].toString() : QString("Analysis %1").arg(i+1);
            createNewTab(name, pageObj);
        }
    } else {
        createNewTab("Analysis 1", root);
    }

    if(ui->tabWidget->count() == 0) createNewTab("Analysis 1");
}

void FittingPage::onChildRequestSave()
{
    saveAllFittingStates();
    QMessageBox::information(this, "保存成功", "所有分析页的状态已保存到项目文件 (pwt) 中。");
}
