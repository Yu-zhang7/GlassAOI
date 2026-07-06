// crypto_utils.h
#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <string>

class __declspec(dllexport) CryptoUtils
{
public:
    static std::string getEncryptionKey();

    static std::string simpleXorEncrypt(const std::string& input, const std::string& key);

    static std::string simpleXorDecrypt(const std::string& input, const std::string& key);

    static std::string base64Encode(const std::string& input);

    static std::string base64Decode(const std::string& input);

    static std::string encrypt(const std::string& input, const std::string& key);

    static std::string decrypt(const std::string& input, const std::string& key);

private:
    CryptoUtils() = delete;
    ~CryptoUtils() = delete;
};

#endif // CRYPTO_UTILS_H