#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include "healthcalculator.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    // 🔥 修改：接受外部传入的 HealthCalculator
    explicit SettingsDialog(HealthCalculator *hc, QWidget *parent = nullptr);
    ~SettingsDialog();

    void setCycleData(const CycleData &data);
    void setRecordCount(int count);

signals:
    void dataManuallyChanged();

private slots:
    void on_btnManualAdjust_clicked();
    void on_btnClose_clicked();

private:
    Ui::SettingsDialog *ui;
    HealthCalculator *healthCalc;  // 使用外部传入的指针，不自己创建
    CycleData currentData;
};

#endif // SETTINGSDIALOG_H