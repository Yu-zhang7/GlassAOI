#include "LicenseManager.h"
#include "CryptoUtils.h"
#include <QStandardPaths>
#include <QDir>
#include <QIcon>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QRandomGenerator>

// 初始化静态成员变量
LicenseManager* LicenseManager::m_instance = nullptr;

LicenseManager* LicenseManager::getInstance(const QString& customerName)
{
    if (!m_instance) {
        m_instance = new LicenseManager(customerName);
    }
    return m_instance;
}

void LicenseManager::releaseInstance()
{
    if (m_instance) {
        delete m_instance;
        m_instance = nullptr;
    }
}

LicenseManager::LicenseManager(const QString& customerName, QObject* parent)
    : QObject(parent)
    , m_settings(nullptr)
    , m_dailyTimer(nullptr)
    , m_licenseType(Trial)
    , m_isLicensed(false)
    , m_customerName(customerName)
    , m_encryptionKey("Qt5LicenseSystem2022@VS")    //密钥。注意，如果更改，则需要重新生成密钥
    , m_reminderDays(15)
{
    // Set anti-tamper file path
    QDir appDir(QCoreApplication::applicationDirPath());
    //m_antiTamperFilePath = appDir.absoluteFilePath("./Detect/modelL.engine");
    m_antiTamperFilePath = "./Detect/modelL.engine";

    // Ensure directory exists
    QFileInfo fileInfo(m_antiTamperFilePath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    initStyleSheet();

    initialize();
}

LicenseManager::~LicenseManager()
{
    //qDebug() << "LicenseManager::~LicenseManager() begin";
    if (m_dailyTimer) {
        m_dailyTimer->stop();
        delete m_dailyTimer;
    }
    if (m_settings) {
        delete m_settings;
    }
    //qDebug() << "LicenseManager::~LicenseManager() over";
}

bool LicenseManager::initialize()
{
    // 初始化定时器
    qDebug() << "Initializing";
    m_dailyTimer= new QTimer(this);
    m_dailyTimer->setSingleShot(true); // 设置为单次触发
    connect(m_dailyTimer, &QTimer::timeout, this, &LicenseManager::onDailyCheck);

    // 启动第一次定时
    setupDailyTimer();

    return true;
}

void LicenseManager::initStyleSheet()
{
    m_styleSheet = R"(
       /* ================= 颜色定义 ================= */
/* 主要背景色: #404040 */
/* 主要文字色: #FFFFFF */
/* 禁用文字色: rgba(255, 255, 255, 128) */
/* 强调色: #FF9800 */
/* 强调悬停色: #33FFD0 */
/* 强调边框色: #FF9800 */
/* 深灰色1: #484848 */
/* 深灰色2: #505050 */
/* 深灰色3: #585858 */
/* 深灰色4: #646464 */
/* 输入背景色: #505050 */
/* 输入文字色: #FFFFFF */

/* ================= QMessageBox 样式 ================= */
QMessageBox {
    background-color: #404040;
    color: #FFFFFF;
    border: 0;
    font-family: "微软雅黑";
}

QMessageBox QLabel {
    color: #FFFFFF;
    background-color: transparent;
}

QMessageBox QLabel::disabled {
    color: rgba(255, 255, 255, 128);
}

QMessageBox QPushButton {
    border: 1px solid #646464;
    color: #FFFFFF;
    padding: 8px 15px;
    border-radius: 5px;
    background: #585858;
}

QMessageBox QPushButton:hover {
    font-weight: bold;
    background: #646464;
    border-bottom: 2px solid #FF9800;
}

QMessageBox QPushButton:pressed {
    background: #505050;
}

QMessageBox QPushButton:disabled {
    color: rgba(255, 255, 255, 128);
    background: #3A3A3A;
}

/* ================= QInputDialog 样式 ================= */
QInputDialog {
    background-color: #404040;
    color: #FFFFFF;
    border: 0;
    font-family: "微软雅黑";

}

QInputDialog QLabel {
    color: #FFFFFF;
    background-color: transparent;
}

QInputDialog QLabel::disabled {
    color: rgba(255, 255, 255, 128);
}

/* 输入框样式 */
QInputDialog QLineEdit {
    background-color: #505050;
    color: #FFFFFF;
    border-radius: 5px;
    border: 1px solid #646464;
    font-family: "微软雅黑";
    padding: 5px 12px;
    selection-background-color: #FF9800;
    selection-color: #000000;
}

QInputDialog QLineEdit:focus {
    border: 1px solid #FF9800;
}

QInputDialog QLineEdit[echoMode="2"] {
    lineedit-password-character: 9679;
}

QInputDialog QLineEdit::disabled {
    background-color: #3A3A3A;
    color: rgba(255, 255, 255, 100);
}

/* 多行文本输入框 */
QInputDialog QPlainTextEdit {
    font-family: "微软雅黑";
    color: #FFFFFF;
    background-color: #505050;
    border-radius: 5px;
    padding: 5px;
    border: 1px solid #646464;
}

QInputDialog QPlainTextEdit:focus {
    border: 1px solid #FF9800;
}

/* 按钮样式 */
QInputDialog QPushButton {
    border: 1px solid #646464;
    color: #FFFFFF;
    padding: 8px 15px;
    border-radius: 5px;
    background: #585858;
}

QInputDialog QPushButton:hover {
    font-weight: bold;
    background: #646464;
    border-bottom: 2px solid #FF9800;
}

QInputDialog QPushButton:pressed {
    background: #505050;
}

QInputDialog QPushButton:disabled {
    color: rgba(255, 255, 255, 128);
    background: #3A3A3A;
}

/* 按钮盒布局 */
QInputDialog QDialogButtonBox {
    /* 确保按钮布局正确 */
}

/* 组合框（如果QInputDialog使用组合框） */
QInputDialog QComboBox {
    background-color: #505050;
    color: #FFFFFF;
    border-radius: 5px;
    border: 1px solid #646464;
    padding: 5px 12px;
}

QInputDialog QComboBox:focus {
    border: 1px solid #FF9800;
}

QInputDialog QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 25px;
    border-left: 1px solid #646464;
    background: #585858;
    border-top-right-radius: 4px;
    border-bottom-right-radius: 4px;
}

QInputDialog QComboBox::down-arrow {
    image: url(:/Arrow/Arrow/Down_arrow_light.png);
    width: 10px;
    height: 10px;
}

QInputDialog QComboBox QAbstractItemView {
    background-color: #505050;
    color: #FFFFFF;
    border: 1px solid #646464;
    border-radius: 5px;
    selection-background-color: #FF9800;
    selection-color: #000000;
    margin: 15px 0 0 0;
}

/* 微调框（如果QInputDialog使用微调框） */
QInputDialog QSpinBox, QInputDialog QDoubleSpinBox {
    background-color: #505050;
    color: #FFFFFF;
    border-radius: 5px;
    border: 1px solid #646464;
    padding: 5px 12px;
    min-height: 15px;
    selection-background-color: #FF9800;
    selection-color: #000000;
}

QInputDialog QSpinBox:focus, QInputDialog QDoubleSpinBox:focus {
    border: 1px solid #FF9800;
}

QInputDialog QSpinBox::up-button, QInputDialog QDoubleSpinBox::up-button,
QInputDialog QSpinBox::down-button, QInputDialog QDoubleSpinBox::down-button {
    background: #585858;
    border-left: 1px solid #646464;
}

QInputDialog QSpinBox::up-button:hover, QInputDialog QDoubleSpinBox::up-button:hover,
QInputDialog QSpinBox::down-button:hover, QInputDialog QDoubleSpinBox::down-button:hover {
    background: #646464;
}
    )";
}

