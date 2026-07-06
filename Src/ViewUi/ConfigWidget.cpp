#include <iostream>
#include <qregexp.h>
#include <qvalidator.h>
#include <qobject.h>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <QDebug>
#include <QSettings>
#include <QMessageBox>
#include <QInputDialog>
#include <unordered_map>

#include "ConfigWidget.h"

#include "ConfigManager.h"	//读写配置文件

#include "Log.hpp"

#include "LicenseManager.h"	//授权管理系统


ConfigWidget::ConfigWidget(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::ConfigWidgetClass())
	, m_model(nullptr)
{
	ui->setupUi(this);

    if (LicenseManager::getInstance()->getLicenseType() == LicenseManager::LicenseType::Permanent)
    {
        ui->pBtn_SetLicenseInfo->setVisible(false);
    }

	Recipe_Init();

	System_ShowSystemConfig();

	QList<QWidget*> widgets = ui->tab_System->findChildren<QWidget*>();
	for (QWidget* widget : widgets) {
		
		// 跳过某些特定控件
		if (widget->objectName() == "pBtn_EditSystemConfig"
			|| widget->objectName() == "pBtn_SaveSystemConfig"
			|| widget->objectName() == "pBtn_CancelSystemConfig"
			|| widget->objectName() == "pBtn_SetLicenseInfo"
			|| widget->objectName() == "pBtn_ExportLicenseInfo"
			|| widget->objectName() == "pBtn_AboutLicense")
		{
			continue;
		}
		widget->setEnabled(false);
	}
	ui->pBtn_CancelSystemConfig->setEnabled(false);
	ui->pBtn_SaveSystemConfig->setEnabled(false);
	ui->pBtn_EditSystemConfig->setEnabled(true);

	/* 连接信号和槽 */
	InitConnect();

	ui->tabWidget->setCurrentIndex(1);	//默认显示配方设置界面

	//20251118: 取消下述三项的显示
	ui->checkBox_normalLevel->setVisible(false);
	ui->checkBox_areaerrorLevel->setVisible(false);
	ui->checkBox_abnormalLevel->setVisible(false);
}

void ConfigWidget::SetCurrentRecipe()
{

	//FileLogPrintf("Recipe: SetCurrentRecipe. currentIndex=%d", CurrentRecipeIndex);
	//switch (CurrentRecipeIndex)
	//{
	//case 0:
	//	ui->radioBtn_Recipe1->setChecked(true);
	//	break;
	//case 1:
	//	ui->radioBtn_Recipe2->setChecked(true);
	//	break;
	//case 2:
	//	ui->radioBtn_Recipe3->setChecked(true);
	//	break;
	//}
}

void ConfigWidget::Recipe_Init()
{
	FILE_LOG_INFO("Recipe widget Init.");

	m_EditRecipeConfig = RecipeInfo;			//获得当前的配方信息

	m_EditRecipeName = DefaultRecipeName;		//获得当前默认配方名称

	ui->splitter_RecipeConfig->setSizes({ 300, 700 });	//设定splitter左右两侧的默认宽度占比

	/* 刷新配方名称列表*/
	Recipe_RefreshConfigFileNameList();

	/* 初始化配方参数的表格视图 */
	InitRecipeView();

	/* 显示当前默认配方参数 */
	Recipe_ShowDefaultParameter();


	//默认展开所有节点
	//ui->TableView_Recipe->expandAll();

	ui->checkBox_minorLevel->setChecked(DISPLAY_MINOR);
	ui->checkBox_mediumLevel->setChecked(DISPLAY_MEDIUM);
	ui->checkBox_seriousLevel->setChecked(DISPLAY_SERIOUS);
	ui->checkBox_normalLevel->setChecked(DISPLAY_NORMAL);
	ui->checkBox_areaerrorLevel->setChecked(DISPLAY_AREAERROR);
	ui->checkBox_abnormalLevel->setChecked(DISPLAY_ABNORMAL);

}

void ConfigWidget::InitRecipeView()
{ 	
	//设置model,并设置标题
	m_model = new QStandardItemModel(this);
	ui->TableView_Recipe->setModel(m_model);
	//设置代理和委托
	m_delegate = new ItemDelegate(this);
	ui->TableView_Recipe->setItemDelegate(m_delegate);

	int width = ui->TableView_Recipe->width() / 5;

	ui->TableView_Recipe->verticalHeader()->setDefaultSectionSize(23);
	ui->TableView_Recipe->verticalHeader()->hide();

	// 设置列头标签
	m_model->setHorizontalHeaderLabels({
		tr("Configuration"),
		tr("Unit"),
		tr("Confidence"),
		tr("Minor"),
		tr("Moderatet") ,
		tr("Major")
		});

	// 使用自定义表头
	m_headerView = new CustomCheckBoxHeaderView(Qt::Horizontal, ui->TableView_Recipe);
	ui->TableView_Recipe->setHorizontalHeader(m_headerView);

	// 只给需要的列添加复选框（例如第1、3、4列）
	//m_headerView->setCheckableSections({ 3, 4, 5 });

	// 先让列根据内容自适应
	ui->TableView_Recipe->resizeColumnsToContents();

	// 设置各列的宽度范围为100-300像素
	m_headerView->setSectionWidthRange(0, 180, 280);
	m_headerView->setSectionWidthRange(1, 70, 70);
	m_headerView->setSectionWidthRange(2, 100, 150);
	m_headerView->setSectionWidthRange(3, 150, 150);
	m_headerView->setSectionWidthRange(4, 150, 150);
	m_headerView->setSectionWidthRange(5, 150, 150);

	// 设置初始状态
	//m_headerView->setCheckState(3, Qt::Checked);
	//m_headerView->setCheckState(4, Qt::Checked);
	//m_headerView->setCheckState(5, Qt::Checked);

}

void ConfigWidget::InitConnect()
{

	auto buttonClick_int = static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked);

	/*************************** 系统设置 ********************************/
    connect(ui->pBtn_EditSystemConfig, &QPushButton::clicked, this, &ConfigWidget::Slot_System_EditConfig);
	connect(ui->pBtn_CancelSystemConfig, &QPushButton::clicked, this, &ConfigWidget::Slot_System_CancelEditConfig);

	connect(ui->pBtn_SaveSystemConfig, &QPushButton::clicked, this, &ConfigWidget::Slot_System_WriteConfig);
	/*********************************************************************/

	/*************************** 授权相关 ********************************/
	connect(ui->pBtn_SetLicenseInfo, &QPushButton::clicked, this, &ConfigWidget::Slot_System_SetLicenseInfo);
	connect(ui->pBtn_ExportLicenseInfo, &QPushButton::clicked, this, &ConfigWidget::Slot_System_ExportLicenseInfo);
	connect(ui->pBtn_AboutLicense, &QPushButton::clicked, this, &ConfigWidget::Slot_System_AboutLicense);
	/*********************************************************************/


	/*************************** 配方设置 ********************************/
	// 连接配方列表项点击信号
	connect(ui->listWidget_RecipeList, &QListWidget::itemClicked, this, &ConfigWidget::Slot_Recipe_SelectedRecipeChanged);

	// 连接配方参数改变信号
	connect(m_model, &QStandardItemModel::dataChanged, this, &ConfigWidget::Slot_Recipe_DataChanged);
	// 连接缺陷类型复选框状态改变信号
	connect(m_delegate, &ItemDelegate::checkboxStateChanged, this, &ConfigWidget::Slot_Recipe_DefectTypeCheckboxStateChanged);
	// 连接缺陷等级(列标题)复选框状态改变信号
	//connect(m_headerView, &CustomCheckBoxHeaderView::checkStateChanged,this, &ConfigWidget::Slot_Recipe_DefectLevelCheckboxStateChanged);
	connect(ui->checkBox_minorLevel, &QCheckBox::clicked, this, &ConfigWidget::Slot_Recipe_SetMinorLevelDisplayState);
	connect(ui->checkBox_mediumLevel, &QCheckBox::clicked, this, &ConfigWidget::Slot_Recipe_SetMediumLevelDisplayStat);
	connect(ui->checkBox_seriousLevel, &QCheckBox::clicked, this, &ConfigWidget::Slot_Recipe_SetSeriousLevelDisplayStat);
	connect(ui->checkBox_normalLevel, &QCheckBox::clicked, this, &ConfigWidget::Slot_Recipe_SetNormalLevelDisplayState);
	connect(ui->checkBox_areaerrorLevel, &QCheckBox::clicked, this, &ConfigWidget::Slot_Recipe_SetAreaErrorLevelDisplayState);
	connect(ui->checkBox_abnormalLevel, &QCheckBox::clicked, this, &ConfigWidget::Slot_Recipe_SetAbnormalLevelDisplayState);

	// 连接配方参数保存信号
	connect(ui->pBtn_SaveRecipe, &QPushButton::clicked, this, &ConfigWidget::Slot_Recipe_Save);

	// 连接配方参数设置为默认 信号
	connect(ui->pBtn_SetDefaultRecipe, &QPushButton::clicked, this, &ConfigWidget::Slot_Recipe_SetDefaultRecipe);

	connect(ui->pBtn_CreatRecipe, &QPushButton::clicked, this, &ConfigWidget::Slot_Recipe_CreateRecipe);

	connect(ui->pBtn_DeleteRecipe, &QPushButton::clicked, this, &ConfigWidget::Slot_Recipe_DeleteRecipe);

    connect(ui->pBtn_RenameRecipe, &QPushButton::clicked, this, &ConfigWidget::Slot_Recipe_Rename);

	connect(ui->pBtn_ExportRecipe, &QPushButton::clicked, this, &ConfigWidget::Slot_Recipe_SaveAs);

    connect(ui->pBtn_ImportRecipe, &QPushButton::clicked, this, &ConfigWidget::Slot_Recipe_Import);

    connect(ui->pBtn_CopyRecipe, &QPushButton::clicked, this, &ConfigWidget::Slot_Recipe_CopyAs);
	// 连接复选框状态改变信号
	connect(m_headerView, &CustomCheckBoxHeaderView::checkStateChanged,
		this, [this](int section, bool checked) {
			qDebug() << "Header checkbox changed for section" << section << ":" << checked;

			// 更新该列所有行的状态
			for (int row = 0; row < m_model->rowCount(); ++row) {
				QModelIndex index = m_model->index(row, section);
				if (index.isValid()) {
					// 复选框状态存储在 UserRole + 10
					m_model->setData(index, checked, Qt::UserRole + 10);
				}
			}

			// 更新视图
			ui->TableView_Recipe->viewport()->update();
		});

	connect(this, &ConfigWidget::Signal_UpdateRecipeDisplay, this, &ConfigWidget::Slot_Recipe_UpdateDisplayFlagToRecipeConfig);
	/*********************************************************************/
}

