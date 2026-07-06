#include "DefectRectShow.h"
#include "Log.hpp"
#include "Global.h"
#include "ConfigManager.h"
#include <QStyle>
#include <QtGlobal>  // qBound, qMin, qMax

DefectRectShow::DefectRectShow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DefectRectShowClass())
{
    ui->setupUi(this);

    // 设置窗口标题栏不显示问号按钮
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    //固定弹窗尺寸不能被修改
    setFixedSize(size());

    ui->label_7->setVisible(false);
    ui->label_DefectLength->setVisible(false);

    ui->lb_thumbnail0->installEventFilter(this);
    ui->lb_thumbnail1->installEventFilter(this);
    ui->lb_thumbnail2->installEventFilter(this);

    connect(this, &DefectRectShow::thumbnailClicked, this, &DefectRectShow::Slot_mainRectImageChange);

    connect(ui->checkBox_DrawEdge, &QCheckBox::clicked, this, &DefectRectShow::Slot_CheckBox_DrawEdgesClick);

}

DefectRectShow::~DefectRectShow()
{
    //FILE_LOG_INFO("~DefectRectShow");
    //std::cout << "DefectRectShow::~DefectRectShow()" << std::endl;
    delete ui;
}

bool DefectRectShow::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui->lb_thumbnail0 || obj == ui->lb_thumbnail1 || obj == ui->lb_thumbnail2)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                if (obj == ui->lb_thumbnail0)
                {
                    emit thumbnailClicked(0);
                }
                else if (obj == ui->lb_thumbnail1)
                {
                    emit thumbnailClicked(1);
                }
                else if (obj == ui->lb_thumbnail2)
                {
                    emit thumbnailClicked(2);
                }

            }
        }
    }
    return QObject::eventFilter(obj, event);
}

