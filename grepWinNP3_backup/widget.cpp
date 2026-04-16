#include "widget.h"
#include "ui_widget.h"
#include "settingsdialog.h"
#include "mooddialog.h"
#include "healthstatsdialog.h"
#include "periodconfirmdialog.h"
#include "historymanagerdialog.h"
#include "periodstartdialog.h"
#include <QMessageBox>
#include <QDebug>
#include <QSqlQuery>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

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

    // 🔥 检查是否首次使用（无任何历史记录）
    QSqlQuery countQuery("SELECT COUNT(*) FROM period_history");
    countQuery.next();
    int recordCount = countQuery.value(0).toInt();

    // Debug
    qDebug() << "当前历史记录数:" << recordCount;

    qDebug() << "=== 构造函数：当前历史记录数 ===" << recordCount;

    if (recordCount == 0) {
        // 首次使用，显示欢迎引导
        QMessageBox::information(this, "欢迎来到 HerSpace 🌸",
                                 "欢迎使用 HerSpace，您的专属女性健康伴侣！\n\n"
                                 "看起来这是您第一次使用，让我们从记录第一次经期开始吧。\n\n"
                                 "点击「🌸 经期来了」按钮，记录您最近一次经期的开始日期。\n"
                                 "系统会根据您的记录，智能预测下次经期，并提供贴心的健康建议。\n\n"
                                 "愿 HerSpace 陪伴您度过每一个温柔的日子 💕");

        // 更新界面提示
        ui->labelCycleInfo->setText("🌸 欢迎！请点击「经期来了」记录您的第一次经期。");
        ui->labelHint->setText("点击「经期来了」开始记录您的健康之旅吧 💕");
    }

    // 检查是否需要确认经期
    // 等界面完全显示后再检查（延迟 1.5 秒）
    QTimer::singleShot(1500, this, &Widget::checkPeriodConfirmation);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::loadLatestCycleData()
{
    currentCycleData = healthCalc->getLatestCycleData();

    // 如果 lastPeriodStart 无效（无任何记录），不做预测
    if (!currentCycleData.lastPeriodStart.isValid()) {
        qDebug() << "loadLatestCycleData: 无有效经期记录";
        ui->calendarWidget->setSelectedDate(QDate::currentDate());
        ui->labelCycleInfo->setText("🌸 欢迎！请点击「经期来了」记录您的第一次经期。");
    } else {
        qDebug() << "loadLatestCycleData: 最近经期 =" << currentCycleData.lastPeriodStart.toString("yyyy-MM-dd");
    }
}

void Widget::updateCycleInfoLabel()
{
    QDate today = QDate::currentDate();

    // 检查是否有历史记录
    QSqlQuery countQuery("SELECT COUNT(*) FROM period_history");
    countQuery.next();
    int recordCount = countQuery.value(0).toInt();

    if (recordCount == 0) {
        ui->labelCycleInfo->setText("🌸 欢迎！请点击「经期来了」记录您的第一次经期。");
        return;
    }

    // 获取最近一次经期
    QSqlQuery lastQuery("SELECT start_date FROM period_history ORDER BY start_date DESC LIMIT 1");
    lastQuery.next();
    QDate lastPeriod = QDate::fromString(lastQuery.value(0).toString(), "yyyy-MM-dd");

    // 使用当前的平均周期预测
    QDate expectedStart = lastPeriod.addDays(currentCycleData.cycleLength);
    int daysUntil = today.daysTo(expectedStart);

    QString info;

    if (daysUntil < 0) {
        // 已经超过预测开始日
        int daysLate = -daysUntil;

        if (daysLate <= 3) {
            info = QString("🌸 预计经期已推迟 %1 天\n别担心，1-3天的波动完全正常。").arg(daysLate);
        } else if (daysLate <= 7) {
            info = QString("🌸 经期已推迟 %1 天\n压力、作息变化都可能导致推迟，放松心情。").arg(daysLate);
        } else if (daysLate <= 14) {
            info = QString("⚠️ 经期已推迟 %1 天\n如果持续推迟，建议关注身体状况。").arg(daysLate);
        } else {
            info = QString("⚠️ 经期已推迟 %1 天\n长时间推迟建议咨询医生。").arg(daysLate);
        }

        // 检查是否已有本月记录
        QSqlQuery thisMonthQuery;
        thisMonthQuery.prepare("SELECT id FROM period_history WHERE start_date >= ?");
        thisMonthQuery.bindValue(0, expectedStart.addDays(-5).toString("yyyy-MM-dd"));
        thisMonthQuery.exec();

        if (!thisMonthQuery.next()) {
            info += "\n\n如果经期已至，请点击「经期来了」记录。";
        }
    } else if (daysUntil == 0) {
        info = "🌸 今天预计是经期开始日，记得照顾好自己哦。";
    } else if (daysUntil <= 3) {
        info = QString("🌸 经期即将到来（预计 %1 天后），注意休息保暖。").arg(daysUntil);
    } else {
        info = QString("🌸 距离下次经期还有 %1 天").arg(daysUntil);
    }

    // 加上当前周期阶段（仅在未推迟或推迟不严重时显示）
    if (daysUntil >= -3) {
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
    }

    ui->labelCycleInfo->setText(info);
}

