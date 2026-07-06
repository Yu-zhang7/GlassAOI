#pragma once

#include <QWidget>
#include "ui_ImageViewWidget.h"
#include "Global.h"
#include "GeneralMethod.h"  //通用方法类。用于处理图像

// 定义共享指针类型
using QueueDefectItemPtr = std::shared_ptr<QueueDefectItem>;


QT_BEGIN_NAMESPACE
namespace Ui { class ImageViewWidgetClass; };
QT_END_NAMESPACE

class ImageViewWidget : public QWidget
{
	Q_OBJECT

public:
	ImageViewWidget(QWidget *parent = nullptr);
	~ImageViewWidget();


private:
	Ui::ImageViewWidgetClass* ui;

public:
	/* 显示当次图像和结果 */
	void ShowCurrentImage(std::vector<cv::Mat>& currentImage_0, std::vector<cv::Mat>& currentImage_1,
		std::vector<cv::Mat>& currentImage_2, cv::Mat& backgroundImage, cv::Mat& PseudoColorImage, std::vector<drawInformation>& showInfo,
		const std::string& timeProduce, const double& scale, const float& glassPhysicalWidth, const float& glassPhysicalHeight,
		const float& glassPixelWidth, const float& glassPixelHeight);

	/* 显示历史数据和结果 */
	void ShowHistoryImage(QString originalTime, const QString& filesPath, const QDateTime& dateTime, 
		const float& glassPhysicalWidth, const float& glassPhysicalHeight);

	/* 显示历史数据和结果 */
	//void ShowHistoryImage(const QString& filesPath, const QDateTime& dateTime, 
	//	const float& glassPhysicalWidth, const float& glassPhysicalHeight);

	/* 设置缺陷类型是否显示 */
	void SetDefectTypeVisibilityFunction(DefectType type, bool visible);

	/* 设置缺陷级别是否显示 */
	void SetDefectLevelVisibilityFunction(DefectLevel level, bool visible);

	/* 设置默认显示tab页 */
	void SetDefaultTabIndex(int index);

signals:
	/* 图像出队列，并显示到界面的信号*/
	void Signal_ImageDeQueue();

	/* 显示图像数量信号 */
	void Signal_UpdateImageCount(int count, int autoShow);

	/* 清空显示信号 */
	void Signal_ClearDisplaySignal();

public slots:
	///* 入队槽函数 */
	//void Slot_Enqueue(const QueueDefectItemPtr& resultItem);

	///* 出队槽函数*/
	//void Slot_Dequeue();

	/* 显示历史数据折线图 槽函数*/
	void Slot_ShowHistory_Chart(const QMap<DefectType, QMap<QDate, int>>& data);

	/* 指定缺陷类型是否显示 槽函数*/
	void Slot_SetDefectTypeVisibility(DefectType type, bool visible);

	/* 指定缺陷级别是否显示 槽函数*/
	void Slot_SetDefectLevelVisibility(DefectLevel level, bool visible);

	/* 清空current图像 槽函数*/
	void Slot_HandleClearDisplay();
private:

	///* 入队操作 */
	//void Enqueue(const QueueDefectItemPtr& resultItem);

	///* 出队操作 */
	//void Dequeue();

	/* 获取当前QGraphicsView中所有的缺陷信息组成的结构体 */
	std::vector<drawInformation> GetDrawInformations(GraphicsView* graphicsview) const;

	/* 保存当前修改并清空item的函数 */
	void SaveAndClearItem(GraphicsView* graphicsview, bool isClear = true);

	/* 场景根据勾选的图例情况，显示相应的瑕疵 */
	void ShowgLegend(GraphicsView* graphicsView);

	/* 刷新显示指定数据集(时间段等)的折线图 */
	void HistoryRecordChart_ShowData(const QMap<DefectType, QMap<QDate, int>>& data);

private:

	GeneralMethod					m_generalMethod;					//图像处理通用方法类
	/* 图像队列操作*/
	//QMutex                          m_queueMutex;						//互斥锁。用于队列操作
	//QQueue<QueueDefectItem>         m_queueDefect;						//图像数据队列
	bool                            m_autoShowImage;					//是否自动显示图像

};

