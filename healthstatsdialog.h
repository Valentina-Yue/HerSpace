#ifndef HEALTHSTATSDIALOG_H
#define HEALTHSTATSDIALOG_H

#include <QDialog>
#include <QtCharts/QChartView>
#include "healthcalculator.h"

// 前向声明
class AIManager;

namespace Ui {
class HealthStatsDialog;
}

class HealthStatsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HealthStatsDialog(HealthCalculator *hc, QWidget *parent = nullptr);
    ~HealthStatsDialog();

    // 设置 AI 建议
    void setAIAdvice(const QString &advice);
    // 设置加载中状态
    void setAILoading();
    // 设置错误状态
    void setAIError(const QString &message);
    // 启用/禁用刷新按钮
    void enableRefreshButton(bool enable);

signals:
    // 请求刷新 AI 建议
    void refreshAIRequested();

private slots:
    void on_btnClose_clicked();
    void on_btnRefreshAI_clicked();

private:
    Ui::HealthStatsDialog *ui;
    HealthCalculator *healthCalc;

    void loadStatistics();
    QChart* createCycleChart();
    QChart* createMoodChart();
};

#endif // HEALTHSTATSDIALOG_H