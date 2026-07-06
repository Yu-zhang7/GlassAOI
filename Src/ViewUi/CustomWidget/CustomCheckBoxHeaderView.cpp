#include "CustomCheckBoxHeaderView.h"
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QAbstractScrollArea>

CustomCheckBoxHeaderView::CustomCheckBoxHeaderView(Qt::Orientation orientation, QWidget* parent)
    : QHeaderView(orientation, parent)
{
    setSectionsClickable(true);
    setHighlightSections(true);
    setMouseTracking(true); // 启用鼠标悬停检测

    // 安装事件过滤器以捕获列宽调整
    if (QAbstractScrollArea* scrollArea = qobject_cast<QAbstractScrollArea*>(parent)) {
        if (QWidget* viewport = scrollArea->viewport()) {
            viewport->installEventFilter(this);
        }
    }
}

void CustomCheckBoxHeaderView::setCheckState(int section, Qt::CheckState state)
{
    if (m_checkStates.value(section, Qt::Unchecked) != state) {
        m_checkStates[section] = state;
        updateSection(section);
    }
}

Qt::CheckState CustomCheckBoxHeaderView::checkState(int section) const
{
    return m_checkStates.value(section, Qt::Unchecked);
}

void CustomCheckBoxHeaderView::addCheckableSection(int section)
{
    if (!m_checkableSections.contains(section)) {
        m_checkableSections.insert(section);
        updateSection(section);
    }
}

void CustomCheckBoxHeaderView::removeCheckableSection(int section)
{
    if (m_checkableSections.contains(section)) {
        m_checkableSections.remove(section);
        updateSection(section);
    }
}

void CustomCheckBoxHeaderView::setCheckableSections(const QList<int>& sections)
{
    m_checkableSections = QSet<int>(sections.begin(), sections.end());
    update();
}

bool CustomCheckBoxHeaderView::isSectionCheckable(int section) const
{
    return m_checkableSections.contains(section);
}

QList<int> CustomCheckBoxHeaderView::checkableSections() const
{
    return m_checkableSections.values();
}

QRect CustomCheckBoxHeaderView::checkboxRect(const QRect& sectionRect) const
{
    const int size = 16; // 固定大小
    const int margin = (sectionRect.height() - size) / 2; // 垂直居中

    // 水平位置：左侧留出空间
    const int x = sectionRect.left() + 8; // 左侧留白8像素

    return QRect(x, sectionRect.top() + margin, size, size);
}

void CustomCheckBoxHeaderView::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const
{
    painter->save();

    // 绘制默认表头背景
    QStyleOptionHeader opt;
    opt.initFrom(this);
    opt.rect = rect;
    opt.section = logicalIndex;
    opt.textAlignment = Qt::AlignCenter;
    opt.text = model()->headerData(logicalIndex, orientation()).toString();

    // 高亮悬停列
    if (logicalIndex == m_hoverSection) {
        opt.state |= QStyle::State_MouseOver;
    }

    style()->drawControl(QStyle::CE_Header, &opt, painter, this);

    // 绘制复选框（如果该列需要）
    if (isSectionCheckable(logicalIndex)) {
        QStyleOptionButton checkOpt;
        checkOpt.rect = checkboxRect(rect);
        checkOpt.state = QStyle::State_Enabled;

        if (checkState(logicalIndex) == Qt::Checked) {
            checkOpt.state |= QStyle::State_On;
        }
        else if (checkState(logicalIndex) == Qt::PartiallyChecked) {
            checkOpt.state |= QStyle::State_NoChange;
        }
        else {
            checkOpt.state |= QStyle::State_Off;
        }

        // 悬停效果
        if (logicalIndex == m_hoverSection && checkOpt.rect.contains(mapFromGlobal(QCursor::pos()))) {
            checkOpt.state |= QStyle::State_MouseOver;
        }

        style()->drawControl(QStyle::CE_CheckBox, &checkOpt, painter);
    }

    painter->restore();
}

void CustomCheckBoxHeaderView::mousePressEvent(QMouseEvent* event)
{
    const int section = logicalIndexAt(event->pos());

    if (section >= 0 && isSectionCheckable(section)) {
        // 获取列矩形区域
        QRect sectionRect = QRect(sectionViewportPosition(section), 0,
            sectionSize(section), height());

        QRect checkRect = checkboxRect(sectionRect);

        if (checkRect.contains(event->pos())) {
            // 切换复选框状态
            Qt::CheckState newState = (checkState(section) == Qt::Checked)
                ? Qt::Unchecked : Qt::Checked;
            setCheckState(section, newState);
            emit checkStateChanged(section, newState == Qt::Checked);
            return;
        }
    }

    QHeaderView::mousePressEvent(event);
}

void CustomCheckBoxHeaderView::mouseMoveEvent(QMouseEvent* event)
{
    // 更新悬停列
    const int oldHover = m_hoverSection;
    m_hoverSection = logicalIndexAt(event->pos());

    // 重绘需要更新的部分
    if (oldHover != m_hoverSection) {
        if (oldHover >= 0) updateSection(oldHover);
        if (m_hoverSection >= 0) updateSection(m_hoverSection);
    }

    QHeaderView::mouseMoveEvent(event);
}

// 事件过滤器 - 用于捕获列宽调整操作
bool CustomCheckBoxHeaderView::eventFilter(QObject* obj, QEvent* ev)
{
    if (ev->type() == QEvent::Resize) {
        applyWidthConstraints();
    }
    return QHeaderView::eventFilter(obj, ev);
}

// 事件处理 - 用于捕获列宽调整操作
bool CustomCheckBoxHeaderView::event(QEvent* event)
{
    // 鼠标离开时清除悬停状态
    if (event->type() == QEvent::Leave) {
        if (m_hoverSection >= 0) {
            const int oldHover = m_hoverSection;
            m_hoverSection = -1;
            updateSection(oldHover);
        }
    }
    // 在布局变化后应用宽度约束
    else if (event->type() == QEvent::LayoutRequest) {
        applyWidthConstraints();
    }

    return QHeaderView::event(event);
}

// 设置列宽范围
void CustomCheckBoxHeaderView::setSectionWidthRange(int section, int minWidth, int maxWidth)
{
    if (minWidth < 0 || maxWidth < minWidth)
        return;

    m_sectionWidthRanges[section] = qMakePair(minWidth, maxWidth);
    applyWidthConstraints(); // 立即应用约束
}

// 清除单列宽度范围
void CustomCheckBoxHeaderView::clearSectionWidthRange(int section)
{
    m_sectionWidthRanges.remove(section);
}

// 清除所有列宽范围
void CustomCheckBoxHeaderView::clearAllWidthRanges()
{
    m_sectionWidthRanges.clear();
}

// 应用宽度约束
void CustomCheckBoxHeaderView::applyWidthConstraints()
{
    if (m_sectionWidthRanges.isEmpty())
        return;

    for (auto it = m_sectionWidthRanges.begin(); it != m_sectionWidthRanges.end(); ++it) {
        int section = it.key();
        int minWidth = it.value().first;
        int maxWidth = it.value().second;
        int currentSize = sectionSize(section);

        if (currentSize < minWidth) {
            QHeaderView::resizeSection(section, minWidth);
        }
        else if (currentSize > maxWidth) {
            QHeaderView::resizeSection(section, maxWidth);
        }
    }
}
