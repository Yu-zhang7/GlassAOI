#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <string>
#include <iostream>
#include <map>
#include <functional>
#include <fstream>
#include <memory>
#include <any>
#include <typeindex>
#include <vector>
#include "json.h"

// 配置项基类
class ConfigBase
{
public:
    virtual ~ConfigBase() = default;

    // 必须由派生类实现的接口
    virtual void exportToJson(Json::Value& root) const = 0;
    virtual bool importFromJson(const Json::Value& root) = 0;
    virtual std::type_index getType() const = 0;
    virtual void* getValuePtr() = 0;
};

// 类型化的配置项
template <typename T>
class Config : public ConfigBase
{
public:
    using CallbackFunc = std::function<void(const T&)>;
    using ToJsonConverter = std::function<Json::Value(const T&)>;
    using FromJsonConverter = std::function<bool(const Json::Value&, T&)>;

    Config(const std::string& key, const T& defaultValue = T(),
        ToJsonConverter toJson = nullptr,
        FromJsonConverter fromJson = nullptr)
        : m_key(key), m_value(defaultValue),
        m_toJsonConverter(std::move(toJson)),
        m_fromJsonConverter(std::move(fromJson))
    {
    }

    // 设置值并触发回调
    void setValue(const T& value)
    {
        static_assert(!std::is_array_v<T>, "Array type is not supported");
        if (m_value != value)
        {
            m_value = value;
            if (m_callback)
            {
                m_callback(m_value);
            }
        }
    }

    // 获取值
    const T& getValue() const { return m_value; }

    // 设置回调函数
    void setCallback(CallbackFunc callback)
    {
        m_callback = std::move(callback);
    }

    // 实现基类接口
    void exportToJson(Json::Value& root) const override
    {
        root[m_key] = convertToJsonValue(m_value);
    }

    bool importFromJson(const Json::Value& root) override
    {
        if (!root.isMember(m_key))
        {
            return false;
        }

        try
        {
            T newValue;
            if (convertFromJsonValue(root[m_key], newValue))
            {
                setValue(newValue);
                return true;
            }
            return false;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error importing config '" << m_key << "': " << e.what() << std::endl;
            return false;
        }
        catch (...)
        {
            std::cerr << "Unknown error importing config '" << m_key << "'" << std::endl;
            return false;
        }
    }

    std::type_index getType() const override
    {
        return std::type_index(typeid(T));
    }

    void* getValuePtr() override
    {
        return &m_value;
    }

private:
    // 将T类型转换为Json::Value的辅助函数
    Json::Value convertToJsonValue(const T& value) const
    {
        if constexpr (std::is_same_v<T, int> ||
            std::is_same_v<T, unsigned int> ||
            std::is_same_v<T, int64_t> ||
            std::is_same_v<T, uint64_t> ||
            std::is_same_v<T, bool>)
        {
            return Json::Value(static_cast<Json::Int64>(value));
        }
        else if constexpr (std::is_same_v<T, double> ||
            std::is_same_v<T, float>)
        {
            return Json::Value(static_cast<double>(value));
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            return Json::Value(value);
        }
        else
        {
            // 对于自定义类型，使用用户提供的转换器
            if (m_toJsonConverter) {
                return m_toJsonConverter(value);
            }
            // 没有转换器时返回null
            return Json::Value();
        }
    }

    // 从Json::Value转换为T类型的辅助函数
    bool convertFromJsonValue(const Json::Value& value, T& outValue) const
    {
        if constexpr (std::is_same_v<T, int>)
        {
            if (value.isInt()) {
                outValue = value.asInt();
                return true;
            }
            return false;
        }
        else if constexpr (std::is_same_v<T, unsigned int>)
        {
            if (value.isUInt()) {
                outValue = value.asUInt();
                return true;
            }
            return false;
        }
        else if constexpr (std::is_same_v<T, int64_t>)
        {
            if (value.isInt64()) {
                outValue = value.asInt64();
                return true;
            }
            return false;
        }
        else if constexpr (std::is_same_v<T, uint64_t>)
        {
            if (value.isUInt64()) {
                outValue = value.asUInt64();
                return true;
            }
            return false;
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            if (value.isDouble()) {
                outValue = value.asDouble();
                return true;
            }
            return false;
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            if (value.isDouble()) {
                outValue = static_cast<float>(value.asDouble());
                return true;
            }
            return false;
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            if (value.isBool()) {
                outValue = value.asBool();
                return true;
            }
            return false;
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            if (value.isString()) {
                outValue = value.asString();
                return true;
            }
            return false;
        }
        else
        {
            // 对于自定义类型，使用用户提供的转换器
            if (m_fromJsonConverter) {
                return m_fromJsonConverter(value, outValue);
            }
            return false;
        }
    }

private:
    std::string     m_key;
    T               m_value;
    CallbackFunc    m_callback;
    ToJsonConverter m_toJsonConverter;
    FromJsonConverter m_fromJsonConverter;
};

// 配置管理器 - 单例
class ConfigManager
{
public:
    // 获取单例实例
    static ConfigManager& getInstance()
    {
        static ConfigManager instance;
        return instance;
    }

