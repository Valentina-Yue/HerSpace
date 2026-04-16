#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDate>
#include "healthcalculator.h"
#include "inspiremanager.h"
#include <QTimer>
#include <QSqlQuery>
#include <QMessageBox>
#include <QDebug>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_calendarWidget_clicked(const QDate &date);
    void on_btnMoodRecord_clicked();
    void on_btnHealthData_clicked();
    void on_btnSettings_clicked();
    void on_btnHistory_clicked();
    void on_btnPeriodStart_clicked();
    void on_btnExit_clicked();

private:
    Ui::Widget *ui;
    HealthCalculator *healthCalc;
    CycleData currentCycleData;
    InspireManager inspireMgr;          // 语录管理器

    void updateUIForSelectedDate(const QDate &date);
    void refreshHeartWidget();
    void updateCycleInfoLabel();      // 更新周期信息标签
    void loadLatestCycleData();       // 从数据库加载最新数据
    void updateInspireQuote();        // 更新语录显示
    void checkPeriodConfirmation();   // 检查是否需要经期确认
    void recalculateAverages();       // 自动计算平均值
};

#endif // WIDGET_H