void ConfigWidget::Slot_System_EditConfig()
{
    // 1. 弹出对话框
    QInputDialog dialog(this);  // 设置 parent
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint); // 移除问号
    //dialog.setOptions(QInputDialog::UsePlainTextEditForTextInput); // 启用多行输入（关键！）
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setWindowTitle(tr("Edit system config"));
    dialog.setLabelText(tr("Please enter the edit password:"));
    dialog.setTextValue(""); // 初始文本

	// 获取对话框中的 QLineEdit 并设置为密码模式
	if (QLineEdit* lineEdit = dialog.findChild<QLineEdit*>())
	{
		lineEdit->setEchoMode(QLineEdit::Password);
		lineEdit->setAttribute(Qt::WA_InputMethodEnabled, false);
	}

    // 执行对话框
    bool ok = (dialog.exec() == QDialog::Accepted);
    QString editPassword = ok ? dialog.textValue() : QString();


	// 2. 检查用户操作
	if (ok)
	{
		// 用户点击了 "OK" 并且输入了非空名称

		// 3. 检查名称是否已存在 (可选但推荐)
		//    假设你有一个方法检查名称是否重复，例如在 QListWidget 或 QMap 中查找
		if (editPassword.isEmpty())
		{
			QMessageBox::warning(this, tr("Edit system config"),
				tr("The edit password cannot be empty."));
			return; // 退出
		}
		else if (editPassword != "bljcv3")
		{
			QMessageBox::warning(this, tr("Edit system config"),
				tr("The edit password is incorrect."));
			return; // 退出
		}
		else
		{
			QList<QWidget*> widgets = ui->tab_System->findChildren<QWidget*>();
			for (QWidget* widget : widgets) {
				
				// 跳过某些特定控件
				if (widget->objectName() == "pBtn_EditSystemConfig"
					|| widget->objectName() == "pBtn_SaveSystemConfig"
					|| widget->objectName() == "pBtn_CancelSystemConfig"
					|| widget->objectName() == "pBtn_SetLicenseInfo"
					|| widget->objectName() == "pBtn_ExportLicenseInfo"
					|| widget->objectName() == "pBtn_AboutLicense")
				{
					continue;
				}
				widget->setEnabled(true);
			}

			ui->pBtn_CancelSystemConfig->setEnabled(true);
			ui->pBtn_SaveSystemConfig->setEnabled(true);
			ui->pBtn_EditSystemConfig->setEnabled(false);
		}
	}

}

void ConfigWidget::Slot_System_CancelEditConfig()
{
	System_ShowSystemConfig();
	QList<QWidget*> widgets = ui->tab_System->findChildren<QWidget*>();
	for (QWidget* widget : widgets) {
		
		// 跳过某些特定控件
		if (widget->objectName() == "pBtn_EditSystemConfig"
			|| widget->objectName() == "pBtn_SaveSystemConfig"
			|| widget->objectName() == "pBtn_CancelSystemConfig"
			|| widget->objectName() == "pBtn_SetLicenseInfo"
			|| widget->objectName() == "pBtn_ExportLicenseInfo"
			|| widget->objectName() == "pBtn_AboutLicense")
		{
			continue;
		}
		widget->setEnabled(false);
	}
	ui->pBtn_CancelSystemConfig->setEnabled(false);
	ui->pBtn_SaveSystemConfig->setEnabled(false);
	ui->pBtn_EditSystemConfig->setEnabled(true);
}

void ConfigWidget::Slot_System_WriteConfig()
{
	QList<QWidget*> widgets = ui->tab_System->findChildren<QWidget*>();
	for (QWidget* widget : widgets) {

		// 跳过某些特定控件
		if (widget->objectName() == "pBtn_EditSystemConfig"
			|| widget->objectName() == "pBtn_SaveSystemConfig"
			|| widget->objectName() == "pBtn_CancelSystemConfig"
			|| widget->objectName() == "pBtn_SetLicenseInfo"
			|| widget->objectName() == "pBtn_ExportLicenseInfo"
			|| widget->objectName() == "pBtn_AboutLicense")
		{
			continue;
		}
		widget->setEnabled(false);
	}
	ui->pBtn_CancelSystemConfig->setEnabled(false);
	ui->pBtn_SaveSystemConfig->setEnabled(false);
	ui->pBtn_EditSystemConfig->setEnabled(true);

	System_SetSystemConfig();	//设置系统参数到全局变量
	emit Signal_WriteSystemConfig();
}

void ConfigWidget::Slot_System_SetLicenseInfo()
{

    QInputDialog dialog(this);  // 设置 parent
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint); // 移除问号
    dialog.setOptions(QInputDialog::UsePlainTextEditForTextInput); // 启用多行输入（关键！）
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setWindowTitle(tr("License key."));
    dialog.setLabelText(tr("Please enter your license key: "));
    dialog.setTextValue(""); // 初始文本

    // 执行对话框
    bool ok = (dialog.exec() == QDialog::Accepted);
    QString text = ok ? dialog.textValue() : QString();

	if (ok && !text.isEmpty()) {
        LicenseManager::getInstance()->validateLicenseKey(text);
	}

}

void ConfigWidget::Slot_System_ExportLicenseInfo()
{
	LicenseManager::getInstance()->exportLicenseInfo();
}

void ConfigWidget::Slot_System_AboutLicense()
{
    QString customerName = LicenseManager::getInstance()->getCustomerName(); 
	LicenseManager::LicenseType licenseType = LicenseManager::getInstance()->getLicenseType();
	QString licenseTypeStr = licenseType == LicenseManager::LicenseType::Permanent ? tr("Permanent") : tr("Trial");
    QString expireDate = LicenseManager::getInstance()->getExpireDate().toString("yyyy-MM-dd");
	if (licenseType == LicenseManager::LicenseType::Permanent)
	{
		expireDate = tr("Never expires");
	}
	int remainingDays = LicenseManager::getInstance()->getRemainingDays();
	QMessageBox::information(this, tr("About License"),
		tr("CustomerName: %1\nLicense type: %2\nExpire Date: %3\nRemaining Days: %4").arg(customerName).arg(licenseTypeStr).arg(expireDate).arg(remainingDays));
}

