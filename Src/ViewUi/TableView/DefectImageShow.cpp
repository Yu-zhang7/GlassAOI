#include "DefectImageShow.h"
#include <QStyle>
#include "Log.hpp"
#include "Global.h"
#include "ConfigManager.h"

#include "Json/glassData2Json.h"    //Json操作类。用于读取历史数据的json信息使用。
#include "HDF5/HDF5Utils.h"         //读取HDF5文件
#include "DisplayMessage.h"         //界面显示运行信息

DefectImageShow::DefectImageShow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DefectImageShowClass())
{
    ui->setupUi(this);

    // 设置窗口标题栏不显示问号按钮
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    //固定弹窗尺寸不能被修改
    setFixedSize(size());

    InitDefectShowStatus();


	if (!IsPseudoColorImage)    //非伪彩色图像模式下，隐藏伪彩色图像标签页
    {
        ui->tabWidget->setTabEnabled(1, false);
        ui->tabWidget->setStyleSheet("QTabBar::tab:disabled { width: 0; height: 0; margin: 0; padding: 0; border: none; color: transparent; }");
    }
    else
    {
        ui->tabWidget->setTabEnabled(1, true);  //伪彩色图像模式下，显示伪彩色图像标签页
    }
}

DefectImageShow::~DefectImageShow()
{
    //std::cout<<"DefectImageShow::~DefectImageShow()"<<std::endl;
    delete ui;
}


void DefectImageShow::InitDefectShowStatus()
{
    ui->graphicsView_His->SetDefectVisibility(DefectLevel::NORMAL,                DISPLAY_NORMAL          );          //0 正常
    ui->graphicsView_His->SetDefectVisibility(DefectLevel::MINOR,                 DISPLAY_MINOR           );          //1 轻微缺陷
    ui->graphicsView_His->SetDefectVisibility(DefectLevel::MEDIUM,                DISPLAY_MEDIUM          );          //2 中等缺陷
    ui->graphicsView_His->SetDefectVisibility(DefectLevel::SERIOUS,               DISPLAY_SERIOUS         );          //3 严重缺陷
    ui->graphicsView_His->SetDefectVisibility(DefectLevel::ABNORMAL,              DISPLAY_ABNORMAL        );          //4 异常
    ui->graphicsView_His->SetDefectVisibility(DefectLevel::AREAERROR,             DISPLAY_AREAERROR       );          //5 区域错误，非玻璃区域判断

    for (size_t i = 0; i < DefectType::DEFECT_TYPE_COUNT; i++)
    {
        DefectType type = static_cast<DefectType>(i);
        ui->graphicsView_His->SetDefectVisibility(type, m_defectTypeVisibility[type]);
    }
}

