#include "ImageViewWidget.h"
#include <optional> //用于队列的出队操作
#include <QtCharts>  // 使用统一头文件代替单独包含
#include "Log.hpp"
#include "Json/glassData2Json.h"    //Json操作类。用于读取历史数据的json信息使用。
#include "HDF5/HDF5Utils.h"         //读取HDF5文件

#include "ChartManager.h"           //图表管理类
#include "CustomTableModel.h"       //自定义表格模型类
#include "DisplayMessage.h"         //界面显示运行信息

#include "Global.h"

ImageViewWidget::ImageViewWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::ImageViewWidgetClass())
{
	ui->setupUi(this);

    if (IsDoubleImage)
    {
        ui->graphicsView_Last->setVisible(true);
    }
    else
    {
        ui->graphicsView_Last->setVisible(false);
    }
}

ImageViewWidget::~ImageViewWidget()
{
    //std::cout << "ImageViewWidget::~ImageViewWidget()" << std::endl;
	delete ui;
}

void ImageViewWidget::Slot_ShowHistory_Chart(const QMap<DefectType, QMap<QDate, int>>& data)
{
    HistoryRecordChart_ShowData(data);
}

void ImageViewWidget::Slot_SetDefectTypeVisibility(DefectType type, bool visible)
{
    SetDefectTypeVisibilityFunction(type, visible);
}

void ImageViewWidget::Slot_SetDefectLevelVisibility(DefectLevel level, bool visible)
{

    SetDefectLevelVisibilityFunction(level, visible);
}

void ImageViewWidget::SetDefectTypeVisibilityFunction(DefectType type, bool visible)
{
    std::string defectTypeStr;
    defectTypeStr = DefectTypeToString(type).toStdString();

    std::string showStr;
    if (visible)
    {
        showStr = "true";
    }
    else
    {
        showStr = "false";
    }
    FILE_LOG_INFO("Type = %s, show = %s", defectTypeStr.c_str(), showStr.c_str());
    //History场景
    ui->graphicsView_His->SetDefectVisibility(type, visible);
    //Last场景
    ui->graphicsView_Last->SetDefectVisibility(type, visible);
    //Real场景
    ui->graphicsView_Real->SetDefectVisibility(type, visible);
}


void ImageViewWidget::SetDefectLevelVisibilityFunction(DefectLevel level, bool visible)
{
    std::string defectLevelStr;
    defectLevelStr = DefectLevelToString(level).toStdString();

    std::string showStr;
    if (visible)
    {
        showStr = "true";
    }
    else
    {
        showStr = "false";
    }
    FILE_LOG_INFO("Level = %s, show = %s", defectLevelStr.c_str(), showStr.c_str());

    //History场景
    if (ui->graphicsView_His)
    {
        ui->graphicsView_His->SetDefectVisibility(level, visible);
    }

    //Last场景
    ui->graphicsView_Last->SetDefectVisibility(level, visible);
    //Real场景
    ui->graphicsView_Real->SetDefectVisibility(level, visible);
}
//
////出队操作
//void ImageViewWidget::Dequeue()
//{
//    FileLogPrintf("Dequeue: Begin.");
//    int queueSize = 0;
//    int autoShowValue = 0;
//    std::optional<QueueDefectItem> item;
//    {
//        QMutexLocker locker(&m_queueMutex);
//        // 出队
//        if (!m_queueDefect.empty())
//        {
//            item = std::move(m_queueDefect.dequeue());
//        }
//        else
//        {
//            m_autoShowImage = 1;
//        }
//        queueSize = m_queueDefect.size();
//        autoShowValue = m_autoShowImage;
//    }
//
//    if (item)
//    {
//        auto&& defectImages_0 = std::move(item->images_ch0);
//        auto&& defectImages_1 = std::move(item->images_ch1);
//        auto&& defectImages_2 = std::move(item->images_ch2);
//        auto&& showInfo = std::move(item->drawInfo);
//        auto&& backGround = std::move(item->backgroundImage);
//        auto&& timeProduce = std::move(item->timeProduce);
//        ShowCurrentImage(
//            defectImages_0,
//            defectImages_1,
//            defectImages_2,
//            backGround,
//            showInfo,
//            timeProduce,
//            item->scale,
//            item->GlassPhysicalWidth,
//            item->GlassPhysicalHeight,
//            item->GlassPixelWidth,
//            item->GlassPixelHeight
//        );
//    }
//    else
//    {
//        emit Signal_ClearDisplaySignal();
//    }
//
//    emit Signal_UpdateImageCount(queueSize, autoShowValue);
//    FileLogPrintf("Dequeue: Completed.");
//}

