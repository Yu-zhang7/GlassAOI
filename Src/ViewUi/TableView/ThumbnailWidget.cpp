#include "ThumbnailWidget.h"
#include <QFile>
#include "Log.hpp"
#include "Json/glassData2Json.h"    //Json操作类。用于读取历史数据的json信息使用。

ThumbnailWidget::ThumbnailWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::ThumbnailWidgetClass())
{
	ui->setupUi(this);
    m_thumbnailWidth = ui->lb_thumbnail_0->width();
    m_thumbnailHeight = ui->lb_thumbnail_0->height();
    ui->lb_thumbnail_0->setScaledContents(false);
    ui->lb_thumbnail_1->setScaledContents(false);
    ui->lb_thumbnail_2->setScaledContents(false);
    ui->lb_thumbnail_3->setScaledContents(false);

}

ThumbnailWidget::~ThumbnailWidget()
{
    //std::cout<<"ThumbnailWidget::~ThumbnailWidget()"<<std::endl;
	delete ui;
}


void ThumbnailWidget::ShowHistoryThumbnailImage(QString originalTime, const QString& filesPath, const QDateTime& dateTime,
    const float& glassPhysicalWidth, const float& glassPhysicalHeight)
{
    FILE_LOG_INFO("ShowThumbnailImage: Begin.");
    if (IsPseudoColorImage)
    {
        ShowHistoryThumbnailImage_PseudoColorImage(originalTime, filesPath, dateTime, glassPhysicalWidth, glassPhysicalHeight);
    }
    else
    {
        ShowHistoryThumbnailImage_NoPseudoColorImage(originalTime, filesPath, dateTime, glassPhysicalWidth, glassPhysicalHeight);
    }
}