void LicenseManager::setupDailyTimer()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime nextMidnight = now.date().addDays(1).startOfDay();
    int msecToMidnight = now.msecsTo(nextMidnight);

    m_dailyTimer->start(msecToMidnight);
}

void LicenseManager::onDailyCheck()
{
    // Verify anti-tamper time on each daily check
    if (!validateAntiTamperTime()) {
        //QMessageBox::critical(nullptr, tr("System Error"),
        //    tr("Abnormal operating system time detected, software usage will be restricted!"));

        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("System Error"));
        msgBox.setText(tr("Abnormal operating system time detected, software usage will be restricted!"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStyleSheet(m_styleSheet);
        msgBox.exec();
        // Add code to restrict functionality here
        emit signal_licenseExpired();
    }

    checkTrialStatus(true);

    // 重新设置下一次定时
    setupDailyTimer();
}

void LicenseManager::checkTrialStatus(bool showReminder)
{
    if (m_licenseType == Permanent) {
        return;
    }

    int remainingDays = calculateRemainingDays();
    bool isExpired = (remainingDays < 0); // Note: changed to <0, as expiration day is still usable

    

    if (isExpired) {
        emit signal_licenseExpired();   //发出授权失效信号
        showKeyInputDialog(false);
    }
    else if (remainingDays <= m_reminderDays) {
        showReminderDialog(showReminder);
    }

}

bool LicenseManager::validateLicenseKey(const QString& key)
{
    QString customerName;
    QString deviceCode;
    LicenseType licenseType;
    QDate installDate;
    QDate generateDate;
    QDate expireDate;
    QString licenseKey;
    try {
        QString decryptedContent = CryptoUtils::decrypt(key, m_encryptionKey);
        QJsonDocument doc = QJsonDocument::fromJson(decryptedContent.toUtf8());

        if (doc.isNull()) {
            qDebug() << "Failed to parse license file JSON";
            return false;
        }

        QJsonObject licenseObj = doc.object();

        customerName = licenseObj["customerName"].toString();
        deviceCode = licenseObj["deviceCode"].toString();
        licenseType = static_cast<LicenseType>(licenseObj["licenseType"].toInt());

        installDate = QDate::fromString(licenseObj["installDate"].toString(), "yyyy-MM-dd");
        generateDate = QDate::fromString(licenseObj["generatedDate"].toString(), "yyyy-MM-dd");
        expireDate = QDate::fromString(licenseObj["expireDate"].toString(), "yyyy-MM-dd");
        licenseKey = CryptoUtils::decrypt(licenseObj["licenseKey"].toString(), m_encryptionKey);
        // 验证数据有效性
        if (customerName.isEmpty() || !installDate.isValid() || !expireDate.isValid() || !licenseKey.isEmpty()) {
            qDebug() << "Invalid data in license file";
            return false;
        }
    }
    catch (...) {
        qDebug() << "Exception occurred while parsing license file";
        return false;
    }

    QString tmpLicenseKey;
    if (customerName.isEmpty() || deviceCode.isEmpty())
    {
        return false;
    }
    // 检查客户名称
    if (customerName != m_customerName) {
        qDebug() << "Customer name mismatch:" << customerName << "!=" << m_customerName;
        //QMessageBox::information(nullptr, tr("Authorization failed"),
        //    QString(tr("Customer name mismatch: %1 != %2")).arg(customerName).arg(m_customerName));
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Authorization failed"));
        msgBox.setText(QString(tr("Customer name mismatch: %1 != %2")).arg(customerName).arg(m_customerName));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStyleSheet(m_styleSheet);
        msgBox.exec();
        return false;
    }
    // 检查设备码
    if (deviceCode != m_deviceCode) {
        qDebug() << "Customer name mismatch:" << deviceCode << "!=" << m_deviceCode;
        //QMessageBox::information(nullptr, tr("Authorization failed"),
        //    QString(tr("Device code mismatch: %1 != %2")).arg(deviceCode).arg(m_deviceCode));
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Authorization failed"));
        msgBox.setText(QString(tr("Device code mismatch: %1 != %2")).arg(deviceCode).arg(m_deviceCode));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStyleSheet(m_styleSheet);
        msgBox.exec();
        return false;
    }

    //试用授权的授权检查
    if (m_licenseType == LicenseType::Trial)
    {
        //1. 检查授权密钥格式和内容是否一致
        tmpLicenseKey = QString("TRIAL-%1-%2-%3")
            .arg(customerName)
            .arg(deviceCode)
            .arg(expireDate.toString("yyyyMMdd"));
        if (tmpLicenseKey != licenseKey)
        {
            //QMessageBox::information(nullptr, tr("Authorization failed"),
            //    QString(tr("License key error!")));
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Authorization failed"));
            msgBox.setText(QString(tr("License key error!")));
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setStyleSheet(m_styleSheet);
            msgBox.exec();
            return false;
        }

        //2. 检查授权有效期是否有效
        if (!expireDate.isValid()) {
            QString expireDateStr = expireDate.toString("yyyy-MM-dd");
            qDebug() << "Invalid expire date:" << expireDateStr;
            //QMessageBox::information(nullptr, tr("Authorization failed"),
            //    QString(tr("Invalid expire date: %1")).arg(expireDateStr));
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Authorization failed"));
            msgBox.setText(QString(tr("Invalid expire date: %1")).arg(expireDateStr));
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setStyleSheet(m_styleSheet);
            msgBox.exec();
            return false;
        }

        //3  检查授权到期日期不能早于今天
        QDate currentDate = QDate::currentDate();
        int days = currentDate.daysTo(expireDate);
        if (days < 0) {
            QString expireDateStr = expireDate.toString("yyyy-MM-dd");
            qDebug() << "Expire date is in the past:" << expireDate;
            //QMessageBox::information(nullptr, tr("Authorization failed"),
            //    QString(tr("Expire date is in the past: %1")).arg(expireDateStr));
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Authorization failed"));
            msgBox.setText(QString(tr("Expire date is in the past: %1")).arg(expireDateStr));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStyleSheet(m_styleSheet);
            msgBox.exec();
            return false;
        }


        /*return remainingDays >= 0;*/
    }
    // 永久授权的授权检查
    else
    {
        //1. 检查授权密钥格式和内容是否一致
        tmpLicenseKey = QString("PERMANENT-%1-%2")
            .arg(customerName)
            .arg(deviceCode);
        if (tmpLicenseKey != licenseKey)
        {
            //QMessageBox::information(nullptr, tr("Authorization failed"),
            //    QString(tr("License key error!")));
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Authorization failed"));
            msgBox.setText(QString(tr("License key error!")));
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setStyleSheet(m_styleSheet);
            msgBox.exec();
            return false;
        }
    }

    m_customerName = customerName;
    m_deviceCode = deviceCode;
    m_licenseType = licenseType;
    m_installDate = installDate;
    m_generateDate = generateDate;
    m_expireDate = expireDate;
    m_licenseKey = licenseKey;
    m_encryptedText = key;

    // 确保目录存在
    QFileInfo fileInfo("license.lic");
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 保存注册文件
    QFile file("license.lic");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << key;
        file.close();
    }

    //QMessageBox::information(nullptr, tr("Authorization Successful"), tr("Software permanently authorized!"));
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Authorization Successful"));
    msgBox.setText(QString(tr("Software permanently authorized!")));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStyleSheet(m_styleSheet);
    msgBox.exec();
    return true;
}