std::vector<drawInformation> ImageViewWidget::GetDrawInformations(GraphicsView* graphicsview) const
{
    /* 获取QGraphicsItems */
    auto items = graphicsview->scene()->items();

    std::vector<drawInformation> infos;
    for (auto it = items.begin(); it != items.end(); ++it)
    {
        QVariant var = (*it)->data(0);
        if (var.isNull())
            continue;

        drawInformation info = var.value<drawInformation>();
        infos.emplace_back(info);
    }

    return infos;
}

void ImageViewWidget::SaveAndClearItem(GraphicsView* graphicsview, bool isClear)
{
    //获取当前是否存在修改
    bool isModify = graphicsview->GetModifiedFlag();

    if (isModify || graphicsview->m_primaryKey != "")
    {
        //获取当前信息列表
        auto mergeInfo = GetDrawInformations(graphicsview);
        if (!mergeInfo.empty())
        {
            //做了更改就拿主键查找，然后进行更新
            /*DataSave dataSave;
            dataSave(m_db, mergeInfo, graphicsview->m_primaryKey);*/
        }

        graphicsview->SetModifyFlag(false);
    }

    //清空QGraphicsView中的内容
    if (isClear)
    {
        emit graphicsview->Signal_ClearRectItem();
    }
}

void ImageViewWidget::ShowCurrentImage(
    std::vector<cv::Mat>& currentImage_0,
    std::vector<cv::Mat>& currentImage_1,
    std::vector<cv::Mat>& currentImage_2,
    cv::Mat& backgroundImage,
    cv::Mat& pseudoColorImage,
    std::vector<drawInformation>& showInfo,
    const std::string& timeProduce,
    const double& scale,
    const float& glassPhysicalWidth,
    const float& glassPhysicalHeight,
    const float& glassPixelWidth,
    const float& glassPixelHeight
)
{
    FILE_LOG_INFO("ShowCurrentImage: Begin.");

    SaveAndClearItem(ui->graphicsView_Last);      //清空左侧场景的内容
	if (IsDoubleImage)  //判断是否是双图显示模式
    {
        if (!IsPseudoColorImage)
        {
            SaveAndClearItem(ui->graphicsView_Real, false);    //因为还要向其他窗口转移内容，所以不进行清除
            //step1. 先移动右侧图像到左侧场景
            //20251031：移除将右图转移到左图的操作。左图直接显示像素图
            ui->graphicsView_Real->MoveTo(ui->graphicsView_Last, scale, scale/*, m_glassPhysicalWidth_Last, m_glassPhysicalHeight_Last*/);

            m_results_Last = std::move(m_results_Real);

            m_glassPixelWidth_Last = m_glassPixelWidth_Real;
            m_glassPixelHeight_Last = m_glassPixelHeight_Real;

            m_glassPhysicalWidth_Last = m_glassPhysicalWidth_Real;
            m_glassPhysicalHeight_Last = m_glassPhysicalHeight_Real;
        }
        //step1. 显示像素图到场景
        else
        {
            SaveAndClearItem(ui->graphicsView_Real);     //先清右侧1图像
            if (pseudoColorImage.empty())
            {
                FILE_LOG_ERROR("ShowCurrentImage error: CurrentImage is empty!");
                m_results_Real.clear();
                //emit ui->graphicsView_Real->Signal_UpdateDefectNumShow(); //刷新缺陷数显示
                DisplayMessage::getInstance().logMessage(tr("PseudoColorImage is empty"), tr("Current"));  //图像为空
                return;
            }

            QPixmap pixmap_pseudoColor = QPixmap::fromImage(Mat2QImage(pseudoColorImage));
            if (pixmap_pseudoColor.isNull())
            {
                FILE_LOG_ERROR("ShowCurrentImage error: The image does not exist!");
                
                //emit ui->graphicsView_Real->Signal_UpdateDefectNumShow();
                DisplayMessage::getInstance().logMessage(tr("Glass image processing failed"), tr("Current"));  //玻璃图像处理失败
                return;
            }
            emit ui->graphicsView_Last->Signal_AddPixmapItem(pixmap_pseudoColor, glassPixelWidth, glassPixelHeight, glassPhysicalWidth, glassPhysicalHeight);

        }
    }
    else
    {
        SaveAndClearItem(ui->graphicsView_Real);      //清空右侧场景的内容
    }




    m_glassPixelWidth_Real = glassPixelWidth;
    m_glassPixelHeight_Real = glassPixelHeight;

    m_glassPhysicalWidth_Real = glassPhysicalWidth;
    m_glassPhysicalHeight_Real = glassPhysicalHeight;
    //m_results_right.clear();    //不确定算法内部是否有clear操作，此处单独执行一遍。
    //FileLogPrintf("ShowCurrentImage: MoveTo Left Completed.");

    //step2. 显示虚拟背景图到场景
    if (backgroundImage.empty())
    {
        FILE_LOG_ERROR("ShowCurrentImage error: CurrentImage is empty!");
        SaveAndClearItem(ui->graphicsView_Real);   //空图时清空场景
        m_results_Real.clear();
        //emit ui->graphicsView_Real->Signal_UpdateDefectNumShow(); //刷新缺陷数显示
        DisplayMessage::getInstance().logMessage(tr("Image is empty"), tr("Current"));  //图像为空
        return;
    }
    QPixmap pixmap = QPixmap::fromImage(Mat2QImage(backgroundImage));
    if (pixmap.isNull())
    {
        FILE_LOG_ERROR("ShowCurrentImage error: The image does not exist!");
        SaveAndClearItem(ui->graphicsView_Real);   //空图时清空场景
        m_results_Real.clear();
        //emit ui->graphicsView_Real->Signal_UpdateDefectNumShow();
        DisplayMessage::getInstance().logMessage(tr("Glass image processing failed"), tr("Current"));  //玻璃图像处理失败
        return;
    }
    emit ui->graphicsView_Real->Signal_AddPixmapItem(pixmap, m_glassPixelWidth_Real,m_glassPixelHeight_Real, m_glassPhysicalWidth_Real, m_glassPhysicalHeight_Real);
    ui->graphicsView_Real->m_primaryKey = timeProduce;

    m_results_Real = std::move(showInfo);

    FILE_LOG_INFO("ShowCurrentImage: Show backgroundImage Completed.");

    //step3. 显示缺陷信息到场景
    if (!m_results_Real.empty())
    {
        int results_size = m_results_Real.size();
        if (results_size != currentImage_0.size() || results_size != currentImage_1.size() || results_size != currentImage_2.size())
        {
            results_size = std::min(m_results_Real.size(), std::min(currentImage_0.size(), std::min(currentImage_1.size(), currentImage_2.size())));
        }
        if (results_size <= 0)
        {
            return;
        }
        bool isLastItem = false;
        for (int i = 0; i < results_size; i++)
        {
            if (isLastItem)
            {
                break;
            }

            cv::Rect colorRect;

            //防护动作，避免ROI越界造成崩溃
            //cv::Rect rect = m_results_Real[i].realRect;
            //if (rect.x < 0) rect.x = 0;
            //if (rect.y < 0) rect.y = 0;
            //if (rect.x + rect.width > glassPixelWidth) {
            //    rect.width = (glassPixelWidth - 1) - rect.x;
            //}
            //if (rect.y + rect.height > glassPixelHeight) {
            //    rect.height = (glassPixelHeight - 1) - rect.y;
            //}
            //// 确保矩形有效
            //if (rect.width <= 0 || rect.height <= 0)
            //{
            //    continue;
            //}
            if (i == results_size - 1)
            {
                isLastItem = true;
            }
            emit ui->graphicsView_Real->Signal_AddRectItem(
                m_results_Real[i], scale, scale, currentImage_0[i], currentImage_1[i], currentImage_2[i], isLastItem);
        }

    }
    FILE_LOG_INFO("ShowCurrentImage: Show Legend Completed.");

    //step4. 刷新界面右侧瑕疵统计
    //emit ui->graphicsView_Real->Signal_UpdateDefectNumShow();

    /******显示结果的事件记录和玻璃尺寸记录，暂时屏蔽20250810*********/
    ////step5. 显示检测结果时间到界面
    //m_dateTime = QDateTime::fromString(QString::fromStdString(timeProduce).left(14), "yyyyMMddhhmmss");
    //emit ChangeTitleTimeSinals(1, m_dateTime);

    //setp6. 显示玻璃尺寸到界面顶部指定区域

    //ui->GlassSize_lab->setText(QString("%1 * %2").arg((int)m_glassPhysicalWidth_Real).arg((int)m_glassPhysicalHeight_Real));

    /************************************************************/

    DisplayMessage::getInstance().logMessage(tr("Image displayed"), tr("Current"));  //图像已显示
    FILE_LOG_INFO("ShowCurrentImage: Completed.");
}


