#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDate>
#include "healthcalculator.h"
#include "emotionalheartwidget.h"   // 必须包含，以便在 ui_widget.h 中识别提升的类

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
};

#endif // WIDGET_H