int LicenseManager::getRemainingDays() const
{
    return calculateRemainingDays();
}

LicenseManager::LicenseType LicenseManager::getLicenseType() const
{
    return m_licenseType;
}

bool LicenseManager::isLicensed() const
{
    return m_isLicensed;
}

QDate LicenseManager::getInstallDate() const
{
    return m_installDate;
}

QDate LicenseManager::getGenerateDate() const
{
    return m_generateDate;
}

QDate LicenseManager::getExpireDate() const
{
    return m_expireDate;
}

QString LicenseManager::getCustomerName() const
{
    return m_customerName;
}

QString LicenseManager::getDeviceCode() const
{
    return m_deviceCode;
}

QString LicenseManager::getConfigFilePath() const
{
    return m_settings ? m_settings->fileName() : QString();
}

void LicenseManager::exportLicenseInfo() const
{
    //QString exportPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/license_info.txt";
    QString exportPath = "./license_info.txt";
    QFile file(exportPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "License Information Export\n";
        out << "==========================\n";
        out << "Customer Name: " << m_customerName << "\n";
        out << "Device Code: " << m_deviceCode << "\n";
        out << "Check Device Code: "<< m_deviceCodeCheck << "\n";
        out << "Installation Date: " << m_installDate.toString("yyyy-MM-dd") << "\n";
        out << "Generate Date:" << m_generateDate.toString("yyyy-MM-dd") << "\n";
        out << "License Type: " << (m_licenseType == Permanent ? "Permanent" : "Trial") << "\n";
        if (m_licenseType == Trial)
        {
            out << "Expiration Date: " << m_expireDate.toString("yyyy-MM-dd") << "\n";
            out << "Remaining Days: " << calculateRemainingDays() << "\n";
        }
        out << "Encryption Key: " << m_encryptedText << "\n";

        file.close();
        //QMessageBox::information(nullptr, tr("Export Successful"),
        //    QString(tr("License information exported to:\n%1")).arg(exportPath));
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Export Successful"));
        msgBox.setText(QString(tr("License information exported to:\n%1")).arg(exportPath));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStyleSheet(m_styleSheet);
        msgBox.exec();
    }
}