void ImageViewWidget::ShowHistoryImage(QString originalTime, const QString& filesPath, const QDateTime& dateTime,
    const float& glassPhysicalWidth, const float& glassPhysicalHeight)
{
    FILE_LOG_INFO("Show_History_Image_begin.");
    SaveAndClearItem(ui->graphicsView_His, true);
    if (ui->tabWidget_view->currentIndex()!=0)  //切换到历史图像
    {
        ui->tabWidget_view->setCurrentIndex(0);
    }
    QString jsonPath = filesPath + ".json";
    QString h5Path_0 = filesPath + "_detectImages_0.h5";
    QString h5Path_1 = filesPath + "_detectImages_1.h5";
    QString h5Path_2 = filesPath + "_detectImages_2.h5";
    QString backGroundImagePath = filesPath + "backGroundImage.jpg";
    ////验证图片和json文件是否存在
    QFile fileJson(jsonPath);
    if (!fileJson.exists())
    {
        //QMessageBox::warning(this, tr("Warning"), tr("The json does not exist!"));
        FILE_LOG_ERROR("ShowHistoryImage error: The json does not exist!");
        //emit ui->graphicsView_His->Signal_UpdateDefectNumShow();
        DisplayMessage::getInstance().logMessage(tr("Defect data file not found"), tr("History"));  //缺陷数据文件未找到
        return;
    }
    //读取json文件
    glassData2Json json;
    m_results_His.clear();
    m_results_His = json.parseJsonToDrawInfos(jsonPath.toStdString(), m_glassPixelWidth_His, m_glassPixelHeight_His);
    FILE_LOG_INFO("ShowHistoryImage: read json Completed!");

    std::vector<cv::Mat>defectImg_ch0;
    std::vector<cv::Mat>defectImg_ch1;
    std::vector<cv::Mat>defectImg_ch2;
    if (m_results_His.size() > 0)
    {

        QFile fileH5_0(h5Path_0);
        if (!fileH5_0.exists())
        {
            //QMessageBox::warning(this, tr("Warning"), tr("The image does not exist!"));
            FILE_LOG_ERROR("ShowHistoryImage error: The fileH5_0 does not exist!");
            SaveAndClearItem(ui->graphicsView_His);   //空图时清空场景
            DisplayMessage::getInstance().logMessage(tr("Defect image file not found"), tr("History"));  //缺陷图像文件未找到
            return;
        }
        QFile fileH5_1(h5Path_1);
        if (!fileH5_1.exists())
        {
            //QMessageBox::warning(this, tr("Warning"), tr("The image does not exist!"));
            FILE_LOG_ERROR("ShowHistoryImage error: The fileH5_1 does not exist!");
            SaveAndClearItem(ui->graphicsView_His);   //空图时清空场景
            DisplayMessage::getInstance().logMessage(tr("Defect image file not found"), tr("History"));  //缺陷图像文件未找到
            return;
        }
        QFile fileH5_2(h5Path_2);
        if (!fileH5_2.exists())
        {
            //QMessageBox::warning(this, tr("Warning"), tr("The image does not exist!"));
            FILE_LOG_ERROR("ShowHistoryImage error: The fileH5_2 does not exist!");
            SaveAndClearItem(ui->graphicsView_His);   //空图时清空场景
            DisplayMessage::getInstance().logMessage(tr("Defect image file not found"),tr("History"));  //缺陷图像文件未找到
            return;
        }
        //读取H5文件
        HDF5Utils hdf5Utils;
        defectImg_ch0 = hdf5Utils.loadImagesFromHDF5(h5Path_0.toStdString());
        defectImg_ch1 = hdf5Utils.loadImagesFromHDF5(h5Path_1.toStdString());
        defectImg_ch2 = hdf5Utils.loadImagesFromHDF5(h5Path_2.toStdString());
    }
    QFile fileBackGroundImage(backGroundImagePath);
    if (!fileBackGroundImage.exists())
    {
        //QMessageBox::warning(this, tr("Warning"), tr("The image does not exist!"));
        FILE_LOG_ERROR("ShowHistoryImage error: The fileBackGroundImage does not exist!");
        SaveAndClearItem(ui->graphicsView_His);   //空图时清空场景
        DisplayMessage::getInstance().logMessage(tr("Defect image file not found"), tr("History"));  //缺陷图像文件未找到
        return;
    }
    //读取图片

    cv::Mat historyImage_BackGround = cv::imread(backGroundImagePath.toStdString(), 0);

    FILE_LOG_INFO("ShowHistoryImage: read images Completed!");

    /* 这里记录主键 */
    ui->graphicsView_His->m_primaryKey = originalTime.toStdString();    //使用ui->graphicsView_His显示历史记录

    //// 获取示例图相对于原图的缩放比
    double scale = m_generalMethod.getScale(historyImage_BackGround.cols, historyImage_BackGround.rows);
    QPixmap pixmap = QPixmap::fromImage(Mat2QImage(historyImage_BackGround));
    if (pixmap.isNull())
    {
        //QMessageBox::warning(this, tr("Warning"), tr("The image does not exist!"));
        FILE_LOG_ERROR("ShowHistoryImage error: The pixmap image does not exist!");
        SaveAndClearItem(ui->graphicsView_His);   //空图时清空场景
        DisplayMessage::getInstance().logMessage(tr("Glass image processing failed"), tr("History"));  //玻璃图像文件处理失败
        return;
    }
    else
    {
        emit ui->graphicsView_His->Signal_AddPixmapItem(pixmap, m_glassPixelWidth_His, m_glassPixelHeight_His, glassPhysicalWidth, glassPhysicalHeight);
    }

    ////////GlassWidth_His = glassPhysicalWidth;
    ////////GlassHeight_His = glassPhysicalHeight;
    ////////ui->GlassSize_lab->setText(QString("%1 * %2").arg((int)GlassWidth_His).arg((int)GlassHeight_His));  //显示记录的玻璃尺寸
    ////////m_dateTime_His = dateTime;
    ////////emit ChangeTitleTimeSinals(0, m_dateTime_His);  //显示历史记录时间

    if (m_results_His.size() != defectImg_ch0.size()
        || m_results_His.size() != defectImg_ch1.size()
        || m_results_His.size() != defectImg_ch2.size())
    {
        FILE_LOG_ERROR("ShowHistoryImage error: Check json and imags size error: Json:%d, img_0:%d, img_1:%d, img_2:%d",
            m_results_His.size(),
            defectImg_ch0.size(),
            defectImg_ch1.size(),
            defectImg_ch2.size()
        );

        FILE_LOG_INFO("ShowHistoryImage: Completed.");
        DisplayMessage::getInstance().logMessage(tr("Invalid defect data"), tr("History"));  //无效缺陷数据
        return;
    }

    ///*2025050602 对缺陷等级进行排序，解决图例显示红色图例在下的问题*/
    //m_results_His = filterAndSortDefects(drawInfos);
    ///*2025050602 对缺陷等级进行排序，解决图例显示红色图例在下的问题*/

    // 按照DefectId进行排序
    std::sort(m_results_His.begin(), m_results_His.end(),
        [](const drawInformation& a, const drawInformation& b) {
            return a.DefectId < b.DefectId; // 按id升序排列
        });

    QMap<DefectType, int> countMap;

    int glassLevel = 0;

    if (m_results_His.size() < 1)
    {
        //QMessageBox::warning(this, tr("Tips"), tr("No Defects!"));
        FILE_LOG_INFO("ShowHistoryImage warning: No Defects!");
        //emit ui->graphicsView_His->Signal_UpdateDefectNumShow();
    }
    else
    {
        //step3. 显示缺陷信息到场景
        int results_size = m_results_His.size();
        if (results_size != defectImg_ch0.size() || results_size != defectImg_ch1.size() || results_size != defectImg_ch2.size())
        {
            results_size = std::min(m_results_His.size(), std::min(defectImg_ch0.size(), std::min(defectImg_ch1.size(), defectImg_ch2.size())));
        }
        if (results_size > 0)
        {
            bool isLastItem = false;
            int imageId = -1;
            for (int i = 0; i < results_size; i++)
            {
                if (isLastItem)
                {
                    break;
                }

                cv::Rect colorRect;

                //防护动作，避免ROI越界造成崩溃
                cv::Rect rect = m_results_His[i].realRect;
                if (rect.x < 0) rect.x = 0;
                if (rect.y < 0) rect.y = 0;
                if (rect.x + rect.width >= m_glassPixelWidth_His) {
                    rect.width = (m_glassPixelWidth_His - 1) - rect.x;
                }
                if (rect.y + rect.height >= m_glassPixelHeight_His) {
                    rect.height = (m_glassPixelHeight_His - 1) - rect.y;
                }
                // 确保矩形有效
                if (rect.width <= 0 || rect.height <= 0)
                {
                    continue;
                }
                imageId = m_results_His[i].DefectId;
                ////////cv::Rect tempRect = m_generalMethod.computeCropRect2Color(rect, m_glassPixelWidth_His, m_glassPixelHeight_His, 256, &colorRect);

                ////////cv::Mat connected = m_generalMethod.segUtils.GetDrawImage(defectImg_ch2[imageId], 20, 0);
                ////////// 定义颜色映射
                ////////static const std::vector<cv::Scalar> color_map = {
                ////////    cv::Scalar(0, 255, 0),    // 绿色 - level 0
                ////////    cv::Scalar(0, 255, 255),  // 黄色 - level 1
                ////////    cv::Scalar(0, 0, 255)     // 红色 - level 2
                ////////};
                ////////int level = 0;
                ////////// 确保level在有效范围内
                ////////int color_index = std::clamp(level, 0, static_cast<int>(color_map.size() - 1));
                ////////cv::Scalar color = color_map[color_index];

                //////////MINOR,//轻微缺陷  1
                //////////MEDIUM,//中等缺陷 2
                //////////SERIOUS,//严重缺陷 3
                ////////if (m_results_His[i].ErrorType == MEDIUM) {
                ////////    level = 1;

                ////////}
                ////////else if (m_results_His[i].ErrorType == SERIOUS) {
                ////////    level = 2;
                ////////}
                ////////// 在原图上绘制边缘
                ////////cv::Mat rectImage0 = m_generalMethod.segUtils.drawEdgesOnImage(defectImg_ch0[i], connected, colorRect, color, level);
                ////////cv::Mat rectImage1 = m_generalMethod.segUtils.drawEdgesOnImage(defectImg_ch1[i], connected, colorRect, color, level);
                ////////cv::Mat rectImage2 = m_generalMethod.segUtils.drawEdgesOnImage(defectImg_ch2[i], connected, colorRect, color, level);

                //cv::Mat rectImage0 = m_generalMethod.drawSemiTransparentRedOverlayWithCircle(currentImage_0[i], colorRect, 0.1);
                //cv::Mat rectImage1 = m_generalMethod.drawSemiTransparentRedOverlayWithCircle(currentImage_1[i], colorRect, 0.1);
                //cv::Mat rectImage2 = m_generalMethod.drawSemiTransparentRedOverlayWithCircle(currentImage_2[i], colorRect, 0.1);

                if (i == results_size - 1)
                {
                    isLastItem = true;
                }
                emit ui->graphicsView_His->Signal_AddRectItem(
                    m_results_His[i], scale, scale, defectImg_ch0[i], defectImg_ch1[i], defectImg_ch2[i], isLastItem);
            }
        }

        FILE_LOG_INFO("ShowHistoryImage: Add Rect Item Completed.");
    }

    FILE_LOG_INFO("ShowHistoryImage: Completed.");
    DisplayMessage::getInstance().logMessage(tr("Image displayed"), tr("History"));  //图像已显示
}

