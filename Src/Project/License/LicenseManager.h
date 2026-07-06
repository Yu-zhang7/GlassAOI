#ifndef LICENSEMANAGER_H
#define LICENSEMANAGER_H

#include <QObject>
#include <QDate>
#include <QDateTime>
#include <QSettings>
#include <QTimer>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>


class LicenseManager : public QObject
{
    Q_OBJECT

public:
    enum LicenseType {
        Trial,
        Permanent
    };

    // 单例获取方法
    static LicenseManager* getInstance(const QString& customerName = "DefaultCustomer");
    static void releaseInstance();

    // 验证许可证状态
    bool validateLicenseStatus();

    // 初始化许可证系统
    bool initialize();

    // 设置样式表
    void initStyleSheet();

    // 设置每日检查定时器
    void setupDailyTimer();

    // 检查密钥状态
    void checkTrialStatus(bool showReminder = false);

    // 验证并更新密钥
    bool validateLicenseKey(const QString& key);

    // 获取剩余天数
    int getRemainingDays() const;

    // 获取许可证类型
    LicenseType getLicenseType() const;

    // 是否已授权
    bool isLicensed() const;

    // 获取安装日期
    QDate getInstallDate() const;

    // 获取生成日期
    QDate getGenerateDate() const;

    // 获取截止日期
    QDate getExpireDate() const;

    // 获取客户名称
    QString getCustomerName() const;

    // 获取设备码
    QString getDeviceCode() const;

    // 获取配置文件路径
    QString getConfigFilePath() const;

    // 导出许可证信息
    void exportLicenseInfo() const;

    // 加载授权文件
    bool loadLicenseFile(const QString& filePath = QString());

    // 获取默认授权文件路径
    QString getDefaultLicenseFilePath() const;

    // 检查防篡改文件是否存在
    bool isAntiTamperFileExists() const;

    // 防篡改时间验证
    bool validateAntiTamperTime();

    // 检查授权文件是否存在
    bool isLicenseFileExists(const QString& filePath = QString()) const;

    // 验证授权信息
    bool validateLicenseInfo();

    // 从注册文件导入授权信息
    bool importRegistrationFile(const QString& filePath);

    // 验证注册文件的有效性
    bool validateRegistrationFile(const QString& filePath) const;

    // 获取注册文件信息（不导入）
    QString getRegistrationFileInfo(const QString& filePath) const;

signals:
    // 许可证状态变化信号 - 添加 const 限定符
    void licenseStatusChanged(LicenseManager::LicenseType type, int remainingDays) const;
    void trialStatusUpdated(int remainingDays, bool isExpired) const;
    
    // 许可证失效信号。用于停止检测并关闭程序
    void signal_licenseExpired() const;

private slots:
    // 每日检查定时器
    void onDailyCheck();

private:
    // 私有构造函数
    explicit LicenseManager(const QString& customerName = "DefaultCustomer", QObject* parent = nullptr);
    ~LicenseManager();

    // 禁用拷贝和赋值
    LicenseManager(const LicenseManager&) = delete;
    LicenseManager& operator=(const LicenseManager&) = delete;

    // 保存许可证信息
    void saveLicenseInfo();

    // 加载许可证信息
    void loadLicenseInfo();

    // 显示提醒对话框
    void showReminderDialog(bool showReminder = false);

    // 显示密钥输入对话框
    void showKeyInputDialog(bool showReminder = false);

    // 计算剩余天数
    int calculateRemainingDays() const;

    // 保存防篡改时间
    void saveAntiTamperTime();

    // 加载防篡改时间
    bool loadAntiTamperTime(QDateTime& loadedLastTime, QString& loadedDeviceCode);

    // 解析授权文件内容
    bool parseLicenseFileContent(const QString& content);

private:
    static LicenseManager* m_instance;  // 单例实例

    QSettings* m_settings;
    QTimer* m_dailyTimer;

    // 许可证信息
    QDate m_installDate;                //安装日期
    QDate m_generateDate;               //生成日期
    QDate m_expireDate;                 //试用到期日期
    LicenseType m_licenseType;          //许可证类型
    QString m_licenseKey;               //许可密钥
    bool m_isLicensed;                  //是否授权
    QString m_customerName;             //客户名称

    QString m_deviceCode;               //设备码(随机码)

    QString m_deviceCodeCheck;          //校验设备码
    // 加密密钥
    QString m_encryptionKey;
    //加密后的密文
    QString m_encryptedText;

    // 提醒天数
    int m_reminderDays;

    // 防篡改文件路径
    QString m_antiTamperFilePath;

    // 样式表
    QString m_styleSheet;

};

#endif // LICENSEMANAGER_H