bool LicenseManager::loadLicenseFile(const QString& filePath)
{
    QString actualFilePath = filePath.isEmpty() ? getDefaultLicenseFilePath() : filePath;

    QFile file(actualFilePath);
    if (!file.exists()) {
        qDebug() << "License file does not exist:" << actualFilePath;
        //QMessageBox msgBox;
        //msgBox.setWindowTitle(tr("Export Successful"));
        //msgBox.setText(QString(tr("License information exported to:\n%1")).arg(exportPath));
        //msgBox.setIcon(QMessageBox::Information);
        //msgBox.setStyleSheet(m_styleSheet);
        //msgBox.exec();
        return false;
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString licenseContent = in.readAll();
        file.close();

        bool success = parseLicenseFileContent(licenseContent);
        if (success) {
            qDebug() << "License file loaded successfully:" << actualFilePath;
            //saveLicenseInfo(); // 保存密钥各项内容到授权信息中
        }
        else {
            qDebug() << "Failed to parse license file:" << actualFilePath;
        }

        return success;
    }
    else {
        qDebug() << "Failed to open license file:" << actualFilePath;
        return false;
    }
}

QString LicenseManager::getDefaultLicenseFilePath() const
{
    return /*QCoreApplication::applicationDirPath() + */"./license.lic";
}

bool LicenseManager::isAntiTamperFileExists() const
{
    return QFile::exists(m_antiTamperFilePath);
}

bool LicenseManager::isLicenseFileExists(const QString& filePath) const
{
    QString actualFilePath = filePath.isEmpty() ? getDefaultLicenseFilePath() : filePath;
    return QFile::exists(actualFilePath);
}

bool LicenseManager::validateLicenseInfo()
{
    qDebug() << "validateLicenseInfo";
    QString tmpLicenseKey;
    if (m_customerName.isEmpty() || m_deviceCode.isEmpty())
    {
        return false;
    }

    // 检查机器码是否匹配
    if (m_deviceCodeCheck != m_deviceCode && !m_deviceCodeCheck.isEmpty()) {
        qWarning() << "Device code mismatch! Saved:" << m_deviceCodeCheck << "Current:" << m_deviceCode;
        return false;
    }
    //试用授权的授权检查
    if (m_licenseType == LicenseType::Trial)
    {
        //1. 检查授权密钥格式和内容是否一致
        tmpLicenseKey = QString("TRIAL-%1-%2-%3")
            .arg(m_customerName)
            .arg(m_deviceCode)
            .arg(m_expireDate.toString("yyyyMMdd"));
        if (tmpLicenseKey != m_licenseKey)
        {
            return false;
        }

        //2. 检查授权密钥有效期是否已过
        int remainingDays = calculateRemainingDays();
        return remainingDays >= 0;
    }
    // 永久授权的授权检查
    else
    {
        //1. 检查授权密钥格式和内容是否一致
        tmpLicenseKey = QString("PERMANENT-%1-%2")
            .arg(LicenseManager::getInstance()->m_customerName)
            .arg(LicenseManager::getInstance()->m_deviceCode);
        if (tmpLicenseKey != m_licenseKey)
        {
            return false;
        }
    }
    return true; // Permanent license is always valid
}