// 清空显示处理函数
void ImageViewWidget::Slot_HandleClearDisplay()
{
    //当空队列时，执行显示动作时，实际执行清空图像动作
    SaveAndClearItem(ui->graphicsView_Real, true); //清空右侧场景中的图像
    SaveAndClearItem(ui->graphicsView_Last, true); //清空右侧场景中的图像
}

void ImageViewWidget::ShowgLegend(GraphicsView* graphicsView)
{

    ////仅执行右侧场景的图例显示刷新
    // /* 正常缺陷类型 */
    //emit graphicsView->Signal_UpdateLevelShow(DefectLevel::ABNORMAL, ui->checkBox_abnormalLevel->isChecked());

    ///* 轻微缺陷等级 */
    //emit graphicsView->Signal_UpdateLevelShow(DefectLevel::MINOR, ui->checkBox_minLevel->isChecked());

    ///* 中等缺陷等级 */
    //emit graphicsView->Signal_UpdateLevelShow(DefectLevel::MEDIUM, ui->checkBox_mediumLevel->isChecked());

    ///* 严重缺陷等级 */
    //emit graphicsView->Signal_UpdateLevelShow(DefectLevel::SERIOUS, ui->checkBox_seriousLevel->isChecked());

    ///* 链接异常缺陷等级 */
    //emit graphicsView->Signal_UpdateLevelShow(DefectLevel::ABNORMAL, ui->checkBox_abnormalLevel->isChecked());

    ///* 区域错误缺陷等级 */
    //emit graphicsView->Signal_UpdateLevelShow(DefectLevel::AREAERROR, ui->checkBox_areaerrorLevel->isChecked());

    ////链接缺陷类型槽函数
    ///* 镀膜不良 */
    //emit graphicsView->Signal_UpdateTypeShow(DefectType::TYPE_POORCOATING, ui->checkBox_poorCoatingType->isChecked());

    ///* 划伤 */
    //emit graphicsView->Signal_UpdateTypeShow(DefectType::TYPE_SCRATCH, ui->checkBox_scratchType->isChecked());

    ///* 结石 */
    //emit graphicsView->Signal_UpdateTypeShow(DefectType::TYPE_CALCULUS, ui->checkBox_calculusType->isChecked());

    ///* 气泡 */
    //emit graphicsView->Signal_UpdateTypeShow(DefectType::TYPE_BUBBLE, ui->checkBox_bubbleType->isChecked());

    ///* 商标 */
    //emit graphicsView->Signal_UpdateTypeShow(DefectType::TYPE_TRADEMARK, ui->checkBox_trademarkType->isChecked());

    ///* 水渍 */
    //emit graphicsView->Signal_UpdateTypeShow(DefectType::TYPE_WATERSTAIN, ui->checkBox_waterStain->isChecked());

    ///* 脏污 */
    //emit graphicsView->Signal_UpdateTypeShow(DefectType::TYPE_SMUDGE, ui->checkBox_smudgeType->isChecked());

    ///* 丝印 */
    //emit graphicsView->Signal_UpdateTypeShow(DefectType::TYPE_SCREENPRINTING, ui->checkBox_ScreenPrintingType->isChecked());

}


