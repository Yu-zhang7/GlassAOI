#include "EditableCheckBox.h"
#include <QMouseEvent>

EditableCheckBox::EditableCheckBox(QWidget *parent)
    : QCheckBox(parent)
    , m_lineEdit(new QLineEdit(this))
    , m_text("")
{
    m_lineEdit->hide();
    connect(m_lineEdit, &QLineEdit::returnPressed, this, &EditableCheckBox::SaveText);
}

void EditableCheckBox::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (m_editable)
    {
        if (event->button() == Qt::LeftButton)
        {
            StartEdit();
        }
    }
    QCheckBox::mouseDoubleClickEvent(event);
}

void EditableCheckBox::SaveText()
{
    m_text = m_lineEdit->text();
    setText(m_text);
    m_lineEdit->hide();
    setText(m_text);
    if (m_saveCallback)
    {
        m_saveCallback(m_text.toStdString());
    }
}

void EditableCheckBox::StartEdit()
{
    m_text = text();
    m_lineEdit->setText(m_text);
    m_lineEdit->setGeometry(rect());
    m_lineEdit->show();
    m_lineEdit->setFocus();
}