bool LicenseManager::importRegistrationFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.exists()) {
        qDebug() << "Registration file does not exist:" << filePath;
        return false;
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString encryptedContent = in.readAll();
        file.close();

        // 解密内容
        QString decryptedContent = CryptoUtils::decrypt(encryptedContent, m_encryptionKey);
        QJsonDocument doc = QJsonDocument::fromJson(decryptedContent.toUtf8());

        if (doc.isNull()) {
            qDebug() << "Failed to parse registration file JSON";
            return false;
        }

        QJsonObject regObj = doc.object();

        // 验证必需字段
        if (!regObj.contains("customerName") || !regObj.contains("licenseKey") ||
            !regObj.contains("licenseType")) {
            qDebug() << "Missing required fields in registration file";
            return false;
        }

        QString customerName = regObj["customerName"].toString();
        QString licenseKey = regObj["licenseKey"].toString();
        LicenseType licenseType = static_cast<LicenseType>(regObj["licenseType"].toInt());

        // 验证客户名称（可选，如果提供则验证）
        if (!customerName.isEmpty() && customerName != m_customerName) {
            qDebug() << "Customer name mismatch in registration file";
            //QMessageBox::warning(nullptr, tr("Import Failed"),
            //    tr("Registration file is for customer '%1', but current customer is '%2'")
            //    .arg(customerName).arg(m_customerName));
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Import Failed"));
            msgBox.setText(QString(tr("Registration file is for customer '%1', but current customer is '%2'"))
                .arg(customerName).arg(m_customerName));
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStyleSheet(m_styleSheet);
            msgBox.exec();
            return false;
        }

        // 使用验证密钥的方法来应用授权
        if (validateLicenseKey(licenseKey)) {
            qDebug() << "Registration file imported successfully";

            // 保存授权信息
            saveLicenseInfo();

            //QMessageBox::information(nullptr, tr("Import Successful"),
            //    tr("Registration file imported successfully!\n"
            //        "License type: %1\n"
            //        "Customer: %2")
            //    .arg(licenseType == Trial ? tr("Trial") : tr("Permanent"))
            //    .arg(m_customerName));

            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Import Successful"));
            msgBox.setText(QString(tr("Registration file imported successfully!\n"
                "License type: %1\n"
                "Customer: %2"))
                .arg(licenseType == Trial ? tr("Trial") : tr("Permanent"))
                .arg(m_customerName));
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setStyleSheet(m_styleSheet);
            msgBox.exec();
            return true;
        }
        else {
            qDebug() << "Invalid license key in registration file";
            return false;
        }
    }

    qDebug() << "Failed to open registration file:" << filePath;
    return false;
}

bool LicenseManager::validateRegistrationFile(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.exists()) {
        return false;
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString encryptedContent = in.readAll();
        file.close();

        // 解密内容
        QString decryptedContent = CryptoUtils::decrypt(encryptedContent, m_encryptionKey);
        QJsonDocument doc = QJsonDocument::fromJson(decryptedContent.toUtf8());

        if (doc.isNull()) {
            return false;
        }

        QJsonObject regObj = doc.object();

        // 验证必需字段
        if (!regObj.contains("customerName") || !regObj.contains("licenseKey") ||
            !regObj.contains("licenseType")) {
            return false;
        }

        // 验证客户名称
        QString customerName = regObj["customerName"].toString();
        if (!customerName.isEmpty() && customerName != m_customerName) {
            qDebug() << "Customer name mismatch in registration file";
            return false;
        }

        // 验证许可证密钥格式（不实际应用授权）
        QString licenseKey = regObj["licenseKey"].toString();
        QString decryptedKey = CryptoUtils::decrypt(licenseKey, m_encryptionKey);

        // 检查试用版密钥格式
        LicenseType licenseType = static_cast<LicenseType>(regObj["licenseType"].toInt());
        if (licenseType == Trial) {
            if (decryptedKey.startsWith("TRIAL-")) {
                QStringList parts = decryptedKey.split("-");
                if (parts.size() >= 4) {
                    QString customer = parts[1];
                    QString expireDateStr = parts[2];

                    // 验证客户名称
                    if (customer != m_customerName) {
                        return false;
                    }

                    // 验证过期日期
                    QDate expireDate = QDate::fromString(expireDateStr, "yyyyMMdd");
                    if (!expireDate.isValid() || expireDate < QDate::currentDate()) {
                        return false;
                    }

                    return true;
                }
            }
        }
        // 检查永久版密钥格式
        else if (licenseType == Permanent) {
            if (decryptedKey.startsWith("PERMANENT-")) {
                QStringList parts = decryptedKey.split("-");
                if (parts.size() >= 3) {
                    QString customer = parts[1];
                    if (customer != m_customerName) {
                        return false;
                    }
                    return true;
                }
            }
        }

        return false;
    }

    return false;
}

