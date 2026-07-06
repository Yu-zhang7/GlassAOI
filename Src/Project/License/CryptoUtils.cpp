#include "CryptoUtils.h"

QString CryptoUtils::simpleXorEncrypt(const QString& input, const QString& key)
{
    QByteArray data = input.toUtf8();
    QByteArray keyData = key.toUtf8();
    QByteArray result;

    for (int i = 0; i < data.size(); ++i) {
        result.append(data[i] ^ keyData[i % keyData.size()]);
    }

    return QString::fromLatin1(result.toHex());
}

QString CryptoUtils::simpleXorDecrypt(const QString& input, const QString& key)
{
    QByteArray data = QByteArray::fromHex(input.toLatin1());
    QByteArray keyData = key.toUtf8();
    QByteArray result;

    for (int i = 0; i < data.size(); ++i) {
        result.append(data[i] ^ keyData[i % keyData.size()]);
    }

    return QString::fromUtf8(result);
}

QString CryptoUtils::base64Encode(const QString& input)
{
    return input.toUtf8().toBase64();
}

QString CryptoUtils::base64Decode(const QString& input)
{
    return QByteArray::fromBase64(input.toUtf8());
}

QString CryptoUtils::encrypt(const QString& input, const QString& key)
{
    QString xorEncrypted = simpleXorEncrypt(input, key);
    return base64Encode(xorEncrypted);
}

QString CryptoUtils::decrypt(const QString& input, const QString& key)
{
    QString base64Decoded = base64Decode(input);
    return simpleXorDecrypt(base64Decoded, key);
}