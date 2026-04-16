#ifndef PERIODSTARTDIALOG_H
#define PERIODSTARTDIALOG_H

#include <QDialog>

namespace Ui {
class PeriodStartDialog;
}

class PeriodStartDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PeriodStartDialog(QWidget *parent = nullptr);
    ~PeriodStartDialog();

    QDate getStartDate() const;
    int getDuration() const;        // 返回 -1 表示未结束
    bool isEnded() const;

private:
    Ui::PeriodStartDialog *ui;
};

#endif // PERIODSTARTDIALOG_H