void ThumbnailWidget::ShowHistoryThumbnailImage_PseudoColorImage(QString originalTime, const QString& filesPath, const QDateTime& dateTime,
    const float& glassPhysicalWidth, const float& glassPhysicalHeight)
{
    //FileLogPrintf("ShowThumbnailImage_PseudoColorImage: Begin.");

    QString backGroundImagePath = filesPath + "backGroundImage.jpg";
    QFile fileBackGroundImage(backGroundImagePath);
    if (!fileBackGroundImage.exists())
    {
        //QMessageBox::warning(this, tr("Warning"), tr("The image does not exist!"));
        FILE_LOG_ERROR("ShowThumbnailImage_PseudoColorImage error: The fileBackGroundImage does not exist!");
        //DisplayMessage::getInstance().logMessage(tr("no defect"), tr("CurrentChart"));
        return;
    }
    //读取图片
    cv::Mat historyImage_BackGround = cv::imread(backGroundImagePath.toStdString(), 0);


    QString pseudoColorImagePath = filesPath + "pseudoColorImage.jpg";
    QFile filePseudoColorImage(pseudoColorImagePath);
    if (!filePseudoColorImage.exists())
    {
        //QMessageBox::warning(this, tr("Warning"), tr("The image does not exist!"));
        FILE_LOG_ERROR("ShowThumbnailImage_PseudoColorImage error: The fileBackGroundImage does not exist!");
        //DisplayMessage::getInstance().logMessage(tr("no defect"), tr("CurrentChart"));
        return;
    }
    //读取图片
    cv::Mat historyImage_pseudoColor = cv::imread(pseudoColorImagePath.toStdString(), 1);


    QString jsonPath = filesPath + ".json";
    QFile fileJson(jsonPath);
    if (!fileJson.exists())
    {
        //QMessageBox::warning(this, tr("Warning"), tr("The json does not exist!"));
        FILE_LOG_ERROR("ShowThumbnailImage_PseudoColorImage error: The json does not exist! %s", jsonPath.toStdString().c_str());
        //emit ui->graphicsView_His->Signal_UpdateDefectNumShow();
        return;
    }
    //读取json文件
    glassData2Json json;
    std::vector<drawInformation> roiDrawInfoResult = json.parseJsonToDrawInfos(jsonPath.toStdString(), m_glassPixelWidth_His, m_glassPixelHeight_His);
    FILE_LOG_INFO("ShowThumbnailImage_PseudoColorImage: read json Completed!");

    cv::Size originalSize = historyImage_BackGround.size(); // 原图尺寸

    cv::Size thumbnailSize(400, 150);  // 缩略图尺寸
    if (!historyImage_pseudoColor.empty())
    {
        thumbnailSize.width = historyImage_pseudoColor.cols;
        thumbnailSize.height = historyImage_pseudoColor.rows;
    }
    else
    {
        thumbnailSize.width = historyImage_BackGround.cols;
        thumbnailSize.height = historyImage_BackGround.rows;
    }
    //thumbnailSize.width = (300 * 1.0) * (originalSize.width / originalSize.height);
    //thumbnailSize.height = 300;
    cv::Mat smallImage = m_GeneralMethod.generateDefectThumbnail(roiDrawInfoResult, originalSize, thumbnailSize, 128);
    //cv::Mat smallImage = m_GeneralMethod.generateDefectThumbnail(roiDrawInfoResult, originalSize, thumbnailSize, 128);
    //cv::imwrite("smallImage.png", smallImage);

    QPixmap pixmap_0, pixmap_1;
    QPixmap pixmap_2, pixmap_3;
    //只调用2张图，一张像素图，一张原始图
    QSize thumbnailPixmapSize = QSize(ui->lb_thumbnail_0->size().width() - 10, ui->lb_thumbnail_0->size().height() - 10);

    m_pixmap_backGround = QPixmap();
    if (!smallImage.empty())
    {
        m_pixmap_backGround = QPixmap::fromImage(Mat2QImage(smallImage));
    }
    m_pixmap_pseudoColor = QPixmap();
    if (!historyImage_pseudoColor.empty())
    {
        m_pixmap_pseudoColor = QPixmap::fromImage(Mat2QImage(historyImage_pseudoColor));
    }

    if (ui->lb_thumbnail_0->pixmap() == nullptr && ui->lb_thumbnail_1->pixmap() == nullptr)
    {
        pixmap_0 = m_pixmap_backGround.scaled(thumbnailPixmapSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		pixmap_1 = m_pixmap_pseudoColor.scaled(thumbnailPixmapSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->lb_thumbnail_0->setPixmap(pixmap_0);
        ui->lb_thumbnail_1->setPixmap(pixmap_1);
    }
    else
    {
        if (ui->lb_thumbnail_2->pixmap() == nullptr && ui->lb_thumbnail_3->pixmap() == nullptr)
        {
            pixmap_0 = m_pixmap_backGround.scaled(thumbnailPixmapSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            pixmap_1 = m_pixmap_pseudoColor.scaled(thumbnailPixmapSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            ui->lb_thumbnail_2->setPixmap(pixmap_0);
            ui->lb_thumbnail_3->setPixmap(pixmap_1);
            m_pixmap_backGround_last = m_pixmap_backGround;
			m_pixmap_pseudoColor_last = m_pixmap_pseudoColor;
        }
        //滚动刷新缩略图
        else
        {
            ui->lb_thumbnail_0->clear();
            ui->lb_thumbnail_1->clear();
            pixmap_0 = m_pixmap_backGround_last.scaled(thumbnailPixmapSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            pixmap_1 = m_pixmap_pseudoColor_last.scaled(thumbnailPixmapSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            ui->lb_thumbnail_0->setPixmap(pixmap_0);
            ui->lb_thumbnail_1->setPixmap(pixmap_1);

            ui->lb_thumbnail_2->clear();
            ui->lb_thumbnail_3->clear();
            pixmap_2 = m_pixmap_backGround.scaled(thumbnailPixmapSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            pixmap_3 = m_pixmap_pseudoColor.scaled(thumbnailPixmapSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            ui->lb_thumbnail_2->setPixmap(pixmap_2);
            ui->lb_thumbnail_3->setPixmap(pixmap_3);
            m_pixmap_backGround_last = m_pixmap_backGround;
            m_pixmap_pseudoColor_last = m_pixmap_pseudoColor;
        }
    }

    //ui->lb_thumbnail_2->setPixmap(pixmap_2);
}


void ThumbnailWidget::ShowHistoryThumbnailImage_NoPseudoColorImage(QString originalTime, const QString& filesPath, const QDateTime& dateTime,
    const float& glassPhysicalWidth, const float& glassPhysicalHeight)
{
    //FileLogPrintf("ShowThumbnailImage: Begin.");

    QString backGroundImagePath = filesPath + "backGroundImage.jpg";
    QFile fileBackGroundImage(backGroundImagePath);
    if (!fileBackGroundImage.exists())
    {
        //QMessageBox::warning(this, tr("Warning"), tr("The image does not exist!"));
        FILE_LOG_ERROR("ShowThumbnailImage_NoPseudoColorImage error: The fileBackGroundImage does not exist!");
        //DisplayMessage::getInstance().logMessage(tr("no defect"), tr("CurrentChart"));
        return;
    }
    //读取图片
    cv::Mat historyImage_BackGround = cv::imread(backGroundImagePath.toStdString(), 0);

    QString jsonPath = filesPath + ".json";
    QFile fileJson(jsonPath);
    if (!fileJson.exists())
    {
        //QMessageBox::warning(this, tr("Warning"), tr("The json does not exist!"));
        FILE_LOG_ERROR("ShowThumbnailImage_NoPseudoColorImage error: The json does not exist! %s", jsonPath.toStdString().c_str());
        //emit ui->graphicsView_His->Signal_UpdateDefectNumShow();
        return;
    }
    //读取json文件
    glassData2Json json;
    std::vector<drawInformation> roiDrawInfoResult = json.parseJsonToDrawInfos(jsonPath.toStdString(), m_glassPixelWidth_His, m_glassPixelHeight_His);
    FILE_LOG_INFO("ShowThumbnailImage_NoPseudoColorImage: read json Completed!");

    cv::Size originalSize = historyImage_BackGround.size(); // 原图尺寸
    originalSize.width = historyImage_BackGround.cols;
    originalSize.height = historyImage_BackGround.rows;
    cv::Size thumbnailSize(400, 150);  // 缩略图尺寸
    //thumbnailSize.width = (300 * 1.0) * (originalSize.width / originalSize.height);
    //thumbnailSize.height = 300;
    cv::Mat smallImage = m_GeneralMethod.generateDefectThumbnail(roiDrawInfoResult, originalSize, thumbnailSize, 128);
    cv::imwrite("smallImage.png", smallImage);

    QPixmap pixmap_1;

    //只调用2张图，一张像素图，一张原始图
    if (!smallImage.empty())
    {
        pixmap_1 = QPixmap::fromImage(Mat2QImage(smallImage));
        pixmap_1 = pixmap_1.scaled(ui->lb_thumbnail_0->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    }


    if (ui->lb_thumbnail_0->pixmap() == nullptr)
    {
        ui->lb_thumbnail_0->setPixmap(pixmap_1);
    }
    else if (ui->lb_thumbnail_1->pixmap() == nullptr)
    {
        ui->lb_thumbnail_1->setPixmap(pixmap_1);
    }
    else if (ui->lb_thumbnail_2->pixmap() == nullptr)
    {
        ui->lb_thumbnail_2->setPixmap(pixmap_1);
    }
    else if (ui->lb_thumbnail_3->pixmap() == nullptr)
    {
        ui->lb_thumbnail_3->setPixmap(pixmap_1);
    }
    else
    {
        //滚动刷新缩略图
        ui->lb_thumbnail_0->setPixmap(*ui->lb_thumbnail_1->pixmap());
        ui->lb_thumbnail_1->setPixmap(*ui->lb_thumbnail_2->pixmap());
        ui->lb_thumbnail_2->setPixmap(*ui->lb_thumbnail_3->pixmap());

        ui->lb_thumbnail_3->clear();
        ui->lb_thumbnail_3->setPixmap(pixmap_1);

    }

    //std::cout<< "width::" << ui->lb_thumbnail_0->width() << "height::"<< ui->lb_thumbnail_0->height() << std::endl;
    //ui->lb_thumbnail_2->setPixmap(pixmap_2);
}

