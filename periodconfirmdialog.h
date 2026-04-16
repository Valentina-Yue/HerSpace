#ifndef PERIODCONFIRMDIALOG_H
#define PERIODCONFIRMDIALOG_H

#include <QDialog>
#include <QDate>
#include "healthcalculator.h"

namespace Ui {
class PeriodConfirmDialog;
}

class PeriodConfirmDialog : public QDialog
{
    Q_OBJECT

public:
    enum Result { Came, NotYet, Early };

    explicit PeriodConfirmDialog(HealthCalculator *hc, const QDate &expectedDate, QWidget *parent = nullptr);
    ~PeriodConfirmDialog();

    Result result() const { return m_result; }
    QDate earlyDate() const { return m_earlyDate; }

private slots:
    void on_btnCame_clicked();
    void on_btnNotYet_clicked();
    void on_btnEarly_clicked();

private:
    Ui::PeriodConfirmDialog *ui;
    HealthCalculator *healthCalc;
    Result m_result;
    QDate m_earlyDate;
    QDate m_expectedDate;
};

#endif // PERIODCONFIRMDIALOG_H