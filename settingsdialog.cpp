#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QFormLayout>
#include <QDateEdit>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QDebug>

SettingsDialog::SettingsDialog(HealthCalculator *hc, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , healthCalc(hc)  // 🔥 使用外部传入的对象
{
    ui->setupUi(this);
    setWindowTitle("周期统计");
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setCycleData(const CycleData &data)
{
    currentData = data;
    ui->labelLastPeriod->setText(data.lastPeriodStart.toString("yyyy-MM-dd"));
    ui->labelCycleLength->setText(QString::number(data.cycleLength) + " 天");
    ui->labelPeriodLength->setText(QString::number(data.periodLength) + " 天");

    qDebug() << "SettingsDialog::setCycleData - 显示数据:"
             << data.lastPeriodStart.toString("yyyy-MM-dd")
             << data.cycleLength << "天"
             << data.periodLength << "天";
}

void SettingsDialog::setRecordCount(int count)
{
    ui->labelRecordCount->setText(QString::number(count) + " 条");
}

void SettingsDialog::on_btnManualAdjust_clicked()
{
    QMessageBox::StandardButton reply = QMessageBox::warning(this, "手动调整",
                                                             "系统已根据您的历史记录自动计算了平均值。\n"
                                                             "手动调整将覆盖这些自动计算结果。\n\n"
                                                             "确定要手动调整吗？",
                                                             QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    QDialog editDialog(this);
    editDialog.setWindowTitle("手动调整周期参数");
    editDialog.setMinimumWidth(350);
    QFormLayout form(&editDialog);

    QDateEdit *dateEdit = new QDateEdit(&editDialog);
    dateEdit->setDate(currentData.lastPeriodStart.isValid() ? currentData.lastPeriodStart : QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    form.addRow("上次经期开始日期:", dateEdit);

    QSpinBox *spinCycle = new QSpinBox(&editDialog);
    spinCycle->setRange(20, 60);
    spinCycle->setValue(currentData.cycleLength);
    form.addRow("平均周期长度(天):", spinCycle);

    QSpinBox *spinDuration = new QSpinBox(&editDialog);
    spinDuration->setRange(2, 10);
    spinDuration->setValue(currentData.periodLength);
    form.addRow("平均经期持续(天):", spinDuration);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &editDialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &editDialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &editDialog, &QDialog::reject);

    if (editDialog.exec() == QDialog::Accepted) {
        currentData.lastPeriodStart = dateEdit->date();
        currentData.cycleLength = spinCycle->value();
        currentData.periodLength = spinDuration->value();

        if (healthCalc->saveCycleData(currentData)) {
            setCycleData(currentData);
            emit dataManuallyChanged();
            QMessageBox::information(this, "成功", "参数已手动更新。");
        } else {
            QMessageBox::warning(this, "错误", "保存失败，请重试。");
        }
    }
}

void SettingsDialog::on_btnClose_clicked()
{
    accept();
}