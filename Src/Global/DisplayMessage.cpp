#include "DisplayMessage.h"
#include <QMetaObject>
#include <QGuiApplication>

DisplayMessage& DisplayMessage::getInstance()
{
    static DisplayMessage instance;
    return instance;
}

void DisplayMessage::setDisplayLabel(QLabel* label)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_displayLabel = label;

    // 连接信号槽，确保跨线程安全
    connect(this, &DisplayMessage::newLogMessage,
        this, [this](const QString& message) {
            if (m_displayLabel) {
                m_displayLabel->setText(message);
            }
        }, Qt::QueuedConnection);
}

void DisplayMessage::logMessage(const QString& message, const QString& source)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 获取当前时间
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    // 构建完整的日志消息
    QString fullMessage;
    bool onlyMessage = true;
    if (onlyMessage)
    {
        fullMessage = message;
    }
    else
    {
        if (source.isEmpty()) {
            fullMessage = QString("[%1] %2").arg(timestamp, message);
        }
        else {
            fullMessage = QString("[%1] [%2] %3").arg(timestamp, source, message);
        }
    }
    // 使用信号槽机制确保线程安全
    emit newLogMessage(fullMessage);
}