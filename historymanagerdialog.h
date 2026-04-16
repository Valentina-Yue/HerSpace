#ifndef HISTORYMANAGERDIALOG_H
#define HISTORYMANAGERDIALOG_H

#include <QDialog>
#include "healthcalculator.h"

namespace Ui {
class HistoryManagerDialog;
}

class HistoryManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HistoryManagerDialog(QWidget *parent = nullptr);
    ~HistoryManagerDialog();

signals:
    void dataChanged();  // 数据变化信号

private slots:
    void on_btnAdd_clicked();
    void on_btnEdit_clicked();      // 🔥 新增
    void on_btnDelete_clicked();
    void on_btnClose_clicked();
    void on_tableHistory_doubleClicked(const QModelIndex &index);  // 🔥 新增：双击编辑

private:
    Ui::HistoryManagerDialog *ui;
    HealthCalculator *healthCalc;
    void refreshTable();
};

#endif // HISTORYMANAGERDIALOG_H