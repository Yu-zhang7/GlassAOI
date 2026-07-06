#include "DefectFilterWidget.h"
#include <QLabel>
#include <QFrame>
#include <QApplication>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include "Log.hpp"
#include "Global.h"


// 添加静态函数，根据DefectType获取对应的图标
QIcon DefectFilterWidget::getDefectIcon(DefectType type)
{
    // 使用提供的DefectTypeToImage函数获取QImage
    QImage image = DefectTypeToImage(type);

    // 如果图像无效，返回空图标
    if (image.isNull()) {
        return QIcon();
    }

    // 将QImage转换为QPixmap并创建图标
    // 可以根据需要调整图标大小，这里建议使用16x16或24x24
    QPixmap pixmap = QPixmap::fromImage(image);

    // 使用 #FF9800 对应的 RGB 值创建 QColor 实例
    QColor bgColor(255, 152, 0); // #FF9800

    QPixmap result(pixmap.size());
    result.fill(bgColor); // 使用特定颜色填充背景
    QPainter painter(&result);
    painter.drawPixmap(0, 0, pixmap); // 将原始图像绘制到新背景上
    painter.end();
    // 缩放图片到合适的大小作为图标（例如16x16）
    pixmap = pixmap.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    return QIcon(result);
}

DefectFilterWidget::DefectFilterWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();

    createCheckBoxes();

    // 在构造函数中初始化定时器
    //m_resizeTimer.setSingleShot(true);
    //m_resizeTimer.setInterval(200); // 200毫秒延迟
    //connect(&m_resizeTimer, &QTimer::timeout, this, &DefectFilterWidget::delayedRearrange);
    //// 延迟布局计算，确保父对象已完成构造
    //QTimer::singleShot(0, this, &DefectFilterWidget::rearrangeCheckBoxes);

    // 触发一次布局（确保首次显示正确）
    QTimer::singleShot(0, this, &DefectFilterWidget::rearrangeCheckBoxes);
}

DefectFilterWidget::~DefectFilterWidget()
{
    //std::cout<<"DefectFilterWidget::~DefectFilterWidget()"<<std::endl;
    // 自动删除所有子控件，包括复选框
}

