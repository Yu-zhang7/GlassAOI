#ifndef EDITABLECHECKBOX_H
#define EDITABLECHECKBOX_H

#include <QCheckBox>
#include <qlineedit.h>
#include <qstring.h>
#include <functional>

class EditableCheckBox : public QCheckBox
{
    Q_OBJECT

public:
    //构造函数
    EditableCheckBox(QWidget* parent);

    void SetSaveCallback(std::function<void(const std::string&)> callback)
    {
        m_saveCallback = callback;
    }

    void SetEditable(bool editable)
    {
        m_editable = editable;
        
    }
    //析构函数
    ~EditableCheckBox() = default;

protected:
    //鼠标双击事件
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private slots:
    //保存文本
    void SaveText();

private:
    //开始编辑
    void StartEdit();

private:
    //编辑框
    QLineEdit*  m_lineEdit;
    QString     m_text;
    
    //保存回调
    std::function<void(const std::string&)> m_saveCallback;

    bool m_editable = false;
};

#endif // !EDITABLECHECKBOX_H