    // 注册配置项（支持自定义类型转换器）
    template <typename T>
    std::shared_ptr<Config<T>> registerConfig(
        const std::string& key,
        const T& defaultValue = T(),
        typename Config<T>::ToJsonConverter toJson = nullptr,
        typename Config<T>::FromJsonConverter fromJson = nullptr)
    {
        auto it = m_configs.find(key);
        if (it != m_configs.end())
        {
            // 尝试转换为正确的类型
            auto config = std::dynamic_pointer_cast<Config<T>>(it->second);
            if (config)
            {
                return config;
            }
            // 类型不匹配，移除旧配置
            std::cerr << "Type mismatch for config '" << key
                << "'. Removing old configuration." << std::endl;
            m_configs.erase(it);
        }

        // 创建新配置
        auto config = std::make_shared<Config<T>>(key, defaultValue, toJson, fromJson);
        m_configs[key] = config;
        return config;
    }

    // 检查配置是否存在
    bool hasConfig(const std::string& key) const
    {
        return m_configs.find(key) != m_configs.end();
    }

    // 获取已经注册的配置
    template <typename T>
    std::shared_ptr<Config<T>> getConfig(const std::string& key)
    {
        auto it = m_configs.find(key);
        if (it != m_configs.end())
        {
            return std::dynamic_pointer_cast<Config<T>>(it->second);
        }
        return nullptr;
    }

    // 设置配置值
    template <typename T>
    bool setConfigValue(const std::string& key, const T& value)
    {
        auto config = getConfig<T>(key);
        if (config)
        {
            config->setValue(value);
            return true;
        }

        // 自动注册配置项如果不存在
        registerConfig(key, value);
        return true;
    }

    // 获取配置值
    template <typename T>
    bool getConfigValue(const std::string& key, T& outValue)
    {
        auto config = getConfig<T>(key);
        if (config)
        {
            outValue = config->getValue();
            return true;
        }
        return false;
    }

    // 导出所有配置到JSON文件（确保保留中文字符）
    bool exportToJsonFile(const std::string& filename)
    {
        Json::Value root;

        for (const auto& entry : m_configs)
        {
            entry.second->exportToJson(root);
        }

        std::ofstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return false;
        }

        Json::StreamWriterBuilder writer;
        // 使用UTF-8编码并保留原始字符串
        writer["emitUTF8"] = true;         // 输出UTF-8字符串
        writer["indentation"] = "    ";     // 4空格缩进
        writer["commentStyle"] = "None";    // 不输出注释
        writer["dropNullPlaceholders"] = false; // 保留null值
        writer["useSpecialFloats"] = false; // 不使用特殊浮点值

        // 创建JSON写入器
        std::unique_ptr<Json::StreamWriter> jsonWriter(writer.newStreamWriter());

        // 写入文件
        jsonWriter->write(root, &file);
        file.close();

        return true;
    }

    // 从JSON文件导入配置
    bool importFromJsonFile(const std::string& filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "Failed to open config file: " << filename << std::endl;
            return false;
        }

        Json::Value root;
        Json::CharReaderBuilder readerBuilder;
        std::string errs;

        // 配置读取器以支持UTF-8
        readerBuilder["collectComments"] = false; // 忽略注释
        readerBuilder["failIfExtra"] = false;     // 允许额外字段
        readerBuilder["rejectDupKeys"] = false;   // 允许重复键
        readerBuilder["allowSpecialFloats"] = false; // 不允许特殊浮点数

        if (!Json::parseFromStream(readerBuilder, file, &root, &errs))
        {
            std::cerr << "Failed to parse JSON: " << errs << std::endl;
            file.close();
            return false;
        }

        file.close();

        bool allSuccess = true;
        for (auto& entry : m_configs)
        {
            if (!entry.second->importFromJson(root))
            {
                std::cerr << "Failed to import config for key: " << entry.first << std::endl;
                allSuccess = false;
            }
        }

        // 注册文件中存在但尚未注册的配置项
        for (const auto& key : root.getMemberNames())
        {
            if (m_configs.find(key) == m_configs.end())
            {
                const Json::Value& value = root[key];
                bool registered = false;

                if (value.isInt()) {
                    auto config = registerConfig(key, value.asInt());
                    registered = true;
                }
                else if (value.isUInt()) {
                    auto config = registerConfig(key, value.asUInt());
                    registered = true;
                }
                else if (value.isInt64()) {
                    auto config = registerConfig(key, value.asInt64());
                    registered = true;
                }
                else if (value.isUInt64()) {
                    auto config = registerConfig(key, value.asUInt64());
                    registered = true;
                }
                else if (value.isDouble()) {
                    auto config = registerConfig(key, value.asDouble());
                    registered = true;
                }
                else if (value.isBool()) {
                    auto config = registerConfig(key, value.asBool());
                    registered = true;
                }
                else if (value.isString()) {
                    auto config = registerConfig(key, value.asString());
                    registered = true;
                }
                else if (value.isObject()) {
                    std::cerr << "Dynamic registration of object types not supported for key: " << key << std::endl;
                }

                if (registered) {
                    std::cout << "Dynamically registered config: " << key << std::endl;
                }
                else {
                    std::cerr << "Unsupported JSON value type for key: " << key << std::endl;
                    allSuccess = false;
                }
            }
        }

        return allSuccess;
    }

    // 移除配置
    bool removeConfig(const std::string& key)
    {
        auto it = m_configs.find(key);
        if (it != m_configs.end())
        {
            m_configs.erase(it);
            return true;
        }
        return false;
    }

    // 获取所有配置键
    std::vector<std::string> getAllConfigKeys() const
    {
        std::vector<std::string> keys;
        for (const auto& pair : m_configs) {
            keys.push_back(pair.first);
        }
        return keys;
    }

private:
    // 私有构造函数确保单例
    ConfigManager() = default;
    ~ConfigManager() = default;

    // 禁止复制和移动
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;

    // 存储所有配置
    std::map<std::string, std::shared_ptr<ConfigBase>> m_configs;
};

#endif