// 生成折线图
void ImageViewWidget::HistoryRecordChart_ShowData(const QMap<DefectType, QMap<QDate, int>>& data)
{

    //折线颜色列表
    QList<QColor> colorList = {
    QColor(Qt::red),
    QColor(255, 165, 0), // 橙色（RGB）
    QColor(Qt::yellow),
    QColor(Qt::green),
    QColor(Qt::cyan),
    QColor(Qt::blue),
    QColor(128, 0, 128)  // 紫色（RGB）
    };

    int colorIndex = 0;

    QChart* chart = new QChart();
    chart->setTitle(tr("Defect Trend Statistics"));

    // 纵轴（Y轴）
    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText(tr("Defect Count"));
    axisY->setLabelFormat("%d");

    // 计算Y轴最大值（最大值 * 1.1）
    int maxY = 0;
    for (const auto& defectData : data) {
        for (int count : defectData.values()) {
            if (count > maxY) maxY = count;
        }
    }
    axisY->setRange(0, maxY * 1.1);
    chart->addAxis(axisY, Qt::AlignLeft);

    // 横轴（X轴）
    QCategoryAxis* axisX = new QCategoryAxis();
    axisX->setTitleText(tr("Date"));
    axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    axisX->setLabelsAngle(90);

    // 统一所有日期，并添加额外的空白边界
    QSet<QDate> allDates;
    for (const auto& dateMap : data) {
        //allDates.unite(dateMap.keys().toSet());   //Qt5.14.2下提示C4996 已弃用接口。
        for (const QDate& date : dateMap.keys()) {
            allDates.insert(date);
        }
    }

    QList<QDate> sortedDates = allDates.values();
    std::sort(sortedDates.begin(), sortedDates.end());

    // 在两侧添加空白
    if (!sortedDates.isEmpty()) {
        sortedDates.prepend(sortedDates.front().addDays(-1));
        sortedDates.append(sortedDates.back().addDays(1));
    }

    QHash<QDate, int> dateIndexMap;
    for (int i = 0; i < sortedDates.size(); ++i) {
        dateIndexMap[sortedDates[i]] = i;
        axisX->append(sortedDates[i].toString("yyyy-MM-dd"), i);
    }
    axisX->setRange(0, sortedDates.size() - 1);
    chart->addAxis(axisX, Qt::AlignBottom);

    QLegend* legend = chart->legend();
    legend->setAlignment(Qt::AlignRight); // 可按需设为 AlignBottom、AlignTop、AlignLeft
    legend->setFont(QFont("Microsoft YaHei", 10)); // 字体略大些，看起来更有间距
    legend->setContentsMargins(10, 10, 10, 10);    // 图例整体边距，模拟内边距效果

    for (auto it = data.begin(); it != data.end(); ++it) {
        DefectType type = it.key();
        QMap<QDate, int> dateMap = it.value();

        QLineSeries* series = new QLineSeries();
        QScatterSeries* scatterSeries = new QScatterSeries(); // 添加散点系列
        scatterSeries->setMarkerShape(QScatterSeries::MarkerShapeCircle); // 圆形
        scatterSeries->setMarkerSize(8.0); // 设置点的大小
        scatterSeries->setPen(QPen(Qt::black, 1)); // 黑色边框
        scatterSeries->setBrush(QBrush(Qt::white)); // 空心（白色填充）
        scatterSeries->setName(""); // 不在图例中显示

        series->setName(QString("%1").arg(DefectTypeToString(type)));
        series->setPointLabelsVisible(true);
        series->setPointLabelsFormat(QString("@yPoint")); // 只显示 Y 轴值

        QColor color = colorList[colorIndex % colorList.size()];
        series->setPen(QPen(color, 2)); // 折线颜色
        scatterSeries->setBrush(QBrush(color)); // 点的颜色

        ++colorIndex;

        for (auto dateIt = dateMap.begin(); dateIt != dateMap.end(); ++dateIt) {
            int x = dateIndexMap[dateIt.key()];
            int y = dateIt.value();
            series->append(x, y);
            scatterSeries->append(x, y); // 散点和折线同步
        }

        chart->addSeries(series);
        chart->addSeries(scatterSeries); // 添加散点
        series->attachAxis(axisX);
        series->attachAxis(axisY);
        scatterSeries->attachAxis(axisX);
        scatterSeries->attachAxis(axisY);

        //空点隐藏
        auto markers = chart->legend()->markers(scatterSeries);
        for (QLegendMarker* marker : markers) {
            marker->setVisible(false);
        }

        // 遍历每个图例项，调整字体与前缀空格来“模拟间距”
        for (QLegendMarker* marker : legend->markers()) {
            // 添加前缀空格，拉开视觉距离（可根据实际情况调整空格数量）
            marker->setLabel("   " + marker->label());
        }


    }

    // 渲染
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QWidget* oldWidget = ui->tabWidget_view->widget(3);
    ui->tabWidget_view->removeTab(3);
    delete oldWidget;  // 安全删除旧控件

    ui->tabWidget_view->insertTab(3, chartView, tr("Historical Analysis"));
    ui->tabWidget_view->setCurrentIndex(3);
    ui->tabWidget_view->update();


}

void ImageViewWidget::SetDefaultTabIndex(int index)
{
    ui->tabWidget_view->setCurrentIndex(index);
}