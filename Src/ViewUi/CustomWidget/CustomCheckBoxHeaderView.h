#ifndef CUSTOMCHECKBOXHEADERVIEW_H
#define CUSTOMCHECKBOXHEADERVIEW_H

#include <QHeaderView>
#include <QStyleOptionButton>
#include <QMouseEvent>
#include <QSet>
#include <QMap>

/*************************示例使用方法*************************

// 设置第2列宽度范围为100-300像素
headerView->setSectionWidthRange(2, 100, 300);

// 清除第2列宽度限制
headerView->clearSectionWidthRange(2);

// 清除所有列宽限制
headerView->clearAllWidthRanges();
***************************************************************/


class CustomCheckBoxHeaderView : public QHeaderView
{
    Q_OBJECT

public:
    explicit CustomCheckBoxHeaderView(Qt::Orientation orientation, QWidget* parent = nullptr);

    // 设置/获取列头复选框状态
    void setCheckState(int section, Qt::CheckState state);
    Qt::CheckState checkState(int section) const;

    // 管理需要复选框的列
    void addCheckableSection(int section);
    void removeCheckableSection(int section);
    void setCheckableSections(const QList<int>& sections);
    bool isSectionCheckable(int section) const;
    QList<int> checkableSections() const;

    // 设置列宽范围
    void setSectionWidthRange(int section, int minWidth, int maxWidth); // 设置指定列的宽度范围（最小、最大）
    void clearSectionWidthRange(int section);       // 清除指定列限制
    void clearAllWidthRanges();                     // 清除所有

signals:
    void checkStateChanged(int section, bool checked);

protected:
    void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* ev) override; // 添加事件过滤器
private:
    QMap<int, Qt::CheckState> m_checkStates;
    QSet<int> m_checkableSections;  // 需要复选框的列
    int m_hoverSection = -1;         // 鼠标悬停的列

    // 存储每列的宽度限制
    QMap<int, QPair<int, int>> m_sectionWidthRanges;  // section -> (min, max)

    void applyWidthConstraints(); // 应用宽度约束

    // 计算复选框位置
    QRect checkboxRect(const QRect& sectionRect) const;
};

#endif // CUSTOMCHECKBOXHEADERVIEW_H