#ifndef CHANGEDEFECTINFODIALOG_H
#define CHANGEDEFECTINFODIALOG_H

#include <QDialog>
#include "Global.h"
#include "ui_ChangeDefectInfoDialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ChangeDefectInfoDialogClass; };
QT_END_NAMESPACE

class ChangeDefectInfoDialog : public QDialog
{
    Q_OBJECT

public:
    /* 构造函数 */
    ChangeDefectInfoDialog(DefectLevel level, DefectType type, QWidget *parent = nullptr);
    
    /* 析构函数 */
    ~ChangeDefectInfoDialog();

    /* 获取缺陷的等级 */
    DefectLevel GetDefectLevel() const
    {
        return m_level;
    }

    /* 获取缺陷的类型 */
    DefectType GetDefectType() const
    {
        return m_type;
    }

private slots:
    /* 接受按钮的函数 */
    void AccpectSlots();

private:
    Ui::ChangeDefectInfoDialogClass *ui;
    DefectLevel                     m_level;//记录缺陷等级
    DefectType                      m_type; //记录缺陷类型
};

#endif
