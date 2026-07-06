#pragma once

#include <QWidget>
#include "ui_DefectInfoWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class DefectInfoWidgetClass; };
QT_END_NAMESPACE

class DefectInfoWidget : public QWidget
{
    Q_OBJECT

public:
    DefectInfoWidget(QWidget *parent = nullptr);
    ~DefectInfoWidget();

private:
    Ui::DefectInfoWidgetClass *ui;
};
