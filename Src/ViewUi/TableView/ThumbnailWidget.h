#pragma once

#include <QWidget>
#include "ui_ThumbnailWidget.h"
#include "GeneralMethod.h"  //通用方法类。用于处理图像

QT_BEGIN_NAMESPACE
namespace Ui { class ThumbnailWidgetClass; };
QT_END_NAMESPACE

class ThumbnailWidget : public QWidget
{
	Q_OBJECT

public:
	ThumbnailWidget(QWidget *parent = nullptr);
	~ThumbnailWidget();

	void ShowHistoryThumbnailImage(QString originalTime, const QString& filesPath, const QDateTime& dateTime, const float& glassPhysicalWidth, const float& glassPhysicalHeight);

private:
	Ui::ThumbnailWidgetClass *ui;
	GeneralMethod m_GeneralMethod;

	void ShowHistoryThumbnailImage_PseudoColorImage(QString originalTime, const QString& filesPath, const QDateTime& dateTime, const float& glassPhysicalWidth, const float& glassPhysicalHeight);

	void ShowHistoryThumbnailImage_NoPseudoColorImage(QString originalTime, const QString& filesPath, const QDateTime& dateTime, const float& glassPhysicalWidth, const float& glassPhysicalHeight);

	int m_thumbnailWidth;
	int m_thumbnailHeight;

	QPixmap m_pixmap_backGround, m_pixmap_pseudoColor;
	QPixmap m_pixmap_backGround_last, m_pixmap_pseudoColor_last;
};