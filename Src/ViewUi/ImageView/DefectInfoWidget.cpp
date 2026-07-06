#include "DefectInfoWidget.h"

DefectInfoWidget::DefectInfoWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DefectInfoWidgetClass())
{
    ui->setupUi(this);
}

DefectInfoWidget::~DefectInfoWidget()
{
    delete ui;
}