void Widget::updateInspireQuote()
{
    int phase = healthCalc->getCyclePhase(currentCycleData);
    QString quote = inspireMgr.getQuoteForPhase(phase);
    ui->labelInspireQuote->setText("✨ " + quote + " ✨");
}

void Widget::refreshHeartWidget()
{
    if (ui->heartContainer) {
        int phase = healthCalc->getCyclePhase(currentCycleData);
        ui->heartContainer->setCyclePhase(phase);
        // 情绪值默认为 0.8，实际可从数据库读取今日情绪
        ui->heartContainer->setMoodLevel(0.8);
    }

    // 阶段变化时更新语录
    updateInspireQuote();
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
    HealthStatsDialog dialog(this);
    dialog.exec();
}

void Widget::on_btnSettings_clicked()
{
    // 🔥 强制从数据库重新加载最新数据
    currentCycleData = healthCalc->getLatestCycleData();

    qDebug() << "打开设置界面 - 当前数据:";
    qDebug() << "  上次经期:" << currentCycleData.lastPeriodStart.toString("yyyy-MM-dd");
    qDebug() << "  周期长度:" << currentCycleData.cycleLength;
    qDebug() << "  持续天数:" << currentCycleData.periodLength;

    // 🔥 传入主窗口的 healthCalc 对象
    SettingsDialog dialog(healthCalc, this);
    dialog.setCycleData(currentCycleData);

    QSqlQuery query("SELECT COUNT(*) FROM period_history");
    if (query.next()) {
        int count = query.value(0).toInt();
        dialog.setRecordCount(count);
        qDebug() << "  记录总数:" << count;
    }

    connect(&dialog, &SettingsDialog::dataManuallyChanged, this, [this]() {
        currentCycleData = healthCalc->getLatestCycleData();
        refreshHeartWidget();
        updateCycleInfoLabel();
        updateUIForSelectedDate(ui->calendarWidget->selectedDate());
    });

    dialog.exec();
}