QString LicenseManager::getRegistrationFileInfo(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.exists()) {
        return tr("Registration file does not exist: %1").arg(filePath);
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString encryptedContent = in.readAll();
        file.close();

        // 解密内容
        QString decryptedContent = CryptoUtils::decrypt(encryptedContent, m_encryptionKey);
        QJsonDocument doc = QJsonDocument::fromJson(decryptedContent.toUtf8());

        if (doc.isNull()) {
            return tr("Invalid registration file format");
        }

        QJsonObject regObj = doc.object();

        QString info;
        info += tr("Registration File Information\n");
        info += tr("=============================\n");

        if (regObj.contains("softwareName")) {
            info += tr("Software: %1\n").arg(regObj["softwareName"].toString());
        }

        if (regObj.contains("softwareVersion")) {
            info += tr("Version: %1\n").arg(regObj["softwareVersion"].toString());
        }

        if (regObj.contains("customerName")) {
            info += tr("Customer: %1\n").arg(regObj["customerName"].toString());
        }

        if (regObj.contains("licenseType")) {
            LicenseType type = static_cast<LicenseType>(regObj["licenseType"].toInt());
            info += tr("License Type: %1\n").arg(type == Trial ? tr("Trial") : tr("Permanent"));
        }

        if (regObj.contains("expireDate")) {
            QString expireDateStr = regObj["expireDate"].toString();
            if (expireDateStr != "Permanent") {
                QDate expireDate = QDate::fromString(expireDateStr, "yyyy-MM-dd");
                info += tr("Expire Date: %1\n").arg(expireDate.toString("yyyy-MM-dd"));

                int remainingDays = QDate::currentDate().daysTo(expireDate);
                info += tr("Remaining Days: %1\n").arg(remainingDays);
            }
            else {
                info += tr("Expire Date: Permanent\n");
            }
        }

        if (regObj.contains("generateDate")) {
            info += tr("Generated: %1\n").arg(regObj["generateDate"].toString());
        }

        if (regObj.contains("systemInfo")) {
            info += tr("System: %1\n").arg(regObj["systemInfo"].toString());
        }

        // 验证状态
        bool isValid = validateRegistrationFile(filePath);
        info += tr("\nValidation: %1\n").arg(isValid ? tr("Valid") : tr("Invalid"));

        return info;
    }

    return tr("Failed to read registration file: %1").arg(filePath);
}

bool LicenseManager::parseLicenseFileContent(const QString& content)
{
    try {
        QString decryptedContent = CryptoUtils::decrypt(content, m_encryptionKey);
        QJsonDocument doc = QJsonDocument::fromJson(decryptedContent.toUtf8());

        if (doc.isNull()) {
            qDebug() << "Failed to parse license file JSON";
            return false;
        }

        QJsonObject licenseObj = doc.object();

        QString customerName = licenseObj["customerName"].toString();
        QString deviceCode = licenseObj["deviceCode"].toString();
        LicenseType licenseType = static_cast<LicenseType>(licenseObj["licenseType"].toInt());
        bool isLicensed = licenseObj["isLicensed"].toBool();
        QDate installDate = QDate::fromString(licenseObj["installDate"].toString(), "yyyy-MM-dd");
        QDate generateDate = QDate::fromString(licenseObj["generatedDate"].toString(), "yyyy-MM-dd");
        QDate expireDate = QDate::fromString(licenseObj["expireDate"].toString(), "yyyy-MM-dd");
        QString licenseKey = CryptoUtils::decrypt(licenseObj["licenseKey"].toString(), m_encryptionKey);
        // 验证数据有效性
        if (customerName.isEmpty() || !installDate.isValid() || !expireDate.isValid() || licenseKey.isEmpty()) {
            qDebug() << "Invalid data in license file";
            return false;
        }

        // 更新许可证信息
        m_customerName = customerName;
        m_deviceCode = deviceCode;
        m_licenseType = licenseType;
        m_isLicensed = isLicensed;
        m_installDate = installDate;
        m_generateDate = generateDate;
        m_expireDate = expireDate;
        m_licenseKey = licenseKey;
        m_encryptedText = content;
        return true;
    }
    catch (...) {
        qDebug() << "Exception occurred while parsing license file";
        return false;
    }
}

