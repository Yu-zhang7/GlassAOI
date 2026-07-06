#include "ItemDelegate.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QStyledItemDelegate>
#include <QApplication>
#include <QStyleOption>
#include <QPainter>
#include <QTreeView>
#include <QTableView>
#include <QHeaderView>
#include <QDebug>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QMouseEvent>
#include <QLabel>
#include "ConfigManager.h"
#include "Global.h"
#include <limits>

const int CHECKBOX_LEFT_MARGIN = 6; // 左侧留白6像素

ItemDelegate::ItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void ItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // 检查是否是需要显示复选框的项
    if (index.data(Qt::UserRole + 8).isValid())
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        // 保存原始文本
        QString text = opt.text;
        opt.text = "";

        // 绘制背景和基础文本
        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter);

        // 计算复选框区域
        QRect checkRect = option.rect;
        checkRect.setWidth(20);
        checkRect = QStyle::alignedRect(opt.direction, Qt::AlignLeft | Qt::AlignVCenter,
            QSize(20, option.rect.height()), option.rect);
        checkRect.adjust(CHECKBOX_LEFT_MARGIN, 0, CHECKBOX_LEFT_MARGIN, 0); // 添加左侧边距
        // 绘制复选框
        QStyleOptionButton checkOpt;
        checkOpt.rect = checkRect;
        checkOpt.state = opt.state;
        checkOpt.state |= index.data(Qt::UserRole + 9).toBool() ? QStyle::State_On : QStyle::State_Off;
        checkOpt.state |= QStyle::State_Enabled;
        QApplication::style()->drawControl(QStyle::CE_CheckBox, &checkOpt, painter);

        //获取图片路径
        QString imagePath = index.data(Qt::UserRole + 20).toString();
        if (!imagePath.isEmpty())
        {
            // 图标区域（在复选框右侧）
            QRect iconRect = option.rect;
            iconRect.setLeft(checkRect.right() + CHECKBOX_LEFT_MARGIN); // 与复选框间距 6px
            iconRect.setWidth(20);
            iconRect.setHeight(20);
            iconRect.moveCenter(QPoint(iconRect.center().x(), option.rect.center().y()));

            // 加载并绘制图片
            QPixmap pixmap(imagePath);
            pixmap = AddBackground(pixmap);
            if (!pixmap.isNull()) {
                painter->drawPixmap(iconRect, pixmap.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
            else {
                // 可选：绘制占位符或调试提示
                painter->drawRect(iconRect);
                painter->drawText(iconRect, Qt::AlignCenter, "?");
            }

            // 更新文本起始位置：跳过复选框 + 图标 + 间距
            int textLeft = iconRect.right() + 4;
            QRect textRect = option.rect;
            textRect.setLeft(textLeft);
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
        }
        else
        {
            // 无图片时，文本紧跟复选框
            QRect textRect = option.rect.adjusted(checkRect.width() + CHECKBOX_LEFT_MARGIN + 4, 0, 0, 0);
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
        }
    }
    else
    {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

bool ItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
    const QStyleOptionViewItem& option, const QModelIndex& index)
{
    // 处理复选框点击
    if (index.data(Qt::UserRole + 8).isValid() && event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (!mouseEvent) {
            return QStyledItemDelegate::editorEvent(event, model, option, index);
        }
        // 计算复选框区域
        QRect checkRect = option.rect;
        checkRect.setWidth(20);
        checkRect = QStyle::alignedRect(option.direction, Qt::AlignLeft | Qt::AlignVCenter,
            QSize(20, option.rect.height()), option.rect);
        checkRect.adjust(CHECKBOX_LEFT_MARGIN, 0, CHECKBOX_LEFT_MARGIN, 0); // 添加左侧边距

        // 图片区域（与 paint 中一致）
        QString imagePath = index.data(Qt::UserRole + 20).toString();
        QRect iconRect;
        if (!imagePath.isEmpty()) {
            iconRect = option.rect;
            iconRect.setLeft(checkRect.right() + CHECKBOX_LEFT_MARGIN);
            iconRect.setWidth(16);
            iconRect.setHeight(16);
            iconRect.moveCenter(QPoint(iconRect.center().x(), option.rect.center().y()));
        }

        // 如果点击在复选框区域内或图片区域
        if (checkRect.contains(mouseEvent->pos()) ||
            (!imagePath.isEmpty() && iconRect.contains(mouseEvent->pos())))
            {
            // 切换复选框状态
            bool newState = !index.data(Qt::UserRole + 9).toBool();
            model->setData(index, newState, Qt::UserRole + 9); // 存储复选框状态
            model->setData(index, index.data(Qt::DisplayRole), Qt::EditRole); // 触发数据改变信号

            // 发射复选框状态改变信号
            emit checkboxStateChanged(index, newState);
            return true;
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

QWidget* ItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // 处理路径选择列
    if (index.data(Qt::UserRole + 1).toString() == "path")
    {
        QString path = QFileDialog::getExistingDirectory(
            parent,
            tr("Choose Directory"),
            QDir::currentPath(),
            QFileDialog::ShowDirsOnly);
        if (!path.isEmpty()) {
            QFileInfo fileInfo(path);
            if (fileInfo.isDir() && !fileInfo.isSymLink()) {
                const_cast<QAbstractItemModel*>(index.model())->setData(index, path);
                //ConfigManager::getInstance().setConfigValue<std::string>(DATA_SAVE_PATH, path.toStdString());

                // 触发视图更新
                if (auto view = qobject_cast<const QTreeView*>(option.widget)) {
                    const_cast<QTreeView*>(view)->viewport()->update();
                }
            }
        }
        return nullptr;
    }
    // 检查是否需要创建复选框+文本编辑器
    if (index.data(Qt::UserRole + 8).isValid())
    {        // 仅当 UserRole+33 为 true 时，才允许进入编辑状态
        if (index.data(Qt::UserRole + 33).toBool())
        {
            QWidget* editor = new QWidget(parent);
            QHBoxLayout* layout = new QHBoxLayout(editor);
            layout->setContentsMargins(CHECKBOX_LEFT_MARGIN, 0, 0, 0);
            layout->setSpacing(4);

            // 创建复选框
            QCheckBox* checkBox = new QCheckBox(editor);
            checkBox->setChecked(index.data(Qt::UserRole + 9).toBool());
            // 获取并显示对应图片
            QString imagePath = index.data(Qt::UserRole + 20).toString();
            QLabel* iconLabel = new QLabel(editor);
            if (!imagePath.isEmpty()) {
                QPixmap pixmap(imagePath);
                pixmap = AddBackground(pixmap);
                if (!pixmap.isNull()) {
                    iconLabel->setPixmap(pixmap.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
            }
            iconLabel->setFixedSize(20, 20);

            // 判断是否允许编辑文本
            bool allowTextEdit = index.data(Qt::UserRole + 33).toBool();

            QLineEdit* lineEdit = nullptr;
            if (allowTextEdit) {
                lineEdit = new QLineEdit(editor);
                lineEdit->setText(index.data(Qt::DisplayRole).toString());
            }

            // 布局：复选框 -> 图标 -> 文本
            layout->addWidget(checkBox);
            layout->addWidget(iconLabel);
            if (lineEdit) {
                layout->addWidget(lineEdit);
            }

            // 连接复选框状态改变信号
            QObject::connect(checkBox, &QCheckBox::stateChanged, [this, index, checkBox]() {
                // 发射复选框状态改变信号
                const_cast<ItemDelegate*>(this)->emit checkboxStateChanged(index, checkBox->isChecked());
                });
            return editor;
        }
        else
        {
            // 不允许编辑文本：禁止进入编辑模式
            return nullptr;
        }
    }
    // 检查是否需要创建数值编辑器
    if (index.data(Qt::UserRole + 2).toString() == "confidence1" ||
        index.data(Qt::UserRole + 3).toString() == "confidence2" ||
        index.data(Qt::UserRole + 10).isValid() || // 有范围设置
        index.data(Qt::UserRole + 11).isValid())
    {
        // 明确创建 QDoubleSpinBox 而不是依赖基类
        QDoubleSpinBox* spinBox = new QDoubleSpinBox(parent);

        // 从模型获取范围数据
        double min = index.data(Qt::UserRole + 10).isValid()
            ? index.data(Qt::UserRole + 10).toDouble()
            : 0.0; // 默认最小值

        double max = index.data(Qt::UserRole + 11).isValid()
            ? index.data(Qt::UserRole + 11).toDouble()
            : 99999.0; // 默认最大值

        double step = index.data(Qt::UserRole + 12).isValid()
            ? index.data(Qt::UserRole + 12).toDouble()
            : 0.1; // 默认步长

        int decimals = index.data(Qt::UserRole + 13).isValid()
            ? index.data(Qt::UserRole + 13).toInt()
            : 2; // 默认小数位数
        spinBox->setRange(min, max);
        spinBox->setSingleStep(step);
        spinBox->setDecimals(decimals);

        // 设置初始值
		QString displayText = index.data(Qt::EditRole).toString();
        displayText = displayText.remove(u8"≥");
        spinBox->setValue(displayText.toDouble());					  
        return spinBox;
    }

    // 其他情况使用基类创建编辑器
    return QStyledItemDelegate::createEditor(parent, option, index);
}

void ItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    // 处理路径选择列（无操作）
    if (index.data(Qt::UserRole + 1).toString() == "path")
    {
        return;
    }
    // 处理复选框+文本编辑器
    if (index.data(Qt::UserRole + 8).isValid())
    {
        QCheckBox* checkBox = editor->findChild<QCheckBox*>();
        QLineEdit* lineEdit = editor->findChild<QLineEdit*>();

        if (checkBox) {
            // 保存复选框状态到UserRole+9
            model->setData(index, checkBox->isChecked(), Qt::UserRole + 9);
        }

        // 只有在允许编辑文本时才保存文本
        bool allowTextEdit = index.data(Qt::UserRole + 33).toBool();
        if (allowTextEdit && lineEdit) {
            model->setData(index, lineEdit->text(), Qt::EditRole);
        }

        // 更新父节点
        UpdateParentValue(model, index.parent());
        return;
    }
    // 处理数值输入列
    if (QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(editor))
    {
        double value = spinBox->value();
        

        // 获取小数位数（如果设置了的话）
        int decimals = index.data(Qt::UserRole + 13).isValid()
            ? index.data(Qt::UserRole + 13).toInt()
            : 2;

        // 判断是否需要添加 ≥ 符号
        if (index.data(Qt::UserRole + 32).toString() == "minor" || // 有范围设置
            index.data(Qt::UserRole + 32).toString() == "moderate" ||
            index.data(Qt::UserRole + 32).toString() == "major")
        {
            QString displayText;
            displayText = u8"≥" + QString::number(value, 'f', decimals);
            model->setData(index, displayText, Qt::DisplayRole);
        }
        // 如果该值影响父节点，则更新父节点
        if (index.data(Qt::UserRole + 32).toString() == "confidence" ||
            index.data(Qt::UserRole + 32).toString() == "confidence1" ||
            index.data(Qt::UserRole + 32).toString() == "confidence2" ||
            index.data(Qt::UserRole + 32).toString() == "mergeDistance" ||
            index.data(Qt::UserRole + 32).toString() == "grayDifference")
        {
            model->setData(index, spinBox->value(), Qt::EditRole);
            UpdateParentValue(model, index.parent());
        }
        return;

    }

    // 处理其他编辑器类型
   QStyledItemDelegate::setModelData(editor, model, index);
}

void ItemDelegate::UpdateParentValue(QAbstractItemModel* model, const QModelIndex& parentIndex) const
{
    QStringList childValues;
    for (int row = 0; row < model->rowCount(parentIndex); ++row) {
        QModelIndex childIndex = model->index(row, 1, parentIndex);
        childValues << model->data(childIndex).toString();
    }
    model->setData(parentIndex.sibling(parentIndex.row(), 1), childValues.join(", "));
}

QPixmap ItemDelegate::AddBackground(const QPixmap& src) const
{
    // 使用 #FF9800 对应的 RGB 值创建 QColor 实例
    QColor bgColor(255, 152, 0); // #FF9800

    QPixmap result(src.size());
    result.fill(bgColor); // 使用特定颜色填充背景
    QPainter painter(&result);
    painter.drawPixmap(0, 0, src); // 将原始图像绘制到新背景上
    painter.end();
    return result;
}

void ItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    editor->setGeometry(option.rect);
}