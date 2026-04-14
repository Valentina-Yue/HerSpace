#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDate>
#include "healthcalculator.h"

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

private:
    Ui::Widget *ui;
    HealthCalculator *healthCalc;
    CycleData currentCycleData;

    void updateUIForSelectedDate(const QDate &date);
    void refreshHeartWidget();
    void updateCycleInfoLabel();      // 新增：更新周期信息标签
    void loadLatestCycleData();       // 新增：从数据库加载最新数据
};

#endif // WIDGET_H