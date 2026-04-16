#ifndef HEALTHSTATSDIALOG_H
#define HEALTHSTATSDIALOG_H

#include <QDialog>
#include <QtCharts/QChartView>
#include "healthcalculator.h"

QT_BEGIN_NAMESPACE
namespace Ui { class HealthStatsDialog; }
QT_END_NAMESPACE

class HealthStatsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HealthStatsDialog(QWidget *parent = nullptr);
    ~HealthStatsDialog();

private:
    Ui::HealthStatsDialog *ui;
    HealthCalculator *healthCalc;
    void loadStatistics();
    QChart* createCycleChart();
};

#endif // HEALTHSTATSDIALOG_H