#ifndef DISPLAY_MESSAGE_H
#define DISPLAY_MESSAGE_H

#include <QObject>
#include <QString>
#include <QLabel>
#include <QDateTime>
#include <mutex>
#include <memory>

class DisplayMessage : public QObject
{
    Q_OBJECT

public:
    static DisplayMessage& getInstance();

    void setDisplayLabel(QLabel* label);
    void logMessage(const QString& message, const QString& source = "");

    // ½ûÖ¹¿½±´ºÍÒÆ¶¯
    DisplayMessage(const DisplayMessage&) = delete;
    DisplayMessage& operator=(const DisplayMessage&) = delete;
    DisplayMessage(DisplayMessage&&) = delete;
    DisplayMessage& operator=(DisplayMessage&&) = delete;

signals:
    void newLogMessage(const QString& message);

private:
    DisplayMessage() = default;
    ~DisplayMessage() = default;

    QLabel* m_displayLabel = nullptr;
    std::mutex m_mutex;
};

#endif // DISPLAY_MESSAGE_H