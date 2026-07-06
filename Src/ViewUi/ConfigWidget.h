#pragma once

#include <QWidget>
#include <qlineedit.h>
#include <qstandarditemmodel.h>
#include <QButtonGroup>
#include "ui_ConfigWidget.h"
#include "ItemDelegate.h"	//自定义项。(表格项可编辑状态)
#include "CustomCheckBoxHeaderView.h"	//自定义表头

#include "Global.h"
/****************使用方法******************************************************************************/
//////int main() {
//////	ConfigManager& configMgr = ConfigManager::getInstance();
//////
//////	// 注册包含中文的配置项
//////	auto stringConfig = configMgr.registerConfig<std::string>("welcome_message", "你好，世界！");
//////
//////	// 导出配置（将保留中文字符）
//////	configMgr.exportToJsonFile("config.json");
//////
//////	// 导入配置（将正确读取中文字符）
//////	configMgr.importFromJsonFile("config.json");
//////
//////	// 验证中文字符
//////	std::string message;
//////	configMgr.getConfigValue("welcome_message", message);
//////	std::cout << "读取的中文消息: " << message << std::endl;
//////
//////	return 0;
//////}
/*********************************************************************************************************/

QT_BEGIN_NAMESPACE
namespace Ui { class ConfigWidgetClass; };
QT_END_NAMESPACE

class ConfigWidget : public QWidget
{
	Q_OBJECT

public:
	ConfigWidget(QWidget* parent = nullptr);
	~ConfigWidget();

private:
	Ui::ConfigWidgetClass* ui;

public:
	//void saveConfig();
	void SetCurrentRecipe();

	void Recipe_Init();

signals:
	void Signal_UpdateRecipe();

	void Signal_UpdateRecipeDisplay();

	void Signal_SetDefectTypeVisibility(DefectType type, bool visible);

	void Signal_SetDefectLevelVisibility(DefectLevel level, bool visible);

	/* 保存并应用系统配置信息 */
	void Signal_WriteSystemConfig();

	/* 更换程序界面语言 */
	void Signal_LanguageChanged(int languageIndex);

	/* 默认配方修改 */
	void Signal_DefaultRecipeChanged();

private slots:
	/* 保存当前配方的参数 */
	void Slot_Recipe_Save();

	/* 设置为默认配方 */
	void Slot_Recipe_SetDefaultRecipe();

	/* 创建配方 */
	void Slot_Recipe_CreateRecipe();

    /* 删除配方 */
    void Slot_Recipe_DeleteRecipe();

	/* 配方改名 */
	void Slot_Recipe_Rename();

	/* 配方导出 */
	void Slot_Recipe_SaveAs();

	/* 配方导入 */
	void Slot_Recipe_Import();

    /* 复制配方 */
    void Slot_Recipe_CopyAs();

    /* 设置显示的瑕疵等级 */
	void Slot_Recipe_SetMinorLevelDisplayState(bool state);
	void Slot_Recipe_SetMediumLevelDisplayStat(bool state);
	void Slot_Recipe_SetSeriousLevelDisplayStat(bool state);
	void Slot_Recipe_SetNormalLevelDisplayState(bool state);
	void Slot_Recipe_SetAreaErrorLevelDisplayState(bool state);
	void Slot_Recipe_SetAbnormalLevelDisplayState(bool state);

	/* TreeView内容变化槽函数 */
	void Slot_Recipe_DataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);

	/* 缺陷类型显示状态变更 槽函数 */
	void Slot_Recipe_DefectTypeCheckboxStateChanged(const QModelIndex& index, bool checked);

	void Slot_Recipe_UpdateDisplayFlagToRecipeConfig();

	/* 缺陷等级显示状态变更 槽函数 */
	void Slot_Recipe_DefectLevelCheckboxStateChanged(int section, bool checked);

	/* 配方列表项单击槽函数 */
	void Slot_Recipe_SelectedRecipeChanged(QListWidgetItem* item);

	/* 保存系统配置信息 */
	void Slot_System_WriteConfig();

	/* 编辑系统配置信息 */
	void Slot_System_EditConfig();

	/* 取消编辑系统配置信息 */
	void Slot_System_CancelEditConfig();


	/* 更新授权信息 */
	void Slot_System_SetLicenseInfo();

	/* 导出授权信息 */
	void Slot_System_ExportLicenseInfo();

	/* 关于授权信息 */
	void Slot_System_AboutLicense();

private:
	/* 初始化配方参数的表格视图 */
	void InitRecipeView();

	/* 显示当前配方参数 */
	void Recipe_DisplayParameter();

	/* 删除配方 */
	void Recipe_DeleteRecipe();

	/* 安全删除配方列表指定项 */
	void Recipe_DeleteItemSafely(int rowToDelete);

	/* 获得配方文件列表并展示 */
	void Recipe_RefreshConfigFileNameList();

	/* 显示默认参数 */
	void Recipe_ShowDefaultParameter();

	/* 瑕疵项填充到表格 */
	void Recipe_AddDefectRow(const RecipeConfig::DefectConfig& defect);

	/* 其他项填充到表格 */
	void Recipe_AddGlobalRow(const QString& labelShow, const QString& labelType, double value, double min, double max, double step, int decimals, const QString& unit = QString());

	/* 数据保存路径项填充到表格 */
	void Recipe_AddStringRow(const QString& label, const QString& labelType, const QString& value);

	/* 导入配方参数 */
	void Recipe_ImportParameter();

	/* 配方参数导出 */
	void Recipe_ExportParameter(const QString& filePath);

	/* 创建数值项 */
	QStandardItem* Recipe_CreateDoubleItem(double value, double min, double max, double step, int decimals);

	/* 动态合并单元格 */
	void Recipe_AdjustTableSpans(int defectCount);

	/* 检测配方名称是否存在 */
	bool Recipe_IsRecipeNameExists(const QString& name);

	/* 更新显示的瑕疵等级到默认配方中 */
	void Recipe_UpdateDisplayFlagToRecipeConfig(RecipeConfig::Config& recipe);


	/* 设置系统配置信息 */
	void System_SetSystemConfig();

	/* 显示系统配置参数到界面 */
	void System_ShowSystemConfig();

	/* 保存配方参数 */
	void Recipe_SaveParameter();

	/* 连接信号和槽函数 */
	void InitConnect();

	QStandardItemModel* m_model;
	ItemDelegate* m_delegate;
	CustomCheckBoxHeaderView* m_headerView;

	bool m_isInitOver = true;	//保留，计划转为预留给后续，进行初始化状态判断，提供给mainWindow。任何一个子模块初始化失败(除读图调试模式)，则不执行传感器发来的启动信号

	bool m_isRefreshView = false;

	int m_mergeValue = 10;

	RecipeConfig::Config m_EditRecipeConfig;

	std::string m_EditRecipeName;
};