void ConfigWidget::Slot_Recipe_SetDefaultRecipe()
{
	/* 设置系统参数到全局变量 */
	if (ui->listWidget_RecipeList->currentItem() == nullptr)
	{
		return;
	}
	DefaultRecipeName = ui->listWidget_RecipeList->currentItem()->text().toStdString();
	/* 显示当前默认配方名称到界面*/
	ui->label_CurrentReipeName->setText(QString::fromStdString(DefaultRecipeName));

	emit Signal_WriteSystemConfig();

	RecipeInfo = m_EditRecipeConfig;			//获得当前的配方信息
	emit Signal_DefaultRecipeChanged();
}

void ConfigWidget::Slot_Recipe_CreateRecipe()
{
	// 1. 弹出文本输入对话框
    QInputDialog dialog(this);  // 设置 parent
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint); // 移除问号
    //dialog.setOptions(QInputDialog::UsePlainTextEditForTextInput); // 启用多行输入（关键！）
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setWindowTitle(tr("Create Recipe"));             // 对话框标题
    dialog.setLabelText(tr("Enter the new recipe name:"));  // 对话框内的提示标签
    dialog.setTextValue(tr("New Recipe"));                      // 初始文本
    // 执行对话框
    bool ok = (dialog.exec() == QDialog::Accepted);
    QString recipeName = ok ? dialog.textValue() : QString();
    

	// 2. 检查用户操作
	if (ok && !recipeName.isEmpty()) {
		// 用户点击了 "OK" 并且输入了非空名称

		// 3. 检查名称是否已存在 (可选但推荐)
		//    假设你有一个方法检查名称是否重复，例如在 QListWidget 或 QMap 中查找
		if (Recipe_IsRecipeNameExists(recipeName))
		{
			QMessageBox::warning(this, tr("Name Exists"),
				tr("A recipe named '%1' already exists. Please choose a different name.").arg(recipeName));
			return; // 不创建，退出
		}

		// 创建默认参数值的配方
		RecipeConfig::Config::create("./Recipes/"+ recipeName +".json");

		// 5. 添加到列表
		ui->listWidget_RecipeList->addItem(recipeName);

		m_EditRecipeName = recipeName.toStdString();

		QString recipeName = QString::fromStdString(m_EditRecipeName);
		// 取消当前所有选中项（可选）
		ui->listWidget_RecipeList->clearSelection();

		// 遍历所有项
		for (int i = 0; i < ui->listWidget_RecipeList->count(); ++i) {
			QListWidgetItem* item = ui->listWidget_RecipeList->item(i);
			if (item && item->text() == recipeName) {
				item->setSelected(true);
				ui->listWidget_RecipeList->setCurrentItem(item); // 确保焦点和当前项也设置
				ui->listWidget_RecipeList->scrollToItem(item);   // 可选：滚动到该项
				break; // 如果只需要选中第一个匹配项
			}
		}



		Recipe_ImportParameter();
		Recipe_DisplayParameter();
		qDebug() << "Created new recipe:" << recipeName;
		QMessageBox::information(this, tr("Success"), tr("Recipe '%1' created successfully.").arg(recipeName));
	}

}

bool ConfigWidget::Recipe_IsRecipeNameExists(const QString& name)
{
	// 遍历 QListWidget 的所有项
	for (int i = 0; i < ui->listWidget_RecipeList->count(); ++i)
	{
		QListWidgetItem* item = ui->listWidget_RecipeList->item(i);
		// 比较当前项的文本与传入的名称
		if (item->text() == name)
		{
			return true; // 找到重复名称，返回 true
		}
	}
	return false; 
}

void ConfigWidget::Slot_Recipe_DeleteRecipe()
{
	Recipe_DeleteRecipe();
}

void ConfigWidget::Slot_Recipe_Rename()
{
	// 1. 获取当前选中的配方项
	QList<QListWidgetItem*> selectedItems = ui->listWidget_RecipeList->selectedItems();
	if (selectedItems.isEmpty()) {
		QMessageBox::warning(this, tr("No Selection"), tr("Please select a recipe to rename."));
		return;
	}

	QString oldName = selectedItems.first()->text(); // 当前配方名
	if (oldName.isEmpty()) return;

    // 2. 弹出重命名对话框
    QInputDialog dialog(this);  // 设置 parent
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint); // 移除问号
    //dialog.setOptions(QInputDialog::UsePlainTextEditForTextInput); // 启用多行输入（关键！）
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setWindowTitle(tr("Rename Recipe"));
    dialog.setLabelText(tr("Enter the new recipe name:"));
    dialog.setTextValue(""); // 初始文本
    // 执行对话框
    bool ok = (dialog.exec() == QDialog::Accepted);
    QString newName = ok ? dialog.textValue() : QString();

	// 3. 用户取消或名称未改变
	if (!ok || newName == oldName)
	{
		QMessageBox::warning(this, tr("Rename Recipe"),
			tr("The recipe name has not changed. Return."));
		return;
	}

	// 4. 检查新名称是否为空
	if (newName.trimmed().isEmpty()) {
		QMessageBox::warning(this, tr("Rename Recipe"), tr("Recipe name cannot be empty."));
		return;
	}

	// 5. 检查新名称是否已存在（排除自身）
	if (Recipe_IsRecipeNameExists(newName)) {
		QMessageBox::warning(this, tr("Name Exists"),
			tr("A recipe named '%1' already exists. Please choose a different name.").arg(newName));
		return;
	}

	// 6. 构造旧路径和新路径
	QString recipesDir = "./Recipes/";
	QString oldPath = recipesDir + oldName + ".json";
	QString newPath = recipesDir + newName + ".json";

	// 7. 尝试重命名文件
	if (!QFile::rename(oldPath, newPath)) {
		QMessageBox::critical(this, tr("Rename Failed"),
			tr("Failed to rename recipe file from '%1' to '%2'. "
				"Make sure the file is not in use and you have write permissions.")
			.arg(oldName, newName));
		return;
	}

	// 8. 更新列表项文本
	selectedItems.first()->setText(newName);

	// 9. 如果当前重命名的是默认配方，则更新默认配方名称
	DefaultRecipeName = ui->listWidget_RecipeList->currentItem()->text().toStdString();
	/* 显示当前默认配方名称到界面*/
	ui->label_CurrentReipeName->setText(QString::fromStdString(DefaultRecipeName));

	emit Signal_WriteSystemConfig();

	// 10. 如果当前正在编辑的就是这个配方，更新内部状态
	if (QString::fromStdString(m_EditRecipeName) == oldName) {
		m_EditRecipeName = newName.toStdString();
	}

	// 12. 提示成功
	qDebug() << "Renamed recipe from" << oldName << "to" << newName;
	QMessageBox::information(this, tr("Success"),
		tr("Recipe renamed successfully from '%1' to '%2'.").arg(oldName, newName));

}

void ConfigWidget::Slot_Recipe_SaveAs()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Export Recipe"), QString(), tr("Recipe File(*.json)"));
	if (!fileName.isEmpty())
	{
		Recipe_ExportParameter(fileName);
	}
	Recipe_RefreshConfigFileNameList();	//刷新列表


}

void ConfigWidget::Slot_Recipe_Import()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Import Recipe"), QString(), tr("Recipe File(*.json)"));
	if (!fileName.isEmpty())
	{
        QFileInfo fileInfo(fileName); // 使用 QString 构造 QFileInfo 对象
        QString recipeName = fileInfo.baseName();
        if (Recipe_IsRecipeNameExists(recipeName))
        {
            recipeName = recipeName + "(1)";
        }


		m_EditRecipeConfig = RecipeConfig::Config::load(fileName);

		if (m_EditRecipeConfig.defects.isEmpty())
		{
			FILE_LOG_ERROR("Load Recipe Parameter Failed.");
			return;
		}

		QString newFileName = "./Recipes/"+ recipeName +".json";
		Recipe_ExportParameter(newFileName);	//导入的配方参数，要保存到配方文件夹中。
		Recipe_RefreshConfigFileNameList();	//刷新列表

		m_EditRecipeName = recipeName.toStdString();

		// 取消当前所有选中项（可选）
		ui->listWidget_RecipeList->clearSelection();

		// 遍历所有项
		for (int i = 0; i < ui->listWidget_RecipeList->count(); ++i) {
			QListWidgetItem* item = ui->listWidget_RecipeList->item(i);
			if (item && item->text() == recipeName) {
				item->setSelected(true);
				ui->listWidget_RecipeList->setCurrentItem(item); // 确保焦点和当前项也设置
				ui->listWidget_RecipeList->scrollToItem(item);   // 滚动到该项
				break; // 选中第一个匹配项就跳出
			}
		}

		Recipe_DisplayParameter();
	}
}