void DefectImageShow::ShowHistoryImage(QString originalTime, const QString& filesPath, const QDateTime& dateTime,
    const float& glassPhysicalWidth, const float& glassPhysicalHeight)
{
    FILE_LOG_INFO("TableView_Show_History_Image_begin.");
    QString jsonPath = filesPath + ".json";
    QString h5Path_0 = filesPath + "_detectImages_0.h5";
    QString h5Path_1 = filesPath + "_detectImages_1.h5";
    QString h5Path_2 = filesPath + "_detectImages_2.h5";
    QString backGroundImagePath = filesPath + "backGroundImage.jpg";
    QString pseudoColorImagePath = filesPath + "pseudoColorImage.jpg";
    ////验证图片和json文件是否存在
    QFile fileJson(jsonPath);
    if (!fileJson.exists())
    {
        //QMessageBox::warning(this, tr("Warning"), tr("The json does not exist!"));
        FILE_LOG_ERROR("TableView_ShowHistoryImage error: The json does not exist! %s", jsonPath.toStdString().c_str());
        //emit ui->graphicsView_His->Signal_UpdateDefectNumShow();
        DisplayMessage::getInstance().logMessage(tr("Invalid defect data"), tr("DisplayImage"));  //无效缺陷数据
        return;
    }
    //读取json文件
    glassData2Json json;
    m_results_His.clear();
    m_results_His = json.parseJsonToDrawInfos(jsonPath.toStdString(), m_glassPixelWidth_His, m_glassPixelHeight_His);
    FILE_LOG_INFO("TableView_ShowHistoryImage: read json Completed!");

    std::vector<cv::Mat>defectImg_ch0;
    std::vector<cv::Mat>defectImg_ch1;
    std::vector<cv::Mat>defectImg_ch2;
    if (m_results_His.size() > 0)
    {

        QFile fileH5_0(h5Path_0);
        if (!fileH5_0.exists())
        {
            //QMessageBox::warning(this, tr("Warning"), tr("The image does not exist!"));
            FILE_LOG_ERROR("TableView_ShowHistoryImage error: The fileH5_0 does not exist!");
            emit ui->graphicsView_His->Signal_ClearRectItem();    //空图时清空场景
            DisplayMessage::getInstance().logMessage(tr("Defect image file not found"), tr("DisplayImage"));  //缺陷图像文件未找到
            return;
        }
        QFile fileH5_1(h5Path_1);
        if (!fileH5_1.exists())
        {
            //QMessageBox::warning(this, tr("Warning"), tr("The image does not exist!"));
            FILE_LOG_ERROR("TableView_ShowHistoryImage error: The fileH5_1 does not exist!");
            emit ui->graphicsView_His->Signal_ClearRectItem();    //空图时清空场景
            DisplayMessage::getInstance().logMessage(tr("Defect image file not found"), tr("DisplayImage"));  //缺陷图像文件未找到
            return;
        }
        QFile fileH5_2(h5Path_2);
        if (!fileH5_2.exists())
        {
            //QMessageBox::warning(this, tr("Warning"), tr("The image does not exist!"));
            FILE_LOG_ERROR("TableView_ShowHistoryImage error: The fileH5_2 does not exist!");
            emit ui->graphicsView_His->Signal_ClearRectItem();    //空图时清空场景
            DisplayMessage::getInstance().logMessage(tr("Defect image file not found"), tr("DisplayImage"));  //缺陷图像文件未找到
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
        FILE_LOG_ERROR("TableView_ShowHistoryImage error: The fileBackGroundImage does not exist!");
        emit ui->graphicsView_His->Signal_ClearRectItem();    //空图时清空场景
        DisplayMessage::getInstance().logMessage(tr("Defect image file not found"), tr("DisplayImage"));  //缺陷图像文件未找到
        return;
    }

    //读取图片

    cv::Mat historyImage_BackGround = cv::imread(backGroundImagePath.toStdString(), 0);


    FILE_LOG_INFO("TableView_ShowHistoryImage: read images Completed!");

    if (IsPseudoColorImage)
    {
        cv::Mat historyImage_PseudoColor;
        QFile filePseudoColorImage(pseudoColorImagePath);
        if (!filePseudoColorImage.exists())
        {
            //QMessageBox::warning(this, tr("Warning"), tr("The image does not exist!"));
            FILE_LOG_ERROR("TableView_ShowHistoryImage error: The filePseudoColorImage does not exist!");
            emit ui->graphicsView_His->Signal_ClearRectItem();    //空图时清空场景
            DisplayMessage::getInstance().logMessage(tr("Defect image file not found"), tr("DisplayImage"));  //缺陷图像文件未找到
            return;
        }
        else
        {
            historyImage_PseudoColor = cv::imread(pseudoColorImagePath.toStdString(), 1);
        }
        //显示伪彩图像
        QPixmap pixmap_pseudoColor = QPixmap::fromImage(Mat2QImage(historyImage_PseudoColor));
        if (pixmap_pseudoColor.isNull())
        {
            //QMessageBox::warning(this, tr("Warning"), tr("The image does not exist!"));
            FILE_LOG_ERROR("TableView_ShowHistoryImage error: The pixmap image does not exist!");
            emit ui->graphicsView_His->Signal_ClearRectItem();    //空图时清空场景
            DisplayMessage::getInstance().logMessage(tr("Glass image processing failed"), tr("DisplayImage"));  //玻璃图像文件处理失败
            return;
        }
        else
        {
            emit ui->graphicsView_His_pseudoColor->Signal_AddPixmapItem(pixmap_pseudoColor, m_glassPixelWidth_His, m_glassPixelHeight_His, glassPhysicalWidth, glassPhysicalHeight);
        }

    }

    /* 这里记录主键 */
    ui->graphicsView_His->m_primaryKey = originalTime.toStdString();    //使用ui->graphicsView_His显示历史记录

    //// 获取示例图相对于原图的缩放比
    double scale = m_generalMethod.getScale(historyImage_BackGround.cols, historyImage_BackGround.rows);
    QPixmap pixmap = QPixmap::fromImage(Mat2QImage(historyImage_BackGround));
    if (pixmap.isNull())
    {
        //QMessageBox::warning(this, tr("Warning"), tr("The image does not exist!"));
        FILE_LOG_ERROR("TableView_ShowHistoryImage error: The pixmap image does not exist!");
        emit ui->graphicsView_His->Signal_ClearRectItem();    //空图时清空场景
        DisplayMessage::getInstance().logMessage(tr("Glass image processing failed"), tr("DisplayImage"));  //玻璃图像文件处理失败
        return;
    }
    else
    {
        emit ui->graphicsView_His->Signal_AddPixmapItem(pixmap, m_glassPixelWidth_His, m_glassPixelHeight_His, glassPhysicalWidth, glassPhysicalHeight);
    }

    if (m_results_His.size() != defectImg_ch0.size()
        || m_results_His.size() != defectImg_ch1.size()
        || m_results_His.size() != defectImg_ch2.size())
    {
        FILE_LOG_INFO("TableView_ShowHistoryImage error: Check json and imags size error: Json:%d, img_0:%d, img_1:%d, img_2:%d",
            m_results_His.size(),
            defectImg_ch0.size(),
            defectImg_ch1.size(),
            defectImg_ch2.size()
        );

        FILE_LOG_INFO("TableView_ShowHistoryImage: Completed.");
        DisplayMessage::getInstance().logMessage(tr("Invalid defect data"), tr("DisplayImage"));  //无效缺陷数据
        return;
    }

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
        FILE_LOG_INFO("TableView_ShowHistoryImage warning: No Defects!");
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

        FILE_LOG_INFO("TableView_ShowHistoryImage: Add Rect Item Completed.");
    }

    FILE_LOG_INFO("TableView_ShowHistoryImage: Completed.");
}

void DefectImageShow::closeEvent(QCloseEvent* event)
{
    //在这里添加你想要执行的逻辑，例如：

    //qDebug() << u8"[DefectImageShow] 窗口即将关闭，执行清理操作...";

    emit ui->graphicsView_His->~GraphicsView();
    // 示例1：保存窗口位置/大小
    // QSettings settings;
    // settings.setValue("DefectImageShow/geometry", saveGeometry());

    // 示例2：释放大图缓存
    // clearImageCache();

    // 示例3：通知父窗口（如果需要）
    // emit aboutToClose();

    // 示例4：弹出确认（可选）
    // if (QMessageBox::question(this, "确认", "确定要关闭吗？") == QMessageBox::No) {
    //     event->ignore(); // 阻止关闭
    //     return;
    // }

    // 调用基类实现，否则窗口不会真正关闭！
    QWidget::closeEvent(event); // 或 QDialog::closeEvent(event);
}