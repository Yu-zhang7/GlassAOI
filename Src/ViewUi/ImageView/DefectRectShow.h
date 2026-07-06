#ifndef DEFECTRECTSHOW_H
#define DEFECTRECTSHOW_H

#include <QDialog>
#include "GraphicsView.h"
#include "ImageWidget.h"
#include "GeneralMethod.h"  //通用方法类。用于处理图像
#include "ui_DefectRectShow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class DefectRectShowClass; };
QT_END_NAMESPACE

class DefectRectShow : public QDialog
{
    Q_OBJECT

public:
    DefectRectShow(QWidget *parent = nullptr);
    ~DefectRectShow();

public:
    /* 设置图片展示窗口及缩略图 */
    void SetPixmap(QPixmap& pixmap0, QPixmap& pixmap1, QPixmap& pixmap2,int showCurrentIndex)
    {
        //ui->graphicsView_small->Signal_AddPixmapItem(pixmap0);
        //emit ui->graphicsView_small->Signal_BelowOnItem();
        if (!pixmap0.isNull())
        {
            ui->lb_thumbnail0->setPixmap(pixmap0);
        }
        if (!pixmap1.isNull())
        {
            ui->lb_thumbnail1->setPixmap(pixmap1);
        }
        if (!pixmap2.isNull())
        {
            ui->lb_thumbnail2->setPixmap(pixmap2);
        }


        //默认显示第三张图片
        emit thumbnailClicked(showCurrentIndex);
    }

    //显示图像
    void SetCvMat(cv::Mat mat0, cv::Mat mat1, cv::Mat mat2, int showCurrentIndex)
    {
        //20260226:增加原始比例的缺陷小图的尺寸传递，用于显示缺陷小图时，正确的标尺尺寸
        m_glassPixelWidth_ruler = mat1.cols;
        m_glassPixelHeight_ruler = mat1.rows;

        m_glassPhysicalWidth_ruler = m_glassPixelWidth_ruler * m_scale_x;
        m_glassPhysicalHeight_ruler = m_glassPixelHeight_ruler * m_scale_y;

        m_matThumbnail0 = mat0;
        m_matThumbnail1 = mat1;
        m_matThumbnail2 = mat2;
        m_currentIndex = showCurrentIndex;
        QPixmap pixmap0;
        QPixmap pixmap1;
        QPixmap pixmap2;
        if (!mat0.empty())
        {
            pixmap0 = QPixmap::fromImage(Mat2QImage(mat0));
        }
        if (!mat1.empty())
        {
            pixmap1 = QPixmap::fromImage(Mat2QImage(mat1));
        }
        if (!mat2.empty())
        {
            pixmap2 = QPixmap::fromImage(Mat2QImage(mat2));
        }


        //默认显示第三张图片
        SetPixmap(pixmap0, pixmap1, pixmap2, m_currentIndex);
    }

    /* 设置信息显示 */
    void SetDefectInfo(drawInformation& info, float glassPixelWidth, float glassPixelHeight, float glassPhysicalWidth, float glassPhysicalHeight);

    /* 绘制瑕疵边缘 */
    void DrawEdgesOnDefect(bool isDraw);

signals:
    void thumbnailClicked(int currIndex);

protected:
    // 重写 eventFilter 函数
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void Slot_mainRectImageChange(int currIndex);

    void Slot_CheckBox_DrawEdgesClick(bool isDraw);

private:
    ///* 缺陷类型和等级转换 */
    //QString DefectLevelToString(DefectLevel& level) const;
    //QString DefectTypeCast(DefectType& type) const;

private:
    Ui::DefectRectShowClass *ui;

    GeneralMethod					m_generalMethod;
    DefectLevel						m_ErrorType;
    DefectType						m_DefectType;
    cv::Rect					    m_realRect;
    float                           m_glassPixelWidth;
    float                           m_glassPixelHeight;
    float                           m_glassPhysicalWidth;
    float                           m_glassPhysicalHeight;
    cv::Mat                         m_matThumbnail0;
    cv::Mat                         m_matThumbnail1;
    cv::Mat                         m_matThumbnail2;
    int                             m_currentIndex;
    float                           m_scale_x;
    float                           m_scale_y;

    //用于视图显示的标尺
    float                           m_glassPixelWidth_ruler;
    float                           m_glassPixelHeight_ruler;
    float                           m_glassPhysicalWidth_ruler;
    float                           m_glassPhysicalHeight_ruler;
};

#endif // DEFECTRECTSHOW_H
