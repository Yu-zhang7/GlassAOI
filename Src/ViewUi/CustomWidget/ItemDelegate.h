#ifndef ITEMDELEGATE_H
#define ITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QModelIndex>
#include <QObject>
#include <QWidget>

class ItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ItemDelegate(QObject* parent = nullptr);

    // 创建编辑器
    QWidget* createEditor(QWidget* parent,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const override;

    // 将编辑器数据保存到模型
    void setModelData(QWidget* editor,
        QAbstractItemModel* model,
        const QModelIndex& index) const override;

signals:
    // 复选框状态改变信号
    void checkboxStateChanged(const QModelIndex& index, bool checked) const;

protected:
    void paint(QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const override;

    bool editorEvent(QEvent* event,
        QAbstractItemModel* model,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) override;

    void updateEditorGeometry(QWidget* editor,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const;
private:
    // 更新父节点值
    void UpdateParentValue(QAbstractItemModel* model,
        const QModelIndex& parentIndex) const;

    //给图标添加背景
    QPixmap AddBackground(const QPixmap& src) const;
};

#endif // ITEMDELEGATE_H