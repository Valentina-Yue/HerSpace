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
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    // 获取用户设置的周期数据
    CycleData getCycleData() const;
    // 用现有数据填充界面（编辑模式）
    void setCycleData(const CycleData &data);

private slots:
    void on_btnSave_clicked();
    void on_btnCancel_clicked();

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H