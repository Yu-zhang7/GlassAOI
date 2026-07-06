#ifndef DEFECTIMAGESHOW_H
#define DEFECTIMAGESHOW_H

#include <QWidget>
#include "GraphicsView.h"
#include "ImageWidget.h"
#include "GeneralMethod.h"
#include "ui_DefectImageShow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class DefectImageShowClass; };
QT_END_NAMESPACE

class DefectImageShow : public QWidget
{
    Q_OBJECT

public:
    DefectImageShow(QWidget *parent = nullptr);
    ~DefectImageShow();

public:
    /* 设置信息显示 */
    void SetDefectInfo(drawInformation& info);

    /* 显示历史数据和结果 */
    void ShowHistoryImage(QString originalTime, const QString& filesPath, const QDateTime& dateTime,
        const float& glassPhysicalWidth, const float& glassPhysicalHeight);

signals:
    void thumbnailClicked(int currIndex);

private slots:

private:
    /* 设定不同类型不同级别的瑕疵显示状态 */
    void InitDefectShowStatus();

private:
    Ui::DefectImageShowClass *ui;

    GeneralMethod					m_generalMethod;					//图像处理通用方法类

    std::vector<drawInformation>	m_results_His;						//显示的历史记录的瑕疵信息Vector

    std::string						m_time_produce_His;					//选择的历史记录的唯一标识

    QDateTime						m_dateTime_His;						//选择的历史记录的显示到界面上的时间


    float							m_glassPixelWidth_His = 0.0f;
    float							m_glassPixelHeight_His = 0.0f;

    float							m_glassPhysicalWidth_His = 0.0f;
    float							m_glassPhysicalHeight_His = 0.0f;

protected:
    void closeEvent(QCloseEvent* event) override;
};

#endif // DEFECTIMAGESHOW_H