// v4.0新增：经期确认交互（预测日与实际不符时的智能响应）
void Widget::checkPeriodConfirmation()
{
    QDate today = QDate::currentDate();

    // 如果没有历史记录，不弹窗
    QSqlQuery countQuery("SELECT COUNT(*) FROM period_history");
    if (countQuery.next() && countQuery.value(0).toInt() == 0) {
        qDebug() << "无历史记录，跳过经期确认检查";
        return;
    }

    // 获取最近一次经期开始日期
    QSqlQuery lastQuery("SELECT start_date FROM period_history ORDER BY start_date DESC LIMIT 1");
    if (!lastQuery.next()) {
        return;
    }
    QDate lastPeriod = QDate::fromString(lastQuery.value(0).toString(), "yyyy-MM-dd");

    // 预测下一次经期（基于最近一次 + 平均周期）
    QDate expectedStart = lastPeriod.addDays(currentCycleData.cycleLength);
    int daysDiff = expectedStart.daysTo(today);

    qDebug() << "=== 经期确认检查 ===";
    qDebug() << "上次经期:" << lastPeriod.toString("yyyy-MM-dd");
    qDebug() << "预测开始:" << expectedStart.toString("yyyy-MM-dd");
    qDebug() << "今天:" << today.toString("yyyy-MM-dd");
    qDebug() << "天数差:" << daysDiff;

    // 检查今天是否已有记录
    QSqlQuery todayQuery;
    todayQuery.prepare("SELECT id FROM period_history WHERE start_date = ?");
    todayQuery.bindValue(0, today.toString("yyyy-MM-dd"));
    todayQuery.exec();
    bool hasTodayRecord = todayQuery.next();

    qDebug() << "今日是否有记录:" << hasTodayRecord;

    // 弹窗条件：
    // 1. 推迟了 0~14 天，或者提前 0~3 天
    // 2. 今天没有记录
    bool shouldPopup = false;
    if (!hasTodayRecord) {
        if (daysDiff >= 0 && daysDiff <= 14) {
            shouldPopup = true;  // 推迟
        } else if (daysDiff >= -3 && daysDiff < 0) {
            shouldPopup = true;  // 提前
        }
    }

    qDebug() << "是否应该弹窗:" << shouldPopup;

    if (shouldPopup) {
        PeriodConfirmDialog dialog(healthCalc, expectedStart, this);
        if (dialog.exec() == QDialog::Accepted) {
            // 处理用户选择
            if (dialog.result() == PeriodConfirmDialog::Came) {
                // 记录今天为经期开始
                healthCalc->recordPeriodStart(today);
                loadLatestCycleData();
                recalculateAverages();
                refreshHeartWidget();
                updateCycleInfoLabel();
                updateUIForSelectedDate(ui->calendarWidget->selectedDate());
                QMessageBox::information(this, "记录成功", "经期已记录，好好休息哦 💕");
            } else if (dialog.result() == PeriodConfirmDialog::Early) {
                QDate earlyDate = dialog.earlyDate();
                healthCalc->recordPeriodStart(earlyDate);
                loadLatestCycleData();
                recalculateAverages();
                refreshHeartWidget();
                updateCycleInfoLabel();
                updateUIForSelectedDate(ui->calendarWidget->selectedDate());
                QMessageBox::information(this, "记录成功",
                                         QString("经期已于 %1 开始，已为您记录。").arg(earlyDate.toString("yyyy-MM-dd")));
            } else {
                // 还没来，显示安慰
                QMessageBox::information(this, "贴心提示",
                                         "身体有自己的节奏，偶尔推迟几天是正常的。\n"
                                         "放松心情，注意休息，她会如约而至。\n\n"
                                         "如果经期已至，请随时点击「经期来了」记录哦。");
            }
        }
    }
}

// 历史经期管理
void Widget::on_btnHistory_clicked()
{
    HistoryManagerDialog dialog(this);

    // 连接数据变化信号
    connect(&dialog, &HistoryManagerDialog::dataChanged, this, [this]() {
        qDebug() << "历史记录已变化，重新计算平均值";
        recalculateAverages();
        currentCycleData = healthCalc->getLatestCycleData();
        refreshHeartWidget();
        updateCycleInfoLabel();
        updateUIForSelectedDate(ui->calendarWidget->selectedDate());
    });

    dialog.exec();
}

