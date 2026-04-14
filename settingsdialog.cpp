#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    // 设置日期选择器默认值为今天
    ui->dateEditLastPeriod->setDate(QDate::currentDate());
    // 设置 SpinBox 范围已在 UI 文件中完成
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

CycleData SettingsDialog::getCycleData() const
{
    CycleData data;
    data.lastPeriodStart = ui->dateEditLastPeriod->date();
    data.cycleLength = ui->spinCycleLength->value();
    data.periodLength = ui->spinPeriodLength->value();
    return data;
}

void SettingsDialog::setCycleData(const CycleData &data)
{
    ui->dateEditLastPeriod->setDate(data.lastPeriodStart);
    ui->spinCycleLength->setValue(data.cycleLength);
    ui->spinPeriodLength->setValue(data.periodLength);
}

void SettingsDialog::on_btnSave_clicked()
{
    accept();  // 关闭对话框并返回 QDialog::Accepted
}

void SettingsDialog::on_btnCancel_clicked()
{
    reject();  // 关闭对话框并返回 QDialog::Rejected
}