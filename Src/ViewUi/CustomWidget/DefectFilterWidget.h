#ifndef DEFECTFILTERWIDGET_H
#define DEFECTFILTERWIDGET_H

#include <QWidget>
#include <QCheckBox>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QResizeEvent>
#include "Global.h" // 包含DefectType定义


/*****************示例**************
//在主窗口头文件中
#include "DefectFilterWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // ... 其他成员

private:
    DefectFilterWidget *m_defectFilterWidget;
};

// 在主窗口实现文件中
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 创建缺陷过滤器部件
    m_defectFilterWidget = new DefectFilterWidget(this);

    // 可以将其放置在任何布局中
    QDockWidget *dockWidget = new QDockWidget(tr("Defect Filter"), this);
    dockWidget->setWidget(m_defectFilterWidget);
    addDockWidget(Qt::RightDockWidgetArea, dockWidget);

    // ... 其他初始化代码
}

// 获取选中的缺陷类型
void MainWindow::someMethod()
{
    QList<DefectType> selectedTypes = m_defectFilterWidget->getSelectedDefectTypes();
    // 使用选中的缺陷类型进行过滤或其他操作
}
**************************/

class DefectFilterWidget : public QWidget
{
    Q_OBJECT

public:

    explicit DefectFilterWidget(QWidget* parent = nullptr);
    ~DefectFilterWidget();

    // 获取选中的缺陷类型
    QList<DefectType> getSelectedDefectTypes() const;

    // 设置缺陷类型checkBoxk控件的显示状态
    void setDisplayStatus(DefectType defectType, bool isDisplay);

    // 设置指定索引的复选框状态
    void setCheckBoxState(int index, bool checked);

    // 设置所有复选框的状态
    void setAllCheckBoxesState(bool checked);

    // 连接复选框状态变化信号到父对象槽函数
    void connectCheckBoxChangesTo(QObject* receiver, const char* slot);

signals:
    // 复选框状态变化信号
    void checkBoxStateChanged(int index, bool checked);

private slots:
    // 内部槽函数，处理单个复选框状态变化
    void onCheckBoxStateChanged(int state);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void setupUI();
    void createCheckBoxes();
    void rearrangeCheckBoxes();
    QIcon getDefectIcon(DefectType type);

    QVBoxLayout* m_mainLayout;
    QVector<QCheckBox*> m_checkBoxes;
    QVector<QHBoxLayout*> m_rowLayouts;

    QTimer m_resizeTimer;

    // 存储父对象连接信息
    QObject* m_receiver;
    const char* m_slot;
};

#endif // DEFECTFILTERWIDGET_H