void DefectRectShow::SetDefectInfo(drawInformation& info, float glassPixelWidth, float glassPixelHeight, float glassPhysicalWidth, float glassPhysicalHeight)
{
    if (info.Errorinfo.size() < 2)
    {

        FILE_LOG_ERROR("DefectRectShow::SetDefectInfo, Errorinfo.size() < 2");
        return;
    }
    m_ErrorType = info.ErrorType;
    m_DefectType = info.DefectType;
    m_realRect = info.realRect;

    m_glassPixelWidth = glassPixelWidth;
    m_glassPixelHeight = glassPixelHeight;

    m_glassPhysicalWidth = glassPhysicalWidth;
    m_glassPhysicalHeight = glassPhysicalHeight;
    
    QString level = DefectLevelToString(m_ErrorType);
    QString type = DefectTypeToString(m_DefectType);
    QString area = "0.0";
    QString length = "0.0";;
    QString diagonal = "0.0";
    QString rect = "";
    float mm_x = 0.0f;
    float mm_y = 0.0f;
    float mm_w = 0.0f;
    float mm_h = 0.0f;
    int mm_center_x = 0;
    int mm_center_y = 0;
    if (info.Errorinfo.size() == 3)
    {
        QString unit = "mm";

        QString unit2 = QStringLiteral("mm") + QChar(0x00B2);

        //20251018要求只保留1位小数
        area = QString("%1 %2").arg(QString::number(info.Errorinfo[0], 'f', 1)).arg(unit2);
        length = QString("%1 %2").arg(QString::number(info.Errorinfo[1], 'f', 1)).arg(unit);
        diagonal = QString("%1 %2").arg(QString::number(info.Errorinfo[2], 'f', 1)).arg(unit);
    }

    /* 设置信息到屏幕 */
    ui->label_defectLevel->setText(level);
    ui->label_defectLevel->setScaledContents(true);

    ui->label_DefectType->setText(type);
    ui->label_DefectType->setScaledContents(true);

    if (!info.rect.empty())
    {
        /*
            注意：由于界面图像顺时针旋转90度，所以宽高的像素映射关系需调整，width->PIXEL_Y_MM  height->PIXEL_X_MM
        */
        //mm_x = Pixle2MM_Y * info.realRect.x;
        //mm_y = Pixle2MM_X * info.realRect.y;
        //mm_w = Pixle2MM_Y * info.realRect.width;
        //mm_h = Pixle2MM_X * info.realRect.height;

        //mm_center_x = Pixle2MM_X * (info.realRect.x + (info.realRect.width / 2));
        //mm_center_y = Pixle2MM_Y * (info.realRect.y + (info.realRect.height / 2));

		//20260212直接使用比例系数计算出的实际位置可能会不准确，改用实际尺寸与像素尺寸的比例来计算
        // 1. 比例系数用 float（匹配 mm_x 等变量的 float 类型）
        m_scale_x = static_cast<float>(glassPhysicalWidth) / glassPixelWidth;
        m_scale_y = static_cast<float>(glassPhysicalHeight) / glassPixelHeight;

        // 2. 边界保护
        info.realRect.x = info.realRect.x < 0 ? 0 :
            (info.realRect.x > glassPixelWidth - 1 ? glassPixelWidth - 1 : info.realRect.x);
        info.realRect.y = info.realRect.y < 0 ? 0 :
            (info.realRect.y > glassPixelHeight - 1 ? glassPixelHeight - 1 : info.realRect.y);

        // 3. 宽度高度边界保护
        info.realRect.width = info.realRect.width > glassPixelWidth - info.realRect.x ?
            glassPixelWidth - info.realRect.x : info.realRect.width;
        info.realRect.height = info.realRect.height > glassPixelHeight - info.realRect.y ?
            glassPixelHeight - info.realRect.y : info.realRect.height;

        //// 3. 计算物理值（全部用 float）
        //mm_x = info.realRect.x * m_scale_x;
        //mm_y = info.realRect.y * m_scale_y;
        //mm_w = info.realRect.width * m_scale_x;
        //mm_h = info.realRect.height * m_scale_y;

        //// 4. 中心点：先计算 float，再转 int（四舍五入）
        //mm_center_x = static_cast<int>((info.realRect.x + info.realRect.width / 2.0f) * m_scale_x + 0.5f);
        //mm_center_y = static_cast<int>((info.realRect.y + info.realRect.height / 2.0f) * m_scale_y + 0.5f);

        //qxz0428修改: realRect在未旋转坐标系, 转换为显示物理坐标, Y从底部算起
        if (RotationDirectionFlag == 0) {
            //CW90: display_x = rows - orig_y, display_y = orig_x
            mm_x = (m_glassPixelHeight - info.realRect.y - info.realRect.height) * Pixle2MM_X;
            mm_y = (m_glassPixelWidth - info.realRect.x) * Pixle2MM_Y;
            mm_w = info.realRect.height * Pixle2MM_X;
            mm_h = info.realRect.width * Pixle2MM_Y;
            mm_center_x = static_cast<int>((m_glassPixelHeight - info.realRect.y - info.realRect.height / 2.0f) * Pixle2MM_X + 0.5f);
            mm_center_y = static_cast<int>((m_glassPixelWidth - info.realRect.x + info.realRect.width / 2.0f) * Pixle2MM_Y + 0.5f);
        }
        else {
            //CCW90: display_x = orig_y, display_y = cols - orig_x
            mm_x = info.realRect.y * Pixle2MM_X;
            mm_y = (info.realRect.x + info.realRect.width) * Pixle2MM_Y;
            mm_w = info.realRect.height * Pixle2MM_X;
            mm_h = info.realRect.width * Pixle2MM_Y;
            mm_center_x = static_cast<int>((info.realRect.y + info.realRect.height / 2.0f) * Pixle2MM_X + 0.5f);
            mm_center_y = static_cast<int>((info.realRect.x + info.realRect.width / 2.0f) * Pixle2MM_Y + 0.5f);
        }

        QString msg = QString("PixleRect: %1, %2, %3, %4")
            .arg(QString::number(mm_x, 'f', 1))
            .arg(QString::number(mm_y, 'f', 1))
            .arg(QString::number(mm_w, 'f', 1))
            .arg(QString::number(mm_h, 'f', 1));
        qDebug() << msg;

        rect = QString("(%1, %2)")
            .arg(QString::number(mm_center_x))
            .arg(QString::number(mm_center_y));
    }
    ui->label_DefectRect->setText(rect);
    ui->label_DefectConfidence->setText(QString::number(info.confidence, 'f', 2));  //20251018要求只保留2位小数


    /*ui->label_DefectLength->setText(length);
    ui->label_DefectLength->setScaledContents(true);*/

    if (info.DefectType == DefectType::TYPE_BUBBLE || info.DefectType == DefectType::TYPE_SCRATCH || info.DefectType == DefectType::TYPE_CALCULUS)
    {
        ui->DefectArea_lab->setText(tr("Bounding Box Dimensions"));
    }
    else if (info.DefectType == DefectType::TYPE_TRADEMARK)
    {
        ui->DefectArea_lab->setText("");
    }
    else
    {
        ui->DefectArea_lab->setText(tr("Defect Area"));
    }

    if (info.DefectType == DefectType::TYPE_TRADEMARK)
    {
        ui->label_DefectArea->setText("");
        ui->label_DefectArea->setScaledContents(true);
    }
    else if (info.DefectType == DefectType::TYPE_SCRATCH
        || info.DefectType == DefectType::TYPE_CALCULUS
        || info.DefectType == DefectType::TYPE_BUBBLE)
    {
        if (diagonal == "0.0")
        {
            ui->label_DefectArea->setText(length);
        }
        else
        {
            ui->label_DefectArea->setText(diagonal);

        }
        ui->label_DefectArea->setScaledContents(true);
    }
    else
    {
        ui->label_DefectArea->setText(area);
        ui->label_DefectArea->setScaledContents(true);
    }

}

void DefectRectShow::Slot_mainRectImageChange( int currentIndex)
{
    m_currentIndex = currentIndex;
    emit ui->graphicsView_small->Signal_ClearRectItem();
    const QPixmap* p = nullptr;
    switch (currentIndex)
    {
    case 0:
        p = ui->lb_thumbnail0->pixmap();
        // 选中图像显示框
        ui->lb_thumbnail0->setProperty("selected", "true");
        // 取消选中
        ui->lb_thumbnail1->setProperty("selected", "false");
        // 取消选中
        ui->lb_thumbnail2->setProperty("selected", "false");
        break;
    case 1:
        p = ui->lb_thumbnail1->pixmap();
        // 取消选中
        ui->lb_thumbnail0->setProperty("selected", "false");
        // 选中图像显示框
        ui->lb_thumbnail1->setProperty("selected", "true");
        // 取消选中
        ui->lb_thumbnail2->setProperty("selected", "false");
        break;
    case 2:
        p = ui->lb_thumbnail2->pixmap();
        // 取消选中
        ui->lb_thumbnail0->setProperty("selected", "false");
        // 取消选中
        ui->lb_thumbnail1->setProperty("selected", "false");
        // 选中图像显示框
        ui->lb_thumbnail2->setProperty("selected", "true");
        break;
    default:
        break;
    }
    QPixmap pixmap = (p ? *p : QPixmap());

    // 刷新样式
    ui->lb_thumbnail0->style()->unpolish(ui->lb_thumbnail0);
    ui->lb_thumbnail0->style()->polish(ui->lb_thumbnail0);

    ui->lb_thumbnail1->style()->unpolish(ui->lb_thumbnail1);
    ui->lb_thumbnail1->style()->polish(ui->lb_thumbnail1);

    ui->lb_thumbnail2->style()->unpolish(ui->lb_thumbnail2);
    ui->lb_thumbnail2->style()->polish(ui->lb_thumbnail2);

    FILE_LOG_INFO("showRectImage begin.");
    emit ui->graphicsView_small->Signal_AddPixmapItem(pixmap, m_glassPixelWidth_ruler, m_glassPixelHeight_ruler, m_glassPhysicalWidth_ruler, m_glassPhysicalHeight_ruler);
    FILE_LOG_INFO("showRectImage Completed.");
}

void DefectRectShow::Slot_CheckBox_DrawEdgesClick(bool isDraw)
{ 
    DrawEdgesOnDefect(isDraw);
}

void DefectRectShow::DrawEdgesOnDefect(bool isDraw)
{ 
    cv::Mat rectImage0;
    cv::Mat rectImage1;
    cv::Mat rectImage2;
    if (isDraw)
    {
        cv::Rect colorRect;

        //防护动作，避免ROI越界造成崩溃
        cv::Rect rect = m_realRect;
        if (rect.x < 0) rect.x = 0;
        if (rect.y < 0) rect.y = 0;
        if (rect.x + rect.width >= m_glassPixelWidth) {
            rect.width = (m_glassPixelWidth - 1) - rect.x;
        }
        if (rect.y + rect.height >= m_glassPixelHeight) {
            rect.height = (m_glassPixelHeight - 1) - rect.y;
        }
        // 确保矩形有效
        if (rect.width <= 0 || rect.height <= 0)
        {
            return;
        }
        cv::Rect tempRect = m_generalMethod.computeCropRect2Color(rect, m_glassPixelWidth, m_glassPixelHeight, 256, &colorRect);

        cv::Mat connected0 = m_generalMethod.segUtils.GetDrawImage(m_matThumbnail0, 10, 0);
        cv::Mat connected1 = m_generalMethod.segUtils.GetDrawImage(m_matThumbnail1, 10, 0);
        cv::Mat connected2 = m_generalMethod.segUtils.GetDrawImage(m_matThumbnail2, 10, 0);


        cv::Mat connected;
        cv::bitwise_or(connected0, connected1, connected);
        cv::bitwise_or(connected, connected2, connected);

        // 创建全0的矩阵
        cv::Mat result = cv::Mat::zeros(connected.size(), connected.type());

        // 确保矩形不超出图像边界
        cv::Rect valid_rect = colorRect & cv::Rect(0, 0, connected.cols, connected.rows);

        // 将原图矩形区域复制到结果图
        connected(valid_rect).copyTo(result(valid_rect));

        // 定义颜色映射
        static const std::vector<cv::Scalar> color_map = {
            cv::Scalar(0, 255, 0),    // 绿色 - level 0
            cv::Scalar(0, 255, 255),  // 黄色 - level 1
            cv::Scalar(0, 0, 255)     // 红色 - level 2
        };
        int level = 0;
        // 确保level在有效范围内
        int color_index = std::clamp(level, 0, static_cast<int>(color_map.size() - 1));
        cv::Scalar color = color_map[color_index];

        //MINOR,//轻微缺陷  1
        //MEDIUM,//中等缺陷 2
        //SERIOUS,//严重缺陷 3
        if (m_ErrorType == MEDIUM) {
            level = 1;

        }
        else if (m_ErrorType == SERIOUS) {
            level = 1;
        }
        // 在原图上绘制边缘
        rectImage0 = m_generalMethod.segUtils.drawEdgesOnImage(m_matThumbnail0, result, colorRect, color, level);
        rectImage1 = m_generalMethod.segUtils.drawEdgesOnImage(m_matThumbnail1, result, colorRect, color, level);
        rectImage2 = m_generalMethod.segUtils.drawEdgesOnImage(m_matThumbnail2, result, colorRect, color, level);

        cv::rectangle(rectImage0, colorRect, cv::Scalar(0, 255, 0), 1);
        cv::rectangle(rectImage1, colorRect, cv::Scalar(0, 255, 0), 1);
        cv::rectangle(rectImage2, colorRect, cv::Scalar(0, 255, 0), 1);
        //}
    }
    else
    {
        rectImage0 = m_matThumbnail0;
        rectImage1 = m_matThumbnail1;
        rectImage2 = m_matThumbnail2;
    }
    QPixmap pixmap0 = QPixmap::fromImage(Mat2QImage(rectImage0));
    QPixmap pixmap1 = QPixmap::fromImage(Mat2QImage(rectImage1));
    QPixmap pixmap2 = QPixmap::fromImage(Mat2QImage(rectImage2));
    SetPixmap(pixmap0, pixmap1, pixmap2, m_currentIndex);
}