// 自动计算平均值
void Widget::recalculateAverages()
{
    QSqlQuery query(DatabaseManager::instance().getDatabase());

    // 先查询有多少条记录
    query.exec("SELECT COUNT(*) FROM period_history");
    int recordCount = 0;
    if (query.next()) {
        recordCount = query.value(0).toInt();
    }

    qDebug() << "recalculateAverages: 记录数 =" << recordCount;

    // 计算平均经期持续天数
    query.exec("SELECT AVG(duration) FROM period_history WHERE duration IS NOT NULL");
    if (query.next() && !query.value(0).isNull()) {
        double avgDuration = query.value(0).toDouble();
        if (avgDuration > 0) {
            currentCycleData.periodLength = qRound(avgDuration);
            qDebug() << "平均持续天数:" << currentCycleData.periodLength;
        }
    }

    // 只有当记录数 >= 2 时才计算平均周期
    if (recordCount >= 2) {
        // 🔥 使用更可靠的查询：先按日期排序，然后计算相邻日期的差值
        QVector<QDate> dates;
        query.exec("SELECT start_date FROM period_history ORDER BY start_date ASC");
        while (query.next()) {
            QDate date = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
            if (date.isValid()) {
                dates.append(date);
            }
        }

        // 计算相邻日期的差值
        QVector<int> cycles;
        for (int i = 1; i < dates.size(); i++) {
            int cycle = dates[i-1].daysTo(dates[i]);
            if (cycle > 0 && cycle < 100) {  // 合理的周期范围：1-99天
                cycles.append(cycle);
                qDebug() << "周期" << i << ":" << dates[i-1].toString("yyyy-MM-dd")
                         << "->" << dates[i].toString("yyyy-MM-dd") << "=" << cycle << "天";
            }
        }

        // 计算平均值
        if (!cycles.isEmpty()) {
            double sum = 0;
            for (int cycle : cycles) {
                sum += cycle;
            }
            double avgCycle = sum / cycles.size();
            currentCycleData.cycleLength = qRound(avgCycle);
            qDebug() << "平均周期:" << currentCycleData.cycleLength << "天 (基于" << cycles.size() << "个周期)";
        } else {
            qDebug() << "无法计算有效周期，保持原有值:" << currentCycleData.cycleLength;
        }
    } else {
        qDebug() << "记录数不足2条，无法计算平均周期";
    }

    // 🔥 关键：保存到数据库
    bool saved = healthCalc->saveCycleData(currentCycleData);
    qDebug() << "保存周期数据:" << (saved ? "成功" : "失败");
    qDebug() << "保存的值 - 上次经期:" << currentCycleData.lastPeriodStart.toString("yyyy-MM-dd")
             << "周期:" << currentCycleData.cycleLength
             << "持续:" << currentCycleData.periodLength;
}

// 创建"经期来了"记录对话框
void Widget::on_btnPeriodStart_clicked()
{
    PeriodStartDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QDate start = dialog.getStartDate();
        int duration = dialog.getDuration();
        bool ended = dialog.isEnded();

        QSqlQuery query(DatabaseManager::instance().getDatabase());

        if (ended && duration > 0) {
            // 已结束，有持续天数
            query.prepare("INSERT OR REPLACE INTO period_history (start_date, duration, end_date) VALUES (?, ?, ?)");
            query.bindValue(0, start.toString("yyyy-MM-dd"));
            query.bindValue(1, duration);
            query.bindValue(2, start.addDays(duration - 1).toString("yyyy-MM-dd"));
        } else {
            // 未结束，只记录开始日期
            query.prepare("INSERT OR REPLACE INTO period_history (start_date) VALUES (?)");
            query.bindValue(0, start.toString("yyyy-MM-dd"));
        }

        if (query.exec()) {
            // 更新当前周期数据中的最近一次经期
            currentCycleData.lastPeriodStart = start;
            if (ended && duration > 0) {
                currentCycleData.periodLength = duration;
            }

            // 重新计算平均值
            recalculateAverages();

            // 🔥 重新从数据库加载，确保数据同步
            currentCycleData = healthCalc->getLatestCycleData();

            // 刷新界面
            refreshHeartWidget();
            updateCycleInfoLabel();
            updateUIForSelectedDate(ui->calendarWidget->selectedDate());

            QMessageBox::information(this, "记录成功",
                                     QString("经期开始于 %1 已记录。\n好好照顾自己哦 💕").arg(start.toString("yyyy-MM-dd")));
        } else {
            QMessageBox::warning(this, "错误", "保存失败：" + query.lastError().text());
            qDebug() << "SQL错误:" << query.lastError().text();
            qDebug() << "执行的SQL:" << query.lastQuery();
        }
    }
}

// 添加退出按钮功能
void Widget::on_btnExit_clicked()
{
    close();
}