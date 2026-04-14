#include "widget.h"
#include "ui_widget.h"
#include "settingsdialog.h"
#include "mooddialog.h"
#include <QMessageBox>
#include <QDebug>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    // 手动连接日历的点击信号到槽函数
    // 🔥 显式连接日历点击信号
    connect(ui->calendarWidget, &QCalendarWidget::clicked,
            this, &Widget::on_calendarWidget_clicked);

    healthCalc = new HealthCalculator(this);

    // 从数据库加载最新周期数据
    loadLatestCycleData();

    // 设置日历默认选中今天
    QDate today = QDate::currentDate();
    ui->calendarWidget->setSelectedDate(today);

    // 刷新爱心显示
    refreshHeartWidget();

    // 更新周期信息标签
    updateCycleInfoLabel();

    // 显示今天对应的周期阶段建议
    updateUIForSelectedDate(today);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::loadLatestCycleData()
{
    currentCycleData = healthCalc->getLatestCycleData();
}

void Widget::updateCycleInfoLabel()
{
    QDate today = QDate::currentDate();
    QDate nextPeriod = healthCalc->predictNextPeriod(currentCycleData);
    int daysUntil = today.daysTo(nextPeriod);

    QString info;
    if (daysUntil < 0) {
        info = "经期可能已开始，请检查设置";
    } else if (daysUntil == 0) {
        info = "今天预计是经期开始日";
    } else {
        info = QString("距离下次经期还有 %1 天").arg(daysUntil);
    }

    // 也可以加上当前周期阶段
    int phase = healthCalc->getCyclePhase(currentCycleData);
    QString phaseName;
    switch(phase) {
    case 0: phaseName = "月经期"; break;
    case 1: phaseName = "卵泡期"; break;
    case 2: phaseName = "排卵期"; break;
    case 3: phaseName = "黄体期"; break;
    default: phaseName = "未知";
    }
    info += QString("，当前：%1").arg(phaseName);

    ui->labelCycleInfo->setText(info);
}

void Widget::refreshHeartWidget()
{
    if (ui->heartContainer) {
        int phase = healthCalc->getCyclePhase(currentCycleData);
        ui->heartContainer->setCyclePhase(phase);
        // 情绪值默认为 0.8，实际可从数据库读取今日情绪
        ui->heartContainer->setMoodLevel(0.8);
    }
}

void Widget::updateUIForSelectedDate(const QDate &date)
{
    int daysSinceLast = currentCycleData.lastPeriodStart.daysTo(date);
    if (daysSinceLast < 0) {
        ui->calendarWidget->setStatusTip("该日期在上次经期之前，无法预测周期阶段");
        return;
    }

    int dayInCycle = daysSinceLast % currentCycleData.cycleLength;
    if (dayInCycle == 0) dayInCycle = currentCycleData.cycleLength;

    int phase = -1;
    if (dayInCycle <= currentCycleData.periodLength) {
        phase = 0;
    } else if (dayInCycle <= 14) {
        phase = 1;
    } else if (dayInCycle <= 16) {
        phase = 2;
    } else {
        phase = 3;
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
    ui->labelHint->setText(statusText);
}

void Widget::on_calendarWidget_clicked(const QDate &date)
{
    updateUIForSelectedDate(date);
}

void Widget::on_btnMoodRecord_clicked()
{
    MoodDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        int level = dialog.getMoodLevel();
        QString diary = dialog.getDiaryText();

        // 将情绪值传递给爱心控件（1-5 映射到 0.2-1.0）
        double moodValue = level / 5.0;
        ui->heartContainer->setMoodLevel(moodValue);

        // 后续可以保存日记和情绪值到数据库
        QMessageBox::information(this, "记录成功", QString("心情已记录：%1 级").arg(level));
    }
}

void Widget::on_btnHealthData_clicked()
{
    QString advice;
    int phase = healthCalc->getCyclePhase(currentCycleData);
    int day = healthCalc->getCurrentDayInCycle(currentCycleData);

    advice = healthCalc->getAdvice(day, phase);

    // 增加一些统计数据
    advice += "\n\n";
    advice += QString("当前周期设置：%1天周期，经期持续%2天。")
                  .arg(currentCycleData.cycleLength)
                  .arg(currentCycleData.periodLength);

    QMessageBox::information(this, "健康建议", advice);
}

void Widget::on_btnSettings_clicked()
{
    SettingsDialog dialog(this);
    dialog.setCycleData(currentCycleData);   // 显示当前设置

    if (dialog.exec() == QDialog::Accepted) {
        // 用户点击了保存
        currentCycleData = dialog.getCycleData();

        // 保存到数据库
        if (healthCalc->saveCycleData(currentCycleData)) {
            QMessageBox::information(this, "成功", "经期设置已保存！");
        } else {
            QMessageBox::warning(this, "错误", "保存失败，请检查数据库！");
        }

        // 刷新界面
        refreshHeartWidget();
        updateCycleInfoLabel();
        updateUIForSelectedDate(ui->calendarWidget->selectedDate());
    }
}