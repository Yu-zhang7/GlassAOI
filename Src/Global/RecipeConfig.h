// config.h
#ifndef RECIPE_CONFIG_H
#define RECIPE_CONFIG_H

#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QJsonDocument>
#include <QDebug>
#include <algorithm>
#include <QDir>
#include <QFileInfo>
#include <QTranslator>
#include <QChar>
#include "GlobalRecipe.h"



/********************* 示例 ********************************************
#include "RecipeConfig.h"
#include <QListWidget>
#include <QMessageBox>

// 1. 列出所有配置文件名
QString folder = "configs/";
auto names = RecipeConfig::Config::listFilenames(folder); // ["cfg1.json", "default.json"]
ui->listWidget->addItems(names);

// 2. 创建新配置
RecipeConfig::Config::create("configs/my_config.json");

// 3. 删除配置
RecipeConfig::Config::remove("configs/old.json");

// 4. 加载配置
RecipeConfig::Config config = Inspection::Config::load("configs/default.json");

// 直接访问 global 成员
QString savePath = config.global.dataSavePath;
double mappingScale = config.global.mappingScale;
int mergeDistance = config.global.mergeDistance;
double logoConfidence = config.global.logoConfidence;

qDebug() << "保存路径:" << savePath;
qDebug() << "映射比例:" << mappingScale;
qDebug() << "合并距离:" << mergeDistance;
qDebug() << "Logo置信度:" << logoConfidence;

// 5. 修改并保存
if (auto* p = config.findDefect("BUBBLE")) {
    p->majorThreshold = 0.8;
}
config.save(); // 自动保存到原路径

// 或另存为
config.saveAs("configs/backup.json");
*********************************************************************/

namespace RecipeConfig {

    // 前向声明
    struct Config;

    // ==================== 数据结构定义 ====================
    struct DefectConfig {
        int id;
        QString type;
        QString name;
        double confidence;
        QString thresholdUnit;
        double minorThreshold;          //轻微瑕疵
        double moderateThreshold;       //中等瑕疵
        double majorThreshold;          //严重瑕疵
        bool isDisplayed = true;

        QJsonObject toJson() const;
        static DefectConfig fromJson(const QJsonObject& obj);
    };

    struct GlobalConfig {
        QString dataSavePathName;
        QString dataSavePath;

        //20251118: 取消映射比例的使用
        //QString mappingScaleName;
        //double mappingScale = 0.4;

        QString mergeDistanceName;
        int mergeDistance = 33;

        // 神经网络置信度
         QString thresholdDetect_1_Name;
         double thresholdDetect_1 = 0.2;
         QString thresholdDetect_2_Name;
         double thresholdDetect_2 = 0.2;

         // 20260311 增加脏污和水渍的额外检测阈值
         QString grayDifference_Name;
         int grayDifference = 0;

        QJsonObject toJson() const;
        static GlobalConfig fromJson(const QJsonObject& obj);
    };

    // ==================== 主配置类 + 文件管理（一体化）====================
    struct Config {
        GlobalConfig global;
        QVector<DefectConfig> defects;

        // ========== 实例方法：对象自身的保存 ==========
        bool save() const {
            return saveAs(currentPath);
        }

        bool saveAs(const QString& filePath) const {
            QJsonObject root;
            root["global"] = global.toJson();
            QJsonArray defectsArray;
            for (const auto& defect : defects) {
                defectsArray.append(defect.toJson());
            }
            root["defects"] = defectsArray;

            QJsonDocument doc(root);
            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                qWarning() << "无法打开文件写入:" << filePath;
                return false;
            }
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();

            // 更新当前路径
            const_cast<Config*>(this)->currentPath = filePath;
            return true;
        }

        // ========== 实例方法：另存配方并补充缺失项 ==========
        static bool copyAs(const Config& sourceRecipe, const QString& targetPath) {
            // 加载源配置
            Config config = sourceRecipe;
            if (config.defects.isEmpty()) {
                qWarning() << "源配置文件无效或不存在。";
                return false;
            }

            QString unit = "mm";

            QString unit2 = QStringLiteral("mm") + QChar(0x00B2);
            for (int i = 0; i < defectTypes.size(); i++)
            {
                DefectType defectType = defectTypes[i];

                bool defectTypeExists = std::any_of(config.defects.begin(), config.defects.end(),
                    [defectType](const DefectConfig& defectConfig) {
                        return defectConfig.id == static_cast<int>(defectType);
                    });
                if (m_defectTypeUsability[defectType] && !defectTypeExists)
                {
                    QString unitStr;
                    if (defectType == DefectType::TYPE_SCRATCH || defectType == DefectType::TYPE_CALCULUS ||
                        defectType == DefectType::TYPE_BUBBLE  || defectType == DefectType::TYPE_TRADEMARK)
                    {
                        unitStr = unit;
                    }
                    else
                    {
                        unitStr = unit2;
                    }
                    config.addDefect({ i, DefectTypeToString(defectType), QCoreApplication::translate("Global", DefectTypeToString(defectType).toStdString().c_str()), 0.5, unitStr, 0.1, 1.0, 9999.0 });
                }
            }
   
            // 保存到目标路径
            return config.saveAs(targetPath);
        }

        // ========== 静态工厂方法：加载 ==========
        static Config load(const QString& filePath) {
            Config config;
            config.currentPath = filePath; // 设置路径

            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly)) {
                qWarning() << u8"无法打开配置文件:" << filePath;
                return config;
            }

            QByteArray data = file.readAll();
            file.close();

            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isNull() || !doc.isObject()) {
                qWarning() << u8"JSON 解析失败或不是对象";
                return config;
            }

            QJsonObject root = doc.object();

            if (root.contains("global") && root["global"].isObject()) {
                config.global = GlobalConfig::fromJson(root["global"].toObject());
            }

            if (root.contains("defects") && root["defects"].isArray()) {
                QJsonArray defectsArray = root["defects"].toArray();
                for (const auto& defectValue : defectsArray) {
                    if (defectValue.isObject()) {
                        config.defects.append(DefectConfig::fromJson(defectValue.toObject()));
                    }
                }
            }

            return config;
        }

        // ========== 静态文件管理方法 ==========
        /// 创建新配置文件（带默认值）
        static bool create(const QString& filePath) {
            QFileInfo info(filePath);
            if (info.suffix().toLower() != "json") {
                qWarning() << "文件扩展名应为 .json:" << filePath;
                return false;
            }

            if (QFile::exists(filePath)) {
                qWarning() << "文件已存在，无法创建:" << filePath;
                return false;
            }

            Config config;
            config.global.dataSavePathName = QCoreApplication::translate("Global", "filePath");
            config.global.dataSavePath = "D:/GlassInspection/SaveData";

            //20251118: 取消映射比例的使用
            //config.global.mappingScaleName = QCoreApplication::translate("Global", "mappingScale");
            //config.global.mappingScale = 0.4;

            config.global.mergeDistanceName = QCoreApplication::translate("Global", "mergeDistance");
            config.global.mergeDistance = 10;

            config.global.thresholdDetect_1_Name = QCoreApplication::translate("Global", "thresholdDetect_1");
            config.global.thresholdDetect_1 = 0.2;

            config.global.thresholdDetect_2_Name = QCoreApplication::translate("Global", "thresholdDetect_2");
            config.global.thresholdDetect_2 = 0.2;

            config.global.grayDifference_Name = QCoreApplication::translate("Global", "grayDifference");
            config.global.grayDifference = 0;

            /*
            POORCOATING,	
            SCRATCH,		
            CALCULUS,		
            BUBBLE,		
            TRADEMARK,		
            WATERSTAIN,	
            SMUDGE,		
            SCREENPRINTING	
            */

            QString unit = "mm";

            QString unit2 = QStringLiteral("mm") + QChar(0x00B2);
            //QString unit2 = "mm²"; 
            
            for (int i = 0; i < defectTypes.size(); i++)
            {
                if (m_defectTypeUsability[defectTypes[i]])
                {
                    QString unitStr;
                    if (defectTypes[i] == DefectType::TYPE_SCRATCH || defectTypes[i] == DefectType::TYPE_CALCULUS ||
                        defectTypes[i] == DefectType::TYPE_BUBBLE || defectTypes[i] == DefectType::TYPE_TRADEMARK)
                    {
                        unitStr = unit;
                    }
                    else
                    {
                        unitStr = unit2;
                    }
                    //config.addDefect({ i, DefectTypeToString(defectTypes[i]), QCoreApplication::translate("Global", DefectTypeToString(defectTypes[i]).toStdString().c_str()), 0.5, unitStr, 0.1, 1.0, 9999.0 });
                    config.addDefect({ i, DefectTypeToString(defectTypes[i]), QString(""), 0.5, unitStr, 0.1, 1.0, 9999.0 });
                }
            }

            return config.saveAs(filePath);
        }

        /// 删除配置文件
        static bool remove(const QString& filePath) {
            if (!QFile::exists(filePath)) {
                qWarning() << "文件不存在:" << filePath;
                return false;
            }

            if (QFileInfo(filePath).suffix().toLower() != "json") {
                qWarning() << "警告：尝试删除非 JSON 文件:" << filePath;
                return false;
            }

            QFile file(filePath);
            if (!file.remove()) {
                qWarning() << "删除失败:" << filePath << file.errorString();
                return false;
            }
            return true;
        }

        /// 获取指定文件夹下所有 .json 文件名（仅文件名）
        static QStringList listFilenames(const QString& folderPath) {
            QStringList result;
            QDir dir(folderPath);

            if (!dir.exists()) {
                qWarning() << "文件夹不存在:" << folderPath;
                return result;
            }

            QFileInfoList list = dir.entryInfoList(QStringList() << "*.json", QDir::Files, QDir::Name);
            for (const QFileInfo& file : list) {
                result.append(file.fileName());
            }
            return result;
        }

        /// 获取完整路径列表
        static QStringList listFilePaths(const QString& folderPath) {
            QStringList result;
            QDir dir(folderPath);
            if (!dir.exists()) return result;

            QFileInfoList list = dir.entryInfoList(QStringList() << "*.json", QDir::Files);
            for (const QFileInfo& file : list) {
                result.append(file.absoluteFilePath());
            }
            return result;
        }

        /// 检查是否为有效配置文件
        static bool isValid(const QString& filePath) {
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly)) return false;

            QByteArray data = file.readAll();
            file.close();

            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(data, &error);
            return (error.error == QJsonParseError::NoError) &&
                doc.isObject() && doc.object().contains("defects");
        }

        // ========== 辅助方法 ==========
        void addDefect(const DefectConfig& defect) {
            defects.append(defect);
        }

        void removeDefect(const QString& type) {
            auto newEnd = std::remove_if(defects.begin(), defects.end(),
                [&type](const DefectConfig& d) { return d.type == type; });
            defects.erase(newEnd, defects.end());
        }

        void removeDefect(const int& id) {
            auto newEnd = std::remove_if(defects.begin(), defects.end(),
                [&id](const DefectConfig& d) { return d.id == id; });
            defects.erase(newEnd, defects.end());
        }

        DefectConfig* findDefect(const QString& type) {
            for (auto& defect : defects) {
                if (defect.type == type) {
                    return &defect;
                }
            }
            return nullptr;
        }

        const DefectConfig* findDefect(const QString& type) const {
            for (const auto& defect : defects) {
                if (defect.type == type) {
                    return &defect;
                }
            }
            return nullptr;
        }

        // 获取当前文件路径
        QString getCurrentPath() const { return currentPath; }

    private:
        mutable QString currentPath; // 记录当前加载/保存的路径
    };

    // ============ 内联实现 ============
    inline QJsonObject DefectConfig::toJson() const {
        QJsonObject obj;
        obj["id"]                       = id;
        obj["type"]                     = type;
        obj["name"]                     = name;
        obj["confidence"]               = confidence;
        obj["thresholdUnit"]            = thresholdUnit;
        obj["minorThreshold"]           = minorThreshold;
        obj["moderateThreshold"]        = moderateThreshold;
        obj["majorThreshold"]           = majorThreshold;
        return obj;
    }

    inline DefectConfig DefectConfig::fromJson(const QJsonObject& obj) {
        DefectConfig config;
        config.id                       = obj["id"].toInt();   
        config.type                     = obj["type"].toString();
        config.name                     = obj["name"].toString();
        config.confidence               = obj["confidence"].toDouble(0.0);
        config.thresholdUnit            = obj["thresholdUnit"].toString("mm");
        config.minorThreshold           = obj["minorThreshold"].toDouble(0.0);
        config.moderateThreshold        = obj["moderateThreshold"].toDouble(0.0);
        config.majorThreshold           = obj["majorThreshold"].toDouble(0.0);
        return config;
    }

    inline QJsonObject GlobalConfig::toJson() const {
        QJsonObject obj;
        obj["dataSavePathName"]         = dataSavePathName;
        obj["dataSavePath"]             = dataSavePath;

        //20251118: 取消映射比例的使用
        //obj["mappingScaleName"]         = mappingScaleName;
        //obj["mappingScale"]             = mappingScale;

        obj["mergeDistanceName"]        = mergeDistanceName;
        obj["mergeDistance"]            = mergeDistance;

        obj["thresholdDetect_1_Name"]   = thresholdDetect_1_Name;
        obj["thresholdDetect_1"]        = thresholdDetect_1;

        obj["thresholdDetect_2_Name"]   = thresholdDetect_2_Name;
        obj["thresholdDetect_2"]        = thresholdDetect_2;

        obj["grayDifference_Name"]      = grayDifference_Name;
        obj["grayDifference"]           = grayDifference;

        return obj;
    }

    inline GlobalConfig GlobalConfig::fromJson(const QJsonObject& obj) {
        GlobalConfig config;
        // 字符串字段（带默认值）
        config.dataSavePathName         = obj.value("dataSavePathName").toString("数据保存路径");
        config.dataSavePath             = obj.value("dataSavePath").toString("./SaveData");

        //20251118: 取消映射比例的使用
        //config.mappingScaleName         = obj.value("mappingScaleName").toString("映射比例");
        //config.mappingScale             = obj.value("mappingScale").toDouble(0.4);

        config.mergeDistanceName        = obj.value("mergeDistanceName").toString("相近缺陷合并距离");
        config.mergeDistance            = obj.value("mergeDistance").toInt(33);

        config.thresholdDetect_1_Name   = obj.value("thresholdDetect_1_Name").toString("threshold_1_Name");
        config.thresholdDetect_1        = obj.value("thresholdDetect_1").toDouble(0.5);

        config.thresholdDetect_2_Name   = obj.value("thresholdDetect_2_Name").toString("threshold_2_Name");
        config.thresholdDetect_2        = obj.value("thresholdDetect_2").toDouble(0.2);

        config.grayDifference_Name      = obj.value("grayDifference_Name").toString("grayDifference_Name");
        config.grayDifference           = obj.value("grayDifference").toInt(0);

        return config;
    }

} // namespace RecipeConfig

#endif // RECIPE_CONFIG_H