void DefectFilterWidget::setupUI()
{

    // 创建主垂直布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setAlignment(Qt::AlignTop);
    m_mainLayout->setSpacing(0);
    m_mainLayout->setContentsMargins(10, 0, 10, 0);
    this->setLayout(m_mainLayout);

    // 设置主布局的大小策略
    //m_mainLayout->setSizeConstraint(QLayout::SetMinimumSize);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void DefectFilterWidget::createCheckBoxes()
{
    int defectTypeCount = static_cast<int>(DefectType::DEFECT_TYPE_COUNT);
    m_checkBoxes.clear();

    for (int i = 0; i < defectTypeCount; ++i) {
        DefectType type = static_cast<DefectType>(i);
        QString text = DefectTypeToString(type);

        // 获取对应缺陷类型的图标
        QIcon icon = getDefectIcon(type);

        QCheckBox* checkBox = new QCheckBox(text, this);
        checkBox->setProperty("defectType", i); // 关键：存储原始枚举值

        // 设置图标
        if (!icon.isNull()) {
            checkBox->setIcon(icon);
            // 可以调整图标大小
            checkBox->setIconSize(QSize(16, 16));
        }
        // 设置文本与图标之间的间距
        checkBox->setStyleSheet("QCheckBox { spacing: 5px; }");

        m_checkBoxes.append(checkBox);

        // 初始可见性由 IsXXXUsed 决定
        checkBox->setVisible(m_defectTypeUsability[type]);
    }

    // 连接信号
    for (QCheckBox* checkBox : m_checkBoxes) {
        connect(checkBox, &QCheckBox::stateChanged, this, &DefectFilterWidget::onCheckBoxStateChanged);
    }

    rearrangeCheckBoxes();
}

void DefectFilterWidget::rearrangeCheckBoxes()
{
    const int availableWidth = width() - 20; // 减去左右 margin
    if (availableWidth <= 0 || m_checkBoxes.isEmpty()) {
        return;
    }

    // 清理旧布局
    qDeleteAll(m_rowLayouts);
    m_rowLayouts.clear();

    QLayoutItem* item;
    while ((item = m_mainLayout->takeAt(0))) {
        delete item;
    }

    const int spacing = 10; // 与 QHBoxLayout::setSpacing 一致

    QHBoxLayout* currentRow = nullptr;
    int currentRowWidth = 0; // 当前行已占用的像素宽度（不含 trailing spacing）

    for (QCheckBox* cb : m_checkBoxes) {
        // Qt 会自动忽略不可见控件，但为了 width 计算准确，我们仍可跳过（非必须）
        if (!cb->isVisible()) {
            continue; // 可选：显式跳过，逻辑更清晰
        }

        int cbWidth = cb->sizeHint().width();

        // 计算加入当前控件后的新宽度
        int newWidth = currentRowWidth;
        if (currentRow != nullptr) {
            // 如果不是第一个控件，需加上 spacing
            newWidth += spacing;
        }
        newWidth += cbWidth;

        // 判断是否超出可用宽度
        if (currentRow == nullptr || newWidth > availableWidth) {
            // 需要换行
            if (currentRow != nullptr) {
                currentRow->addStretch(); // 左对齐
            }

            // 创建新行
            currentRow = new QHBoxLayout();
            currentRow->setSpacing(spacing);
            currentRow->setContentsMargins(0, 0, 0, 0);
            m_mainLayout->addLayout(currentRow);
            m_rowLayouts.append(currentRow);

            // 重置当前行状态
            currentRowWidth = cbWidth;
        }
        else {
            // 可以加入当前行
            currentRowWidth = newWidth;
        }

        currentRow->addWidget(cb);
    }

    // 处理最后一行
    if (currentRow != nullptr) {
        currentRow->addStretch();
    }

    m_mainLayout->addStretch(); // 顶部对齐
    updateGeometry();
}

// 设置指定索引的复选框状态
void DefectFilterWidget::setCheckBoxState(int index, bool checked)
{
    // 检查索引是否有效
    if (index < 0 || index >= m_checkBoxes.size()) {
        qWarning() << "Invalid checkbox index:" << index;
        return;
    }

    // 设置复选框状态
    m_checkBoxes[index]->setChecked(checked);
}

// 设置所有复选框的状态
void DefectFilterWidget::setAllCheckBoxesState(bool checked)
{
    // 遍历所有复选框并设置状态
    for (QCheckBox* checkBox : m_checkBoxes) {
        checkBox->setChecked(checked);
    }
}

// 连接复选框状态变化信号到父对象槽函数
void DefectFilterWidget::connectCheckBoxChangesTo(QObject* receiver, const char* slot)
{
    m_receiver = receiver;
    m_slot = slot;

    // 连接内部信号到父对象的槽函数
    connect(this, SIGNAL(checkBoxStateChanged(int, bool)), receiver, slot);
}

// 内部槽函数，处理单个复选框状态变化
void DefectFilterWidget::onCheckBoxStateChanged(int state)
{
    // 获取发送信号的复选框
    QCheckBox* checkBox = qobject_cast<QCheckBox*>(sender());
    if (!checkBox) {
        return;
    }

    // 获取复选框的索引
    int index = checkBox->property("defectType").toInt();
    bool checked = (state == Qt::Checked);

    // 发出复选框状态变化信号
    emit checkBoxStateChanged(index, checked);
    std::cout<<"DefectFilterWidget::onCheckBoxStateChanged"<<index <<"," << checked << std::endl;
}

void DefectFilterWidget::resizeEvent(QResizeEvent* event)
{
    qDebug() << "DefectFilterWidget resized to:" << event->size();
    QWidget::resizeEvent(event);

    //m_resizeTimer.start(); // 启动或重新启动定时器
    // 重新布局复选框
    rearrangeCheckBoxes();
}

void DefectFilterWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    static bool firstShow = true;
    if (firstShow) {
        firstShow = false;
        rearrangeCheckBoxes();
    }
}

QList<DefectType> DefectFilterWidget::getSelectedDefectTypes() const
{
    QList<DefectType> selectedTypes;

    for (QCheckBox* checkBox : m_checkBoxes) {
        if (checkBox->isChecked()) {
            int typeValue = checkBox->property("defectType").toInt();
            selectedTypes.append(static_cast<DefectType>(typeValue));
        }
    }

    return selectedTypes;
}

void DefectFilterWidget::setDisplayStatus(DefectType defectType, bool isDisplay)
{
    // 遍历所有复选框查找与指定defectType匹配的复选框并设置其可见性
    for (QCheckBox* checkBox : m_checkBoxes)
    {
        if (checkBox->property("defectType").toInt() == (int)defectType)
        {
            checkBox->setVisible(isDisplay);
            break; 
        }
    }

    // 重新排列复选框以适应新的布局要求
    rearrangeCheckBoxes();
}