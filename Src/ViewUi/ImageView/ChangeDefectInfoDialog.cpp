#include "ChangeDefectInfoDialog.h"
#include <qinputdialog.h>

ChangeDefectInfoDialog::ChangeDefectInfoDialog(DefectLevel level, DefectType type, QWidget *parent)
    : QDialog(parent)
    , m_level(level)
    , m_type(type)
    , ui(new Ui::ChangeDefectInfoDialogClass())
{
    ui->setupUi(this);
    // 设置窗口标题栏不显示问号按钮
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    //固定弹窗尺寸不能被修改
    setFixedSize(size());

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ChangeDefectInfoDialog::AccpectSlots);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ChangeDefectInfoDialog::reject);

    int defectLevelCount = static_cast<int>(DefectLevel::DEFECT_LEVEL_COUNT);
    for (size_t i = 0; i < defectLevelCount; i++)
    {
        DefectLevel level = static_cast<DefectLevel>(i);
        ui->comboBox_defectLevel->addItem(DefectLevelToString(level), i);
    }

    int defectTypeCount = static_cast<int>(DefectType::DEFECT_TYPE_COUNT);
    for (size_t i = 0; i < defectTypeCount; i++)
    {
        DefectType type = static_cast<DefectType>(i);
        ui->comboBox_defectType->addItem(DefectTypeToString(type), i);
    }

    ui->comboBox_defectLevel->setCurrentIndex(static_cast<int>(m_level));
    ui->comboBox_defectType->setCurrentIndex(static_cast<int>(m_type));
}

ChangeDefectInfoDialog::~ChangeDefectInfoDialog()
{
    //std::cout << "ChangeDefectInfoDialog::~ChangeDefectInfoDialog()" << std::endl;
    delete ui;
}

void ChangeDefectInfoDialog::AccpectSlots()
{
    m_level = static_cast<DefectLevel>(ui->comboBox_defectLevel->currentIndex());
    m_type = static_cast<DefectType>(ui->comboBox_defectType->currentIndex());

    accept();
}
