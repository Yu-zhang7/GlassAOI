#ifndef CRYPTOUTILS_H
#define CRYPTOUTILS_H

#include <QString>
#include <QByteArray>

class CryptoUtils
{
public:
    // 简单的XOR加密
    static QString simpleXorEncrypt(const QString& input, const QString& key);
    static QString simpleXorDecrypt(const QString& input, const QString& key);

    // Base64编码解码
    static QString base64Encode(const QString& input);
    static QString base64Decode(const QString& input);

    // 组合加密
    static QString encrypt(const QString& input, const QString& key);
    static QString decrypt(const QString& input, const QString& key);

private:
    CryptoUtils() = delete;
    ~CryptoUtils() = delete;
};

#endif // CRYPTOUTILS_H