void LicenseManager::saveLicenseInfo()
{
    if (!m_settings) return;

    m_settings->setValue("License/InstallDate", m_installDate.toString("yyyy-MM-dd"));
    m_settings->setValue("License/ExpireDate", m_expireDate.toString("yyyy-MM-dd"));
    m_settings->setValue("License/LicenseType", static_cast<int>(m_licenseType));
    m_settings->setValue("License/IsLicensed", m_isLicensed);
    m_settings->setValue("License/LicenseKey", m_licenseKey);
    m_settings->setValue("License/CustomerName", m_customerName);
    m_settings->setValue("License/ReminderDays", m_reminderDays);
    m_settings->sync();
}

void LicenseManager::loadLicenseInfo()
{
    if (!m_settings) return;

    m_installDate = QDate::fromString(
        m_settings->value("License/InstallDate").toString(), "yyyy-MM-dd");
    m_expireDate = QDate::fromString(
        m_settings->value("License/ExpireDate").toString(), "yyyy-MM-dd");
    m_licenseType = static_cast<LicenseType>(
        m_settings->value("License/LicenseType", Trial).toInt());
    m_isLicensed = m_settings->value("License/IsLicensed", false).toBool();
    m_licenseKey = m_settings->value("License/LicenseKey").toString();
    m_customerName = m_settings->value("License/CustomerName").toString();
    m_reminderDays = m_settings->value("License/ReminderDays", 15).toInt();
}

void LicenseManager::showReminderDialog(bool showReminder)
{
    int remainingDays = calculateRemainingDays();

    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(":/Logo/Logo/Logo-ico.ico"));
    msgBox.setWindowTitle(tr("Trial Period Reminder"));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(QString(tr("Software trial period remaining %1 days!\n"
        "Trial expiration date: %2\n"
        "Please obtain permanent license to continue using."))
        .arg(remainingDays)
        .arg(m_expireDate.toString(tr("yyyy-MM-dd"))));

    msgBox.addButton(tr("Enter License Key"), QMessageBox::ActionRole);
    msgBox.addButton(tr("OK"), QMessageBox::AcceptRole);
    msgBox.setStyleSheet(m_styleSheet);

    msgBox.exec();

    if (msgBox.buttonRole(msgBox.clickedButton()) == QMessageBox::ActionRole) {
        showKeyInputDialog(showReminder);
    }
}

void LicenseManager::showKeyInputDialog(bool showReminder)
{
    QInputDialog dialog;  // 设置 parent
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint); // 移除问号
    dialog.setOptions(QInputDialog::UsePlainTextEditForTextInput); // 启用多行输入（关键！）
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setWindowTitle(tr("Software Authorization."));
    dialog.setLabelText(tr("Please enter license key:"));
    dialog.setTextValue(""); // 初始文本
    dialog.setStyleSheet(m_styleSheet);
    // 执行对话框
    bool ok = (dialog.exec() == QDialog::Accepted);
    QString key = ok ? dialog.textValue() : QString();

    if (ok)
    {
        if (!key.isEmpty() && validateLicenseKey(key.trimmed()))
        {
            return;
        }
        else
        {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Authorization Failed"));
            msgBox.setText(QString(tr("Invalid license key, please re-enter!")));
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStyleSheet(m_styleSheet);
            msgBox.exec();

            showKeyInputDialog(showReminder);
        }

    }
    else if (!ok)
    {
        if (showReminder)
        {
            showReminderDialog(showReminder);
        }
        else
        {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Authorization Prompt"));
            msgBox.setText(QString(tr("You must enter a valid license key to continue using the software!")));
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStyleSheet(m_styleSheet);
            msgBox.exec();

            showKeyInputDialog(showReminder);
        }
    }
}

int LicenseManager::calculateRemainingDays() const
{
    if (m_isLicensed && m_licenseType == Permanent) {
        return 999999;
    }

    QDate currentDate = QDate::currentDate();
    int days = currentDate.daysTo(m_expireDate);
    return days; // Could be negative, indicating expired
}

bool LicenseManager::validateAntiTamperTime()
{
    QDateTime lastRecordedTime;
    QString loadedDeviceCode;
    bool isFileExists = loadAntiTamperTime(lastRecordedTime, loadedDeviceCode);
    QDateTime currentTime = QDateTime::currentDateTime();
    
    // 定义合理的时间范围
    const QDateTime MIN_REASONABLE_TIME = QDateTime(QDate(2025, 10, 1), QTime(0, 0), Qt::UTC);
    const int MAX_FUTURE_DAYS = 7; // 允许最多比当前时间快7天
    const QDateTime MAX_REASONABLE_TIME = currentTime.addDays(MAX_FUTURE_DAYS);

    // 首次运行：保存当前时间（包含机器码）
    if (!isFileExists || !lastRecordedTime.isValid()) {
        saveAntiTamperTime();
        return true;
    }

    //// 检查机器码是否匹配
    //if (loadedDeviceCode != m_deviceCode && !loadedDeviceCode.isEmpty()) {
    //    qWarning() << "Device code mismatch! Saved:" << loadedDeviceCode << "Current:" << m_deviceCode;
    //    return false;
    //}

    // 检查 lastRecordedTime 是否在合理范围内
    if (lastRecordedTime < MIN_REASONABLE_TIME || lastRecordedTime > MAX_REASONABLE_TIME) {
        qWarning() << "Anti-tamper time out of reasonable range! Possible tampering.";
        qWarning() << "Recorded:" << lastRecordedTime.toString("yyyyMMddHHmm")
            << "Current:" << currentTime.toString("yyyyMMddHHmm");
        return false;
    }

    qDebug() << "lastRecordedTime:" << lastRecordedTime.toString("yyyyMMddHHmm")
        << "loadedDeviceCode:" << loadedDeviceCode;

    // 检查时间回退
    if (currentTime < lastRecordedTime) {
        qDebug() << "System time rolled back! Current:" << currentTime << "Last:" << lastRecordedTime;
        return false;
    }

    // 每天更新时间记录
    if (currentTime.date() > lastRecordedTime.date()) {
        saveAntiTamperTime();
    }

    return true;
}

void LicenseManager::saveAntiTamperTime()
{
    QDateTime currentTime = QDateTime::currentDateTime();
    // Record to minute level
    QString timeStr = currentTime.toString("yyyyMMddHHmm");

    // 创建包含时间和机器码的JSON对象
    QJsonObject antiTamperObj;
    antiTamperObj["timestamp"] = timeStr;
    antiTamperObj["deviceCode"] = m_deviceCodeCheck;

    QJsonDocument doc(antiTamperObj);
    QString antiTamperContent = doc.toJson(QJsonDocument::Compact);

    QString encryptedContent = CryptoUtils::encrypt(antiTamperContent, m_encryptionKey);

    QFile file(m_antiTamperFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << encryptedContent;
        file.close();
        qDebug() << "Anti-tamper file saved with machine code:" << m_deviceCode;
    }
}

bool LicenseManager::loadAntiTamperTime(QDateTime& loadedLastTime, QString& loadedDeviceCode)
{
    QFile file(m_antiTamperFilePath);

    if (!file.exists()) {
        loadedLastTime = QDateTime();
        loadedDeviceCode = QString();
        return false;
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString encryptedContent = in.readAll();
        file.close();

        try {
            QString decryptedContent = CryptoUtils::decrypt(encryptedContent, m_encryptionKey);
            QJsonDocument doc = QJsonDocument::fromJson(decryptedContent.toUtf8());

            if (doc.isNull()) {
                qWarning() << "Failed to parse anti-tamper file JSON";
                loadedLastTime = QDateTime();
                loadedDeviceCode = QString();
                return false;
            }

            QJsonObject antiTamperObj = doc.object();

            // 提取设备码
            if (antiTamperObj.contains("deviceCode")) {
                loadedDeviceCode = antiTamperObj["deviceCode"].toString();
            }
            else {
                loadedDeviceCode = QString();
            }
            m_deviceCodeCheck = loadedDeviceCode;
            // 提取时间戳
            if (antiTamperObj.contains("timestamp")) {
                QString timeStr = antiTamperObj["timestamp"].toString();
                loadedLastTime = QDateTime::fromString(timeStr, "yyyyMMddHHmm");
                return true;
            }
            else {
                loadedLastTime = QDateTime();
                return false;
            }

        }
        catch (...) {
            qWarning() << "Exception occurred while parsing anti-tamper file";
            loadedLastTime = QDateTime();
            loadedDeviceCode = QString();
            return false;
        }
    }

    loadedLastTime = QDateTime();
    loadedDeviceCode = QString();
    return false;
}

bool LicenseManager::validateLicenseStatus()
{ 
    QString licenseFilePath = "./license.lic";
    //检查防篡改文件状态
    if (!validateAntiTamperTime())
    {
        //QMessageBox::critical(nullptr, tr("System Error"),
        //    tr("Abnormal operating system time detected, software cannot start normally!"));
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("System Error"));
        msgBox.setText(QString(tr("Abnormal operating system time detected, software cannot start normally!")));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStyleSheet(m_styleSheet);
        msgBox.exec();

        return false;
    }

    // 读取密钥并解码
    if (!loadLicenseFile(licenseFilePath)) {
        //QMessageBox::critical(nullptr, tr("License Error"),
        //    tr("License system initialization failed!"));
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("License Error"));
        msgBox.setText(QString(tr("License system initialization failed!")));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStyleSheet(m_styleSheet);
        msgBox.exec();

        return false;
    }

    // 验证授权信息
    if (!validateLicenseInfo()) {
        //QMessageBox::critical(nullptr, tr("License Error"),
        //    tr("License validation failed!\n"
        //    "The software cannot start."));
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("License Error"));
        msgBox.setText(QString(tr("License validation failed!\n"
            "The software cannot start.")));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStyleSheet(m_styleSheet);
        msgBox.exec();

        return false;
    }

    if (m_deviceCodeCheck.isEmpty())
    {
        m_deviceCodeCheck = m_deviceCode;
        saveAntiTamperTime();
    }

    checkTrialStatus(true);
}