void ConfigWidget::Slot_Recipe_CopyAs()
{
    // 1. 获取当前选中的配方项
    QList<QListWidgetItem*> selectedItems = ui->listWidget_RecipeList->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a recipe to copy."));
        return;
    }

    QString recipe_selected = selectedItems.first()->text(); // 当前配方名
    if (recipe_selected.isEmpty()) return;

    QInputDialog dialog(this);  // 设置 parent
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint); // 移除问号
    //dialog.setOptions(QInputDialog::UsePlainTextEditForTextInput); // 启用多行输入（关键！）
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setWindowTitle(tr("Recipe Copy As"));             // 对话框标题
    dialog.setLabelText(tr("Enter the new recipe name:"));  // 对话框内的提示标签
    dialog.setTextValue(tr("New Recipe"));                      // 初始文本
    // 执行对话框
    bool ok = (dialog.exec() == QDialog::Accepted);
    QString recipeName = ok ? dialog.textValue() : QString();


    // 2. 检查用户操作
    if (ok && !recipeName.isEmpty()) {

        if (Recipe_IsRecipeNameExists(recipeName))
        {
            QMessageBox::warning(this, tr("Name Exists"),
                tr("A recipe named '%1' already exists. Please choose a different name.").arg(recipeName));
            return; // 不创建，退出
        }
        QString copyAsFileName = "./Recipes/" + recipeName + ".json";

        //检查并另存为新的配方。检查是否有启用但配方中没有的瑕疵项
        RecipeConfig::Config::copyAs(m_EditRecipeConfig, copyAsFileName);

        m_EditRecipeConfig = RecipeConfig::Config::load(copyAsFileName);
    }

    Recipe_RefreshConfigFileNameList();	//刷新列表
    m_EditRecipeName = recipeName.toStdString();

    // 取消当前所有选中项（可选）
    ui->listWidget_RecipeList->clearSelection();

    // 遍历所有项
    for (int i = 0; i < ui->listWidget_RecipeList->count(); ++i) {
        QListWidgetItem* item = ui->listWidget_RecipeList->item(i);
        if (item && item->text() == recipeName) {
            item->setSelected(true);
            ui->listWidget_RecipeList->setCurrentItem(item); // 确保焦点和当前项也设置
            ui->listWidget_RecipeList->scrollToItem(item);   // 滚动到该项
            break; // 选中第一个匹配项就跳出
        }
    }
    Recipe_DisplayParameter();
}

void ConfigWidget::Slot_Recipe_SetMinorLevelDisplayState(bool state)
{
	DefectLevel defectLevel = DefectLevel::MINOR;
	DISPLAY_MINOR = state;

	emit Signal_WriteSystemConfig();
	emit Signal_SetDefectLevelVisibility(defectLevel, state);
}

void ConfigWidget::Slot_Recipe_SetMediumLevelDisplayStat(bool state)
{
	DefectLevel defectLevel = DefectLevel::MEDIUM;
	DISPLAY_MEDIUM = state;

	emit Signal_WriteSystemConfig();
	emit Signal_SetDefectLevelVisibility(defectLevel, state);
}

void ConfigWidget::Slot_Recipe_SetSeriousLevelDisplayStat(bool state)
{
	DefectLevel defectLevel = DefectLevel::SERIOUS;
	DISPLAY_SERIOUS = state;

	emit Signal_WriteSystemConfig();
	emit Signal_SetDefectLevelVisibility(defectLevel, state);
}

void ConfigWidget::Slot_Recipe_SetNormalLevelDisplayState(bool state)
{
	DefectLevel defectLevel = DefectLevel::NORMAL;
	DISPLAY_NORMAL = state;

	emit Signal_WriteSystemConfig();
	emit Signal_SetDefectLevelVisibility(defectLevel, state);

}

void ConfigWidget::Slot_Recipe_SetAreaErrorLevelDisplayState(bool state)
{
	DefectLevel defectLevel = DefectLevel::AREAERROR;
	DISPLAY_AREAERROR = state;

	emit Signal_WriteSystemConfig();
	emit Signal_SetDefectLevelVisibility(defectLevel, state);
}

void ConfigWidget::Slot_Recipe_SetAbnormalLevelDisplayState(bool state)
{
	DefectLevel defectLevel = DefectLevel::ABNORMAL;
	DISPLAY_ABNORMAL = state;

	emit Signal_WriteSystemConfig();
	emit Signal_SetDefectLevelVisibility(defectLevel, state);
}

void ConfigWidget::Slot_Recipe_DataChanged(const QModelIndex& topLeft, const QModelIndex& /*bottomRight*/, const QVector<int>& /*roles*/)
{
	if (m_isRefreshView)
	{
		return;
	}

	QStandardItem* item = m_model->itemFromIndex(topLeft);
	if (!item) return;

	// 获取标记信息
	int defectTypeId = item->data(Qt::UserRole + 31).toInt();
	QString fieldType = item->data(Qt::UserRole + 32).toString(); // name, confidence, minor, etc.


	bool ok;
	QString displayText = item->text();
	displayText = displayText.remove(u8"≥");
	float value = displayText.toFloat(&ok);

	auto showWarning = [this](const QString& msg) {
		QMessageBox::warning(this, tr("Warning"), msg);
		};

	auto restoreValue = [this, item](float oldVal) {
		qDebug() << "bbb";
		m_isRefreshView = true;
		int decimals = item->data(Qt::UserRole + 13).isValid()
			? item->data(Qt::UserRole + 13).toInt()
			: 2;
		QString displayText = u8"≥" + QString::number(oldVal, 'f', decimals); // 显示值
		item->setData(displayText, Qt::DisplayRole);   // 显示带 ≥ 的文本
		//item->setText(QString::number(oldVal, 'f', 3));
		m_isRefreshView = false;
		};

	if (defectTypeId == 999)
	{
		if (fieldType == "mergeDistance")
		{
			m_EditRecipeConfig.global.mergeDistance = value;
		}
		else if (fieldType == "confidence1")
		{
			m_EditRecipeConfig.global.thresholdDetect_1 = value;
		}
		else if (fieldType == "confidence2")
		{
			m_EditRecipeConfig.global.thresholdDetect_2 = value;
		}
		//20251118: 取消映射比例的使用
		//else if (m_EditRecipeConfig.global.mappingScaleName == fieldType)
		//{
		//	m_EditRecipeConfig.global.mappingScale = value;
		//}
		else if (fieldType == "savePath")
		{
			m_EditRecipeConfig.global.dataSavePath = item->text();
		}
		else if (fieldType == "grayDifference")
		{
			m_EditRecipeConfig.global.grayDifference = (int)value;
		}
	}
	else
	{

		// 查找对应缺陷在 m_recipeInfo.defects 中的索引
		auto it = std::find_if(m_EditRecipeConfig.defects.begin(), m_EditRecipeConfig.defects.end(),
			[&defectTypeId](const RecipeConfig::DefectConfig& d) {
				return d.id == defectTypeId;
			});

		if (it == m_EditRecipeConfig.defects.end())
        {
            DefectType type = static_cast<DefectType>(it->id);
			showWarning(tr("Unknown defect type: ") + DefectTypeToString(type));
			return;
		}


		RecipeConfig::DefectConfig& defect = *it;

		// 开始根据 fieldType 更新
		if (fieldType == "name") {
			defect.name = item->text();
		}
		else if (fieldType == "confidence") {
			if (!ok || value < 0.0f || value > 1.0f) {
				showWarning(tr("Confidence must be between 0.0 and 1.0"));
				restoreValue(defect.confidence);
				return;
			}
			defect.confidence = value;
		}
		else if (fieldType == "minor") {
			float moderate = defect.moderateThreshold;
			if (!ok || value >= moderate) {
				showWarning(tr("Minor must be less than Moderate"));
				restoreValue(defect.minorThreshold);
				return;
			}
			defect.minorThreshold = value;
		}
		else if (fieldType == "moderate") {
			float minor = defect.minorThreshold;
			float major = defect.majorThreshold;
			if (!ok || value <= minor || value >= major) {
				showWarning(tr("Moderate must be between Minor and Major"));
				restoreValue(defect.moderateThreshold);
				return;
			}
			defect.moderateThreshold = value;
		}
		else if (fieldType == "major") {
			float moderate = defect.moderateThreshold;
			if (!ok || value <= moderate) {
				showWarning(tr("Major must be greater than Moderate"));
				restoreValue(defect.majorThreshold);
				return;
			}
			defect.majorThreshold = value;
		}
	}
	// 内存中已更新，等待 Save 时统一写入文件
}

/* 缺陷类型显示状态变更 槽函数 */
void ConfigWidget::Slot_Recipe_DefectTypeCheckboxStateChanged(const QModelIndex& index, bool checked)
{


	//typedef enum DefectType
	//{

	//	TYPE_POORCOATING,							//0 镀膜不良
	//	TYPE_SCRATCH,								//1 划伤
	//	TYPE_CALCULUS,								//2 结石
	//	TYPE_BUBBLE,								//3 气泡
	//	TYPE_TRADEMARK,								//4 商标
	//	TYPE_WATERSTAIN,							//5 水渍
	//	TYPE_SMUDGE,								//6 脏污
	//	TYPE_SCREENPRINTING							//7 丝印缺陷
	//};

	int DefectTypeRow = index.row();
	qDebug() << "Checkbox state changed at row:" << DefectTypeRow
		<< "column:" << index.column()
		<< "new state:" << checked;
	int defectTypeId = index.data(Qt::UserRole + 31).toInt();
    DefectType defectType = static_cast<DefectType>(defectTypeId);;

    m_defectTypeVisibility[defectType]= checked;

	emit Signal_UpdateRecipeDisplay();
	emit Signal_WriteSystemConfig();
	emit Signal_SetDefectTypeVisibility(defectType, checked);
}

void ConfigWidget::Slot_Recipe_UpdateDisplayFlagToRecipeConfig()
{ 
	Recipe_UpdateDisplayFlagToRecipeConfig(RecipeInfo);	//更新瑕疵类型显示状态到默认配方中
	//Recipe_UpdateDisplayFlagToRecipeConfig(m_EditRecipeConfig);	//更新瑕疵类型显示状态到当前展示的配方中
}

//更新瑕疵类型显示状态到默认配方中
void ConfigWidget::Recipe_UpdateDisplayFlagToRecipeConfig(RecipeConfig::Config& recipe)
{ 

	//根据设置的瑕疵类型显示标志，设置每个类型对应图例的显示状态
	for (auto& defect : recipe.defects)
	{
        DefectType defectType= static_cast<DefectType>(defect.id);;
        defect.isDisplayed = m_defectTypeVisibility[defectType];
	}
}

/* 缺陷等级显示状态变更 槽函数 */
void ConfigWidget::Slot_Recipe_DefectLevelCheckboxStateChanged(int section, bool checked)
{	
	//typedef enum DefectLevel
	//{

	//	NORMAL,										//0 正常
	//	MINOR,										//1 轻微缺陷
	//	MEDIUM,										//2 中等缺陷
	//	SERIOUS,									//3 严重缺陷
	//	ABNORMAL,									//4 异常
	//	AREAERROR									//5 区域错误，非玻璃区域判断
	//};
    DefectLevel defectLevel;
	if (section == 3)
	{
        defectLevel = DefectLevel::MINOR;
		DISPLAY_MINOR = checked;
	}
    else if (section == 4)
    {
        defectLevel = DefectLevel::MEDIUM;
		DISPLAY_MEDIUM = checked;
    }
    else if (section == 5)
    {
        defectLevel = DefectLevel::SERIOUS;
		DISPLAY_SERIOUS = checked;
    }

	else
	{
		defectLevel = DefectLevel::NORMAL;
		DISPLAY_NORMAL = checked;
	}

	emit Signal_WriteSystemConfig();
	emit Signal_SetDefectLevelVisibility(defectLevel, checked);
    //DefectRecipeItem::LevelCheckStateChanged(section, checked);
}

void ConfigWidget::Slot_Recipe_SelectedRecipeChanged(QListWidgetItem* item)
{
	if (!item)
	{
		return;
	}
	QString itemText = item->text(); // 获取项的文本
	m_EditRecipeName = item->text().toStdString();
	Recipe_ImportParameter();
	Recipe_DisplayParameter();
}

void ConfigWidget::System_ShowSystemConfig()
{
	/******** 客户名称 ********************************/
	ui->lineEdit_Customer->setText(CustomerName.c_str());
	/*************************************************/

	/******** 自动清理图像 ***************************/
	ui->checkBox_IsAutoCleanup->setChecked(IsAutoCleanup);
	ui->spinBox_ImagesRetentionDays->setValue(ImagesRetentionDays);
	ui->spinBox_DailyCleaningTime->setValue(AutoCleanupTime);
	/*************************************************/

	/******** 计算参数 *******************************/
	ui->doubleSpinBox_WidthScale->setValue(widthScale);
	ui->doubleSpinBox_HeightScale->setValue(heightScale);
	ui->doubleSpinBox_PixleToMM_X->setValue(Pixle2MM_X);
	ui->doubleSpinBox_PixleToMM_Y->setValue(Pixle2MM_Y);
	/*************************************************/

	/******** 系统 ***********************************/
	ui->spinBox_CameraCount->setValue(CameraCount);
	ui->spinBox_LightCount->setValue(LightCount);
	int index = ui->comboBox_Language->findText(QString::fromStdString(Language));
	if (index != -1)
	{
		ui->comboBox_Language->setCurrentIndex(index);
	}
	else
	{
		ui->comboBox_Language->setCurrentIndex(2);
	}
	ui->checkBox_SendResults->setChecked(SendResults);
	/*************************************************/

	/******** 图像参数 *******************************/
	ui->spinBox_ExposureTime->setValue(ExposureTime);
	ui->spinBox_ImageHeight->setValue(ImageHeight);
	ui->dSpinBox_StopDelay->setValue(DelayStopTime);
	if (RotationDirectionFlag == 0)
	{
		ui->checkBox_RotateCW->setChecked(true);
	}
	else
	{
		ui->checkBox_RotateCCW->setChecked(true);
	}
	/*************************************************/

	/******** 相机参数 ******************************/
	ui->lineEdit_Camera_1->setText(QString::fromStdString(Camera1));
	if (camerainfo.size() >= 2 && camerainfo[1].size() == 4)
	{
		ui->spinBox_pixelstart_1->setValue(camerainfo[1][0]);
		ui->spinBox_pixelend_1->setValue(camerainfo[1][1]);
		ui->spinBox_ditancemm_1->setValue(camerainfo[1][2]);
		ui->spinBox_yoffsetpxile_1->setValue(camerainfo[1][3]);
	}

	ui->lineEdit_Camera_2->setText(QString::fromStdString(Camera2));
	if (camerainfo.size() >= 3 && camerainfo[2].size() == 4)
	{
		ui->spinBox_pixelstart_2->setValue(camerainfo[2][0]);
		ui->spinBox_pixelend_2->setValue(camerainfo[2][1]);
		ui->spinBox_ditancemm_2->setValue(camerainfo[2][2]);
		ui->spinBox_yoffsetpxile_2->setValue(camerainfo[2][3]);
	}

	ui->lineEdit_Camera_3->setText(QString::fromStdString(Camera3));
	if (camerainfo.size() >= 4 && camerainfo[3].size() == 4)
	{
		ui->spinBox_pixelstart_3->setValue(camerainfo[3][0]);
		ui->spinBox_pixelend_3->setValue(camerainfo[3][1]);
		ui->spinBox_ditancemm_3->setValue(camerainfo[3][2]);
		ui->spinBox_yoffsetpxile_3->setValue(camerainfo[3][3]);
	}

	ui->lineEdit_Camera_4->setText(QString::fromStdString(Camera4));
	if (camerainfo.size() >= 5 && camerainfo[4].size() == 4)
	{
		ui->spinBox_pixelstart_4->setValue(camerainfo[4][0]);
		ui->spinBox_pixelend_4->setValue(camerainfo[4][1]);
		ui->spinBox_ditancemm_4->setValue(camerainfo[4][2]);
		ui->spinBox_yoffsetpxile_4->setValue(camerainfo[4][3]);
	}

	ui->lineEdit_Camera_5->setText(QString::fromStdString(Camera5));
	if (camerainfo.size() >= 6 && camerainfo[5].size() == 4)
	{
		ui->spinBox_pixelstart_5->setValue(camerainfo[5][0]);
		ui->spinBox_pixelend_5->setValue(camerainfo[5][1]);
		ui->spinBox_ditancemm_5->setValue(camerainfo[5][2]);
		ui->spinBox_yoffsetpxile_5->setValue(camerainfo[5][3]);
	}

	ui->lineEdit_Camera_6->setText(QString::fromStdString(Camera6));
	if (camerainfo.size() >= 7 && camerainfo[6].size() == 4)
	{
		ui->spinBox_pixelstart_6->setValue(camerainfo[6][0]);
		ui->spinBox_pixelend_6->setValue(camerainfo[6][1]);
		ui->spinBox_ditancemm_6->setValue(camerainfo[6][2]);
		ui->spinBox_yoffsetpxile_6->setValue(camerainfo[6][3]);
	}

	ui->lineEdit_Camera_7->setText(QString::fromStdString(Camera7));
	if (camerainfo.size() >= 8 && camerainfo[7].size() == 4)
	{
		ui->spinBox_pixelstart_7->setValue(camerainfo[7][0]);
		ui->spinBox_pixelend_7->setValue(camerainfo[7][1]);
		ui->spinBox_ditancemm_7->setValue(camerainfo[7][2]);
		ui->spinBox_yoffsetpxile_7->setValue(camerainfo[7][3]);
	}

	ui->lineEdit_Camera_8->setText(QString::fromStdString(Camera8));
	if (camerainfo.size() >= 9 && camerainfo[8].size() == 4)
	{
		ui->spinBox_pixelstart_8->setValue(camerainfo[8][0]);
		ui->spinBox_pixelend_8->setValue(camerainfo[8][1]);
		ui->spinBox_ditancemm_8->setValue(camerainfo[8][2]);
		ui->spinBox_yoffsetpxile_8->setValue(camerainfo[8][3]);
	}

	ui->lineEdit_Camera_9->setText(QString::fromStdString(Camera9));
	if (camerainfo.size() >= 10 && camerainfo[9].size() == 4)
	{
		ui->spinBox_pixelstart_9->setValue(camerainfo[9][0]);
		ui->spinBox_pixelend_9->setValue(camerainfo[9][1]);
		ui->spinBox_ditancemm_9->setValue(camerainfo[9][2]);
		ui->spinBox_yoffsetpxile_9->setValue(camerainfo[9][3]);
	}
	/*************************************************/



}


void ConfigWidget::System_SetSystemConfig()
{
	/******** 客户名称 ********************************/
	//CustomerName = ui->lineEdit_Customer->text().toLocal8Bit().data();
	CustomerName = ui->lineEdit_Customer->text().toStdString();
	/*************************************************/

	/******** 自动清理图像 ***************************/
	IsAutoCleanup = ui->checkBox_IsAutoCleanup->isChecked();;
	ImagesRetentionDays = ui->spinBox_ImagesRetentionDays->value();
	AutoCleanupTime = ui->spinBox_DailyCleaningTime->value();
	/*************************************************/

	/******** 计算参数 *******************************/
	widthScale = ui->doubleSpinBox_WidthScale->value();
	heightScale = ui->doubleSpinBox_HeightScale->value();
	Pixle2MM_X = ui->doubleSpinBox_PixleToMM_X->value();
	Pixle2MM_Y = ui->doubleSpinBox_PixleToMM_Y->value();
	/*************************************************/

	/******** 系统 ***********************************/
	CameraCount = ui->spinBox_CameraCount->value();
	LightCount = ui->spinBox_LightCount->value();
	QString newLanguage = ui->comboBox_Language->currentText();
	if (newLanguage.toStdString() != Language)
	{
		emit Signal_LanguageChanged(ui->comboBox_Language->currentIndex());	//发送语言包改变信号
	}
	Language = newLanguage.toStdString();
	SendResults = ui->checkBox_SendResults->isChecked();
	/*************************************************/

	/******** 图像参数 *******************************/
	ExposureTime = ui->spinBox_ExposureTime->value();
	ImageHeight = ui->spinBox_ImageHeight->value();
	DelayStopTime = ui->dSpinBox_StopDelay->value();
	RotationDirectionFlag = ui->checkBox_RotateCW->isChecked() ? 0 : 1;
	/*************************************************/

	/******** 相机参数 ******************************/
	Camera1 = ui->lineEdit_Camera_1->text().toStdString();

	if (camerainfo.size() >= 2 && camerainfo[1].size() == 4)
	{
		camerainfo[1][0] = ui->spinBox_pixelstart_1->value();
		camerainfo[1][1] = ui->spinBox_pixelend_1->value();
		camerainfo[1][2] = ui->spinBox_ditancemm_1->value();
		camerainfo[1][3] = ui->spinBox_yoffsetpxile_1->value();
	}

	Camera2 = ui->lineEdit_Camera_2->text().toStdString();
	if (camerainfo.size() >= 3 && camerainfo[2].size() == 4)
	{
		camerainfo[2][0] = ui->spinBox_pixelstart_2->value();
		camerainfo[2][1] = ui->spinBox_pixelend_2->value();
		camerainfo[2][2] = ui->spinBox_ditancemm_2->value();
		camerainfo[2][3] = ui->spinBox_yoffsetpxile_2->value();
	}
	Camera3 = ui->lineEdit_Camera_3->text().toStdString();
	if (camerainfo.size() >= 4 && camerainfo[3].size() == 4)
	{
		camerainfo[3][0] = ui->spinBox_pixelstart_3->value();
		camerainfo[3][1] = ui->spinBox_pixelend_3->value();
		camerainfo[3][2] = ui->spinBox_ditancemm_3->value();
		camerainfo[3][3] = ui->spinBox_yoffsetpxile_3->value();
	}
	Camera4 = ui->lineEdit_Camera_4->text().toStdString();
	if (camerainfo.size() >= 5 && camerainfo[4].size() == 4)
	{
		camerainfo[4][0] = ui->spinBox_pixelstart_4->value();
		camerainfo[4][1] = ui->spinBox_pixelend_4->value();
		camerainfo[4][2] = ui->spinBox_ditancemm_4->value();
		camerainfo[4][3] = ui->spinBox_yoffsetpxile_4->value();
	}
	Camera5 = ui->lineEdit_Camera_5->text().toStdString();
	if (camerainfo.size() >= 6 && camerainfo[5].size() == 4)
	{
		camerainfo[5][0] = ui->spinBox_pixelstart_5->value();
		camerainfo[5][1] = ui->spinBox_pixelend_5->value();
		camerainfo[5][2] = ui->spinBox_ditancemm_5->value();
		camerainfo[5][3] = ui->spinBox_yoffsetpxile_5->value();
	}
	Camera6 = ui->lineEdit_Camera_6->text().toStdString();
	if (camerainfo.size() >= 7 && camerainfo[6].size() == 4)
	{
		camerainfo[6][0] = ui->spinBox_pixelstart_6->value();
		camerainfo[6][1] = ui->spinBox_pixelend_6->value();
		camerainfo[6][2] = ui->spinBox_ditancemm_6->value();
		camerainfo[6][3] = ui->spinBox_yoffsetpxile_6->value();
	}
	Camera7 = ui->lineEdit_Camera_7->text().toStdString();
	if (camerainfo.size() >= 8 && camerainfo[7].size() == 4)
	{
		camerainfo[7][0] = ui->spinBox_pixelstart_7->value();
		camerainfo[7][1] = ui->spinBox_pixelend_7->value();
		camerainfo[7][2] = ui->spinBox_ditancemm_7->value();
		camerainfo[7][3] = ui->spinBox_yoffsetpxile_7->value();
	}
	Camera8 = ui->lineEdit_Camera_8->text().toStdString();
	if (camerainfo.size() >= 9 && camerainfo[8].size() == 4)
	{
		camerainfo[8][0] = ui->spinBox_pixelstart_8->value();
		camerainfo[8][1] = ui->spinBox_pixelend_8->value();
		camerainfo[8][2] = ui->spinBox_ditancemm_8->value();
		camerainfo[8][3] = ui->spinBox_yoffsetpxile_8->value();
	}
	Camera9 = ui->lineEdit_Camera_9->text().toStdString();
	if (camerainfo.size() >= 10 && camerainfo[9].size() == 4)
	{
		camerainfo[9][0] = ui->spinBox_pixelstart_9->value();
		camerainfo[9][1] = ui->spinBox_pixelend_9->value();
		camerainfo[9][2] = ui->spinBox_ditancemm_9->value();
		camerainfo[9][3] = ui->spinBox_yoffsetpxile_9->value();
	}

	/*************************************************/

}

ConfigWidget::~ConfigWidget()
{
	//std::cout << "ConfigWidget::~ConfigWidget()" << std::endl;
	delete ui;
}

void ConfigWidget::Slot_Recipe_Save()
{
	Recipe_SaveParameter();
}

void ConfigWidget::Recipe_SaveParameter()
{
	FILE_LOG_INFO("Recipe: SaveParameter Begin.");
	m_EditRecipeConfig.save(); // 自动保存到原路径

	if (m_EditRecipeName == DefaultRecipeName)
	{
		RecipeInfo = m_EditRecipeConfig;			//获得当前的配方信息
		emit Signal_DefaultRecipeChanged();
	}
	
	FILE_LOG_INFO("Recipe: SaveParameter Completed.");
}

void ConfigWidget::Recipe_DisplayParameter()
{
	///* 显示当前默认配方名称到界面*/
	//ui->label_CurrentReipeName->setText(QString::fromStdString(m_EditRecipeName));

	/* 显示配方参数对应的名称到界面*/
	ui->lineEdit_RecipeName->setText(QString::fromStdString(m_EditRecipeName));
	
	// 清空模型
	m_model->clear();

	FILE_LOG_INFO("Show parameter from new Recipe .");

	// 设置列头
	m_model->setHorizontalHeaderLabels({
		tr("RecipeType"), tr("unit"), tr("confidence"),
		tr("Minor"), tr("Moderatet"), tr("Major")
		});

	// ======== 1. 填充所有缺陷项（来自 m_EditRecipeConfig.defects）========
	for (const auto& defect : m_EditRecipeConfig.defects)
	{
		Recipe_AddDefectRow(defect);
	}

	// ======== 2. 填充全局配置项（来自 config.global）========
	const auto& global = m_EditRecipeConfig.global;

	// 添加数值型全局配置
	Recipe_AddGlobalRow(
		//global.mergeDistanceName,			// 显示名称
		QString(tr("mergeDistance")),		// 显示名称
		QString(u8"mergeDistance"),			// 类型标枪
		global.mergeDistance, 				// 当前值
		0, 2000, 1, 0, 						// 最小值、最大值、步长、小数位
		"mm"								// 单位(每单位的不传递即可)
	);

	Recipe_AddGlobalRow(
		//global.thresholdDetect_1_Name,	// 显示名称
		QString(tr("thresholdDetect_1")),	// 显示名称
		QString(u8"confidence1"),			// 类型标签
		global.thresholdDetect_1, 			// 当前值
		0, 2000, 0.1, 1 						// 最小值、最大值、步长、小数位
	);

	Recipe_AddGlobalRow(
		//global.thresholdDetect_2_Name,	// 显示名称
		QString(tr("thresholdDetect_2")),	// 显示名称
		QString(u8"confidence2"),			// 类型标签
		global.thresholdDetect_2, 			// 当前值
		0, 2000, 0.1, 1 						// 最小值、最大值、步长、小数位
	);

	Recipe_AddGlobalRow(
		//global.grayDifference_Name,		// 显示名称
		QString(tr("grayDifference")),		// 显示名称
		QString(u8"grayDifference"),		// 类型标签
		global.grayDifference, 				// 当前值
		0, 2000, 1, 0 						// 最小值、最大值、步长、小数位
	);
	//20251118: 取消映射比例的使用
	//Recipe_AddGlobalRow(
	//	global.mappingScaleName,			// 显示名称
	//	global.mappingScale,				// 当前值
	//	0.0, 1.0, 0.01, 3					// 最小值、最大值、步长、小数位
	//);

	// 路径项用专用函数
	Recipe_AddStringRow(
		//global.dataSavePathName,			// 显示名称
		QString(tr("filePath")),		// 显示名称
		QString(u8"savePath"),				// 类型标签
		global.dataSavePath					// 路径文本字符串
	);

	// ======== 4. 调整合并单元格跨度 ========
	int defectCount = m_EditRecipeConfig.defects.size();
	Recipe_AdjustTableSpans(defectCount);

	// 刷新视图
	ui->TableView_Recipe->update();
}

void ConfigWidget::Recipe_DeleteRecipe()
{
	// 1. 获取当前行号
	int currentRow = ui->listWidget_RecipeList->currentRow();

	// 2. 检查当前行是否有效
	if (currentRow < 0 || currentRow >= ui->listWidget_RecipeList->count()) {

		QMessageBox::information(this, tr("Warning"), tr("Please select a recipe first."));
		return;
	}

	// 3. 获取当前行的 QListWidgetItem
	QListWidgetItem* currentItem = ui->listWidget_RecipeList->item(currentRow);
	if (!currentItem) {
		qDebug() << "无法获取当前行的项。";
		return; 
	}

	QString recipeName = currentItem->text(); // 获得当前行的配方名称

	// 4. 检查是否当前默认配方
	if (recipeName == QString::fromStdString(DefaultRecipeName))
	{
		QMessageBox::information(this, tr("Warning"), tr("The current recipe is set as default and cannot be deleted. Please change the default recipe first."));
		return;
	}

	RecipeConfig::Config::remove("./Recipes/"+ recipeName+".json");

	// 提示用户确认删除
	// 如果不需要确认，可以注释掉这个 QMessageBox 部分
	int ret = QMessageBox::question(this, tr("Delete Recipe"),
		QString(tr("Are you sure you want to delete recipe ""%1""?\n")).arg(recipeName),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No);
	if (ret != QMessageBox::Yes)
	{
		return;
	}

	//刷新列表(列表不再显示删除的项)
	//Recipe_RefreshConfigFileNameList();

	//删除指定项
	Recipe_DeleteItemSafely(currentRow);

	//展示新的选中行(当前行)
	QListWidgetItem* newCurrentItem = ui->listWidget_RecipeList->currentItem();
	if (!newCurrentItem)
	{
		return;
	}
	QString newRecipeName = newCurrentItem->text(); // 获得新的当前行的配方名称
	m_EditRecipeName=newRecipeName.toStdString();
	Recipe_ImportParameter();
	Recipe_DisplayParameter();

}

void ConfigWidget::Recipe_DeleteItemSafely(int rowToDelete)
{
	// 1. 记录要删除的行号
	int targetRow = rowToDelete;
	// 2. 获取要删除的项 (用于释放内存)
	QListWidgetItem* item = ui->listWidget_RecipeList->takeItem(targetRow);
	delete item; // takeItem 后需要手动 delete

	// 3. 安全地设置新的当前行 (焦点)
	int itemCount = ui->listWidget_RecipeList->count();

	if (itemCount == 0) {
		// 列表为空，无需设置焦点
		qDebug() << "列表已空。";
		return;
	}

	// 优先选择原行的上一行
	int newCurrentRow = targetRow - 1;
	// 如果上一行不存在 (如删除了第0行)，则选择新的第0行
	if (newCurrentRow < 0)
	{
		newCurrentRow = 0;
	}
	// 如果新行号仍在有效范围内，则设置为当前行
	if (newCurrentRow < itemCount)
	{
		ui->listWidget_RecipeList->setCurrentRow(newCurrentRow);
		qDebug() << "焦点已移动到行:" << newCurrentRow;
	}
	else
	{
		// 理论上 itemCount > 0 时 newCurrentRow 不会 >= itemCount
		qDebug() << "无法设置新焦点。";
	}
}

void ConfigWidget::Recipe_RefreshConfigFileNameList()
{
	/* step0: 从配方文件目录中获取配方文件名称列表 */
	QString folderPath = QString::fromStdString(RecipeFolderPath);
	QStringList filenames;
	QDir dir(folderPath);

	if (!dir.exists())
	{
		qWarning() << "文件夹不存在:" << folderPath;
		return;
	}

	// 过滤 .json 文件
	QFileInfoList list = dir.entryInfoList(QStringList() << "*.json", QDir::Files, QDir::Name);
	for (const QFileInfo& fileInfo : list)
	{
		//filenames.append(fileInfo.fileName()); // 只取文件名，如 "config.json"
		filenames.append(fileInfo.baseName());	//只取文件名，如 "config"
	}
	/***********************************************/

	/* step1:显示配方名称到列表*/
	ui->listWidget_RecipeList->clear();
	ui->listWidget_RecipeList->addItems(filenames);
	/***********************************************/
}

void ConfigWidget::Recipe_ShowDefaultParameter()
{ 
	/* 显示当前默认配方名称到界面*/
	QString defaultName = QString::fromStdString(DefaultRecipeName);
	ui->label_CurrentReipeName->setText(defaultName);

	if (!ui->listWidget_RecipeList) return;

	// 取消当前所有选中项（可选）
	ui->listWidget_RecipeList->clearSelection();

	// 遍历所有项
	for (int i = 0; i < ui->listWidget_RecipeList->count(); ++i) {
		QListWidgetItem* item = ui->listWidget_RecipeList->item(i);
		if (item && item->text() == defaultName) {
			item->setSelected(true);
			ui->listWidget_RecipeList->setCurrentItem(item); // 确保焦点和当前项也设置
			ui->listWidget_RecipeList->scrollToItem(item);   // 滚动到该项
			break; // 只需要选中第一个匹配项
		}
	}
	Recipe_DisplayParameter();
}

void ConfigWidget::Recipe_AddDefectRow(const RecipeConfig::DefectConfig& defect)
{
	// 名称项（带复选框状态）
	DefectType defectType = static_cast<DefectType>(defect.id);
	QString nameCheckBoxText = DefectTypeToString(defectType);
	QStandardItem* nameItem = new QStandardItem(nameCheckBoxText);
	nameItem->setData(true, Qt::UserRole + 8);  // 启用复选框

	bool isChecked=defect.isDisplayed;

	nameItem->setData(isChecked, Qt::UserRole + 9);  // 默认启用
	// 设置图标
	QString iconPath = GetDefectIconPath(defect.id);
	nameItem->setData(iconPath, Qt::UserRole + 20);			// 存储图标路径
	nameItem->setData(defect.id, Qt::UserRole + 31);		// 标记缺陷类型
	nameItem->setData("name", Qt::UserRole + 32);			// 标记字段类型
    nameItem->setData(false, Qt::UserRole + 33);			// 设置缺陷名称的文本是否可以编辑

	// 单位
	QStandardItem* unitItem = new QStandardItem(defect.thresholdUnit);
	unitItem->setFlags(unitItem->flags() & ~Qt::ItemIsEditable);
	unitItem->setData(defect.id, Qt::UserRole + 31);
	unitItem->setData("unit", Qt::UserRole + 32);

	// 置信度
	QStandardItem* confItem = Recipe_CreateDoubleItem(
		defect.confidence, 0.0, 1.0, 0.01, 2);
	confItem->setData(defect.id, Qt::UserRole + 31);		// 标记缺陷类型
	confItem->setData("confidence", Qt::UserRole + 32);			// 标记字段类型

	QString prefix = u8"≥";  
	// 阈值
	int decimals = 3;
	QStandardItem* minorItem = Recipe_CreateDoubleItem(
		defect.minorThreshold, 0.0001, 99999.0, 0.1, decimals);
	minorItem->setData(defect.id, Qt::UserRole + 31);
	minorItem->setData("minor", Qt::UserRole + 32);
	QString displayText = prefix + QString::number(defect.minorThreshold, 'f', decimals); // 显示值
	minorItem->setData(displayText, Qt::DisplayRole);   // 显示带 ≥ 的文本

	decimals = 3;
	QStandardItem* moderateItem = Recipe_CreateDoubleItem(
		defect.moderateThreshold, 0.0001, 99999.0, 0.1, decimals);
	moderateItem->setData(defect.id, Qt::UserRole + 31);
	moderateItem->setData("moderate", Qt::UserRole + 32);
	QString displayText1 = prefix + QString::number(defect.moderateThreshold, 'f', decimals); // 显示值
	moderateItem->setData(displayText1, Qt::DisplayRole);   // 显示带 ≥ 的文本

	decimals = 3;
	QStandardItem* majorItem = Recipe_CreateDoubleItem(
		defect.majorThreshold, 0.0001, 99999.0, 0.1, decimals);
	majorItem->setData(defect.id, Qt::UserRole + 31);
	majorItem->setData("major", Qt::UserRole + 32);
	QString displayText2 = prefix + QString::number(defect.majorThreshold, 'f', decimals); // 显示值
	majorItem->setData(displayText2, Qt::DisplayRole);   // 显示带 ≥ 的文本

	m_model->appendRow({ nameItem, unitItem, confItem, minorItem, moderateItem, majorItem });
}

void ConfigWidget::Recipe_AddGlobalRow(
	const QString& labelShow, 
	const QString& labelType,
	double value,
	double min, double max, double step, int decimals,
	const QString& unit)
{
	QStandardItem* nameItem = new QStandardItem(labelShow);
	nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);

	if (!unit.isEmpty()) {
		QStandardItem* unitItem = new QStandardItem(unit);
		unitItem->setFlags(unitItem->flags() & ~Qt::ItemIsEditable);

		QStandardItem* valueItem = Recipe_CreateDoubleItem(value, min, max, step, decimals);
		valueItem->setData(999, Qt::UserRole + 31); // 标识
		valueItem->setData(labelType, Qt::UserRole + 32); // 标识
		
		m_model->appendRow({ nameItem, unitItem, valueItem });
	}
	else {
		QStandardItem* valueItem = Recipe_CreateDoubleItem(value, min, max, step, decimals);
		valueItem->setData(999, Qt::UserRole + 31); // 标识
		valueItem->setData(labelType, Qt::UserRole + 32); // 标识
		m_model->appendRow({ nameItem, valueItem });
	}
}

void ConfigWidget::Recipe_AddStringRow(const QString& label, const QString& labelType, const QString& value)
{
	auto* nameItem = new QStandardItem(label);
	nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);

	auto* valueItem = new QStandardItem(value);
	valueItem->setData("path", Qt::UserRole + 1);  // 标记用途
	valueItem->setData(999, Qt::UserRole + 31); // 标识
	valueItem->setData(labelType, Qt::UserRole + 32); // 标识
	valueItem->setToolTip(value);

	m_model->appendRow({ nameItem, valueItem });
}

void ConfigWidget::Recipe_ImportParameter()
{

	QString configPath = "./Recipes/" + QString::fromStdString(m_EditRecipeName) + ".json"; // 替换为你的实际路径
	m_EditRecipeConfig = RecipeConfig::Config::load(configPath);

	//检查并处理没有使用的瑕疵类型
    QSet<int> typesToRemove;
    for (const auto& defect : RecipeInfo.defects)
    {
        //设置可选瑕疵类型是否启用
        DefectType defectType = static_cast<DefectType>(defect.id);

        if (!m_defectTypeUsability[defectType])
        {
            typesToRemove.insert(defect.id);
        }
    }
    for (const int& type : typesToRemove)
    {
        m_EditRecipeConfig.removeDefect(type);
    }

	if (m_EditRecipeConfig.defects.isEmpty())
	{
		FILE_LOG_ERROR("Load Recipe Parameter Failed.");
		return ;
	}

	//根据设置的瑕疵类型显示标志，设置每个类型对应图例的显示状态
	Recipe_UpdateDisplayFlagToRecipeConfig(m_EditRecipeConfig);
}

void ConfigWidget::Recipe_ExportParameter(const QString& filePath)
{
	if (m_EditRecipeConfig.saveAs(filePath))
	{
		FILE_LOG_INFO("Export Recipe Parameter Success.");
	}
}

QStandardItem* ConfigWidget::Recipe_CreateDoubleItem(double value, double min, double max, double step, int decimals)
{
	QStandardItem* item = new QStandardItem;
	item->setData(value, Qt::EditRole);
	item->setData(min, Qt::UserRole + 10);
	item->setData(max, Qt::UserRole + 11);
	item->setData(step, Qt::UserRole + 12);
	item->setData(decimals, Qt::UserRole + 13);
	return item;
}

void ConfigWidget::Recipe_AdjustTableSpans(int defectCount)
{
	int row = defectCount;

	// 合并距离：占用后4列
	ui->TableView_Recipe->setSpan(row++, 2, 1, 4);

	// 映射比例、置信度等：占用后5列
	ui->TableView_Recipe->setSpan(row++, 1, 1, 5);
	ui->TableView_Recipe->setSpan(row++, 1, 1, 5);
	ui->TableView_Recipe->setSpan(row++, 1, 1, 5);
	ui->TableView_Recipe->setSpan(row++, 1, 1, 5);
	ui->TableView_Recipe->setSpan(row++, 1, 1, 5); // 路径
}

