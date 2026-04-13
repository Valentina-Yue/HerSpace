#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>
#include <QDebug>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    // 初始化健康计算器（内部会打开/创建数据库）
    healthCalc = new HealthCalculator(this);

    // 获取最新周期数据
    currentCycleData = healthCalc->getLatestCycleData();

    // 设置日历默认选中日期为今天
    QDate today = QDate::currentDate();
    ui->calendarWidget->setSelectedDate(today);

    // 初始化爱心显示
    refreshHeartWidget();

    // 显示今天对应的周期信息
    updateUIForSelectedDate(today);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_calendarWidget_clicked(const QDate &date)
{
    updateUIForSelectedDate(date);
}

void Widget::updateUIForSelectedDate(const QDate &date)
{
    // 计算当前选中日期在周期中的位置（基于 lastPeriodStart）
    CycleData data = currentCycleData;
    int daysSinceLast = data.lastPeriodStart.daysTo(date);

    if (daysSinceLast < 0) {
        // 如果选中日期在上次经期开始之前，显示提示
        ui->calendarWidget->setStatusTip("该日期在上次经期之前，无法预测周期阶段");
        return;
    }

    int dayInCycle = daysSinceLast % data.cycleLength;
    if (dayInCycle == 0) dayInCycle = data.cycleLength; // 余数为0时表示第 cycleLength 天

    int phase = -1;
    if (dayInCycle <= data.periodLength) {
        phase = 0; // 月经期
    } else if (dayInCycle <= 14) {
        phase = 1; // 卵泡期
    } else if (dayInCycle <= 16) {
        phase = 2; // 排卵期
    } else {
        phase = 3; // 黄体期
    }

    QString phaseName;
    switch (phase) {
    case 0: phaseName = "月经期"; break;
    case 1: phaseName = "卵泡期"; break;
    case 2: phaseName = "排卵期"; break;
    case 3: phaseName = "黄体期"; break;
    default: phaseName = "未知";
    }

    QString advice = healthCalc->getAdvice(dayInCycle, phase);
    QString statusText = QString("选中日期：%1，周期第 %2 天，%3。%4")
                             .arg(date.toString("yyyy-MM-dd"))
                             .arg(dayInCycle)
                             .arg(phaseName)
                             .arg(advice);

    ui->calendarWidget->setStatusTip(statusText);
}

void Widget::refreshHeartWidget()
{
    if (ui->heartContainer) {
        int phase = healthCalc->getCyclePhase(currentCycleData);
        ui->heartContainer->setCyclePhase(phase);
        // 情绪值暂时固定为 0.8（后续可连接情绪记录模块）
        ui->heartContainer->setMoodLevel(0.8);
    }
}

void Widget::on_btnMoodRecord_clicked()
{
    QMessageBox::information(this, "记录情绪", "情绪记录功能开发中，敬请期待！");
}

void Widget::on_btnHealthData_clicked()
{
    QMessageBox::information(this, "健康数据", "健康数据统计功能开发中，敬请期待！");
}

void Widget::on_btnSettings_clicked()
{
    QMessageBox::information(this, "设置", "设置功能开发中，敬请期待！");
}