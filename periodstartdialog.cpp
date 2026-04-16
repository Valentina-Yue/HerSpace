#include "periodstartdialog.h"
#include "ui_periodstartdialog.h"

PeriodStartDialog::PeriodStartDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PeriodStartDialog)
{
    ui->setupUi(this);

    // 设置默认日期为今天
    ui->dateEditStart->setDate(QDate::currentDate());

    // 🔥 手动连接复选框和 spinBox 的启用状态
    connect(ui->checkBoxEnded, &QCheckBox::toggled, ui->spinDuration, &QSpinBox::setEnabled);

    // 初始状态：未勾选时 spinBox 禁用
    ui->spinDuration->setEnabled(false);
}

PeriodStartDialog::~PeriodStartDialog()
{
    delete ui;
}

QDate PeriodStartDialog::getStartDate() const
{
    return ui->dateEditStart->date();
}

int PeriodStartDialog::getDuration() const
{
    if (ui->checkBoxEnded->isChecked()) {
        return ui->spinDuration->value();
    }
    return -1;  // 表示尚未结束
}

bool PeriodStartDialog::isEnded() const
{
    return ui->checkBoxEnded->isChecked();
}