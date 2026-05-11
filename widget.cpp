#include "widget.h"
#include "ui_widget.h"
#include "settingsdialog.h"
#include "mooddialog.h"
#include "periodstartdialog.h"
#include "historymanagerdialog.h"
#include "healthstatsdialog.h"
#include "periodconfirmdialog.h"
#include "databasemanager.h"
#include "aimanager.h"
#include "moodhistorydialog.h"
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <QSqlQuery>
#include <QSqlError>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , isInPeriod(false)               // 🔥 初始化成员变量
    , periodStartDate(QDate())        // 🔥 初始化为无效日期
{
    ui->setupUi(this);

    // 显式连接日历点击信号
    connect(ui->calendarWidget, &QCalendarWidget::clicked,
            this, &Widget::on_calendarWidget_clicked);

    healthCalc = new HealthCalculator(this);

    // 创建 AI 管理器
    aiManager = new AIManager(this);
    connect(aiManager, &AIManager::adviceReady, this, &Widget::onAIAdviceReady);
    connect(aiManager, &AIManager::inspirationReady, this, &Widget::onAIInspirationReady);
    connect(aiManager, &AIManager::error, this, &Widget::onAIError);

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

    // 首次使用检测
    QSqlDatabase db = DatabaseManager::instance().getDatabase();
    QSqlQuery countQuery(db);
    countQuery.exec("SELECT COUNT(*) FROM period_history");
    int recordCount = 0;
    if (countQuery.next()) {
        recordCount = countQuery.value(0).toInt();
    }
    qDebug() << "当前历史记录数:" << recordCount;

    if (recordCount == 0) {
        QMessageBox::information(this, "欢迎来到 HerSpace 🌸",
                                 "欢迎使用 HerSpace，您的专属女性健康伴侣！\n\n"
                                 "看起来这是您第一次使用，让我们从记录第一次经期开始吧。\n\n"
                                 "点击「经期来了」按钮，记录您最近一次经期的开始日期。\n"
                                 "系统会根据您的记录，智能预测下次经期，并提供贴心的健康建议。\n\n"
                                 "愿 HerSpace 陪伴您度过每一个温柔的日子 💕");
        ui->labelCycleInfo->setText("🌸 欢迎！请点击「经期来了」记录您的第一次经期。");
    }

    // 🔥 加载经期状态：检查是否正在经期中
    QSqlQuery lastQuery(db);
    lastQuery.exec("SELECT start_date, duration FROM period_history ORDER BY start_date DESC LIMIT 1");
    if (lastQuery.next()) {
        QDate start = QDate::fromString(lastQuery.value(0).toString(), "yyyy-MM-dd");
        int duration = lastQuery.value(1).toInt();  // 如果未记录持续时间，duration 可能为 0 或 NULL（toInt 返回 0）
        int daysSince = start.daysTo(today);

        // 判断条件：持续时间未知（duration == 0）或者还在持续天数内
        if (duration == 0 || daysSince < duration) {
            isInPeriod = true;
            periodStartDate = start;
            qDebug() << "加载经期状态：正在经期中，开始日期" << start.toString("yyyy-MM-dd")
                     << "已持续" << daysSince << "天";
        } else {
            isInPeriod = false;
            qDebug() << "加载经期状态：不在经期中";
        }
    }

    // 延迟检查经期确认（确保界面完全显示）
    QTimer::singleShot(1500, this, &Widget::checkPeriodConfirmation);

    // 延迟请求 AI 每日语录（避免启动时网络阻塞）
    QTimer::singleShot(2000, this, &Widget::requestAIInspiration);
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
    QSqlDatabase db = DatabaseManager::instance().getDatabase();

    // 检查是否有历史记录
    QSqlQuery countQuery(db);
    countQuery.exec("SELECT COUNT(*) FROM period_history");
    countQuery.next();
    int recordCount = countQuery.value(0).toInt();

    if (recordCount == 0) {
        ui->labelCycleInfo->setText("🌸 欢迎！请点击「经期来了」记录您的第一次经期。");
        return;
    }

    // 获取最近一次经期
    QSqlQuery lastQuery(db);
    lastQuery.exec("SELECT start_date FROM period_history ORDER BY start_date DESC LIMIT 1");
    lastQuery.next();
    QDate lastPeriod = QDate::fromString(lastQuery.value(0).toString(), "yyyy-MM-dd");

    // 使用当前的平均周期预测下一次经期
    QDate expectedStart = lastPeriod.addDays(currentCycleData.cycleLength);
    int daysUntil = today.daysTo(expectedStart);    // 正数表示还没到，负数表示已超过
    int daysLate = -daysUntil;                     // 推迟天数（如果已超过）

    // 检查是否处于推迟状态
    bool isDelayed = false;
    int phase = healthCalc->getCyclePhase(currentCycleData, &isDelayed);

    QString info;

    // 优先判断是否正在经期中（基于最近一次记录就是今天或前几天且在持续天数内）
    bool inPeriod = false;
    int daysSinceLast = lastPeriod.daysTo(today);
    if (daysSinceLast >= 0 && daysSinceLast < currentCycleData.periodLength) {
        inPeriod = true;
    }

    if (inPeriod) {
        int periodDay = daysSinceLast + 1;
        info = QString("🌸 月经期第 %1 天\n注意保暖，多喝热水，避免剧烈运动。").arg(periodDay);
    } else if (isDelayed) {
        // 经期推迟中
        if (daysLate <= 3) {
            info = QString("🌸 经期推迟 %1 天\n1-3天的波动完全正常，可能是压力或作息影响。放松心情，她会如约而至。").arg(daysLate);
        } else if (daysLate <= 7) {
            info = QString("🌸 经期推迟 %1 天\n一周内的推迟仍属常见，注意休息，减少焦虑。").arg(daysLate);
        } else if (daysLate <= 14) {
            info = QString("⚠️ 经期推迟 %1 天\n如果持续推迟，建议关注身体状况，必要时咨询医生。").arg(daysLate);
        } else {
            info = QString("⚠️ 经期推迟 %1 天\n长时间推迟建议就医检查，排除内分泌或压力因素。").arg(daysLate);
        }

        // 检查是否已有本月记录
        QSqlQuery thisMonthQuery;
        thisMonthQuery.prepare("SELECT id FROM period_history WHERE start_date >= ?");
        thisMonthQuery.bindValue(0, expectedStart.addDays(-5).toString("yyyy-MM-dd"));
        thisMonthQuery.exec();

        if (!thisMonthQuery.next()) {
            info += "\n\n如果经期已至，请点击「经期来了」记录。";
        }
    } else {
        // 正常周期内，显示阶段信息
        QString phaseName;
        switch(phase) {
        case 0: phaseName = "月经期"; break;
        case 1: phaseName = "卵泡期"; break;
        case 2: phaseName = "排卵期"; break;
        case 3: phaseName = "黄体期"; break;
        default: phaseName = "未知阶段";
        }

        if (daysUntil == 0) {
            info = "🌸 今天预计是经期开始日，记得照顾好自己哦。";
        } else if (daysUntil <= 3) {
            info = QString("🌸 经期即将到来（预计 %1 天后），注意休息保暖。").arg(daysUntil);
        } else {
            info = QString("🌸 距离下次经期还有 %1 天").arg(daysUntil);
        }
        info += QString("，当前：%1").arg(phaseName);
    }

    ui->labelCycleInfo->setText(info);
}


void Widget::updateInspireQuote()
{
    bool isDelayed = false;
    int phase = healthCalc->getCyclePhase(currentCycleData, &isDelayed);

    if (isDelayed) {
        // 推迟状态：请求 AI 生成暖心话语
        requestAIInspiration();
        // 在 AI 返回之前，先显示一个临时占位语
        ui->labelInspireQuote->setText("✨ 身体有自己的节奏，偶尔的推迟是她在悄悄调整 ✨");
    } else {
        // 正常周期：使用本地语录库
        QString quote = inspireMgr.getQuoteForPhase(phase);
        ui->labelInspireQuote->setText("✨ " + quote + " ✨");
    }
}

void Widget::refreshHeartWidget()
{
    if (ui->heartContainer) {
        bool isDelayed = false;
        int phase = healthCalc->getCyclePhase(currentCycleData, &isDelayed);

        // 检查是否正在经期中
        QDate lastPeriod = currentCycleData.lastPeriodStart;
        int daysSinceLast = lastPeriod.daysTo(QDate::currentDate());
        bool inPeriod = (daysSinceLast >= 0 && daysSinceLast < currentCycleData.periodLength);

        int displayPhase = phase;
        if (inPeriod) {
            displayPhase = 0;  // 强制显示月经期
        } else if (isDelayed) {
            displayPhase = -1; // 推迟中
        }

        ui->heartContainer->setCyclePhase(displayPhase);

        // 从数据库获取今日心情
        QSqlDatabase db = DatabaseManager::instance().getDatabase();
        QSqlQuery query(db);
        query.prepare("SELECT mood_level FROM mood_history WHERE date = ?");
        query.bindValue(0, QDate::currentDate().toString("yyyy-MM-dd"));
        if (query.exec() && query.next()) {
            int level = query.value(0).toInt();
            double moodValue = level / 5.0;
            ui->heartContainer->setMoodLevel(moodValue);
        } else {
            ui->heartContainer->setMoodLevel(0.6); // 默认心情
        }
    }
}

void Widget::updateUIForSelectedDate(const QDate &date)
{
    QDate lastPeriod = currentCycleData.lastPeriodStart;
    int cycleLen = currentCycleData.cycleLength;
    int periodLen = currentCycleData.periodLength;

    // 计算选中日期与上次经期开始的天数差
    int daysSinceLast = lastPeriod.daysTo(date);

    // 如果选中日期在上次经期之前，无法预测
    if (daysSinceLast < 0) {
        ui->labelHint->setText("该日期在上次经期开始之前，无法计算周期阶段。");
        return;
    }

    // 预测下次经期开始日期
    QDate expectedStart = lastPeriod.addDays(cycleLen);
    bool isDelayed = (date >= expectedStart) && (daysSinceLast >= cycleLen);

    int dayInCycle;
    if (isDelayed) {
        // 推迟状态：显示推迟天数，不再用常规周期阶段
        int daysLate = daysSinceLast - cycleLen + 1; // +1 是因为预测开始那天就算推迟第1天？
        // 更直观：从预测日开始算起推迟了几天
        daysLate = expectedStart.daysTo(date);
        QString hintText = QString("选中日期：%1\n⚠️ 经期已推迟 %2 天。\n身体有自己的节奏，偶尔波动是正常的。")
                               .arg(date.toString("yyyy-MM-dd"))
                               .arg(daysLate);
        ui->labelHint->setText(hintText);
        return;
    }

    // // 正常周期内：计算当前是周期第几天（从1开始）
    // if (daysSinceLast == 0) {
    //     dayInCycle = 1;
    // } else {
    //     dayInCycle = (daysSinceLast % cycleLen);
    //     if (dayInCycle == 0) {
    //         dayInCycle = cycleLen;
    //     }
    // }

    // 🔥 修复：正确计算周期第几天（1-based）
    // 例如：daysSinceLast = 0 → 第1天
    //      daysSinceLast = 1 → 第2天
    //      daysSinceLast = 27 → 第28天
    //      daysSinceLast = 28 → 第1天（下一个周期）
    dayInCycle = (daysSinceLast % cycleLen) + 1;

    // 判断周期阶段
    int phase = -1;
    if (dayInCycle <= periodLen) {
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
    QString hintText = QString("选中日期：%1，周期第 %2 天，%3\n%4")
                           .arg(date.toString("yyyy-MM-dd"))
                           .arg(dayInCycle)
                           .arg(phaseName)
                           .arg(advice);
    ui->labelHint->setText(hintText);
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

        QSqlDatabase db = DatabaseManager::instance().getDatabase();
        if (!db.isOpen()) {
            QMessageBox::critical(this, "错误", "数据库未打开，无法保存心情记录。");
            return;
        }

        QString todayStr = QDate::currentDate().toString("yyyy-MM-dd");

        // 第一步：获取今日已有日记（如果有）
        QString existingDiary;
        {
            QSqlQuery query(db);
            query.prepare("SELECT diary_text FROM mood_history WHERE date = ?");
            query.bindValue(0, todayStr);
            if (query.exec() && query.next()) {
                existingDiary = query.value(0).toString();
            }
        }

        // 合并日记
        QString finalDiary = diary;
        if (!existingDiary.isEmpty() && !diary.isEmpty()) {
            finalDiary = existingDiary + "\n\n---\n\n" + diary;
        } else if (!existingDiary.isEmpty()) {
            finalDiary = existingDiary;
        }

        // 第二步：插入或替换记录
        {
            QSqlQuery query(db);
            query.prepare("INSERT OR REPLACE INTO mood_history (date, mood_level, diary_text) VALUES (?, ?, ?)");
            query.bindValue(0, todayStr);
            query.bindValue(1, level);
            query.bindValue(2, finalDiary);

            // 🔥 调试：打印绑定的值
            qDebug() << "准备插入情绪记录: date=" << todayStr << " level=" << level << " diary length=" << finalDiary.length();
            qDebug() << "Bound values count:" << query.boundValues().size();

            if (!query.exec()) {
                QMessageBox::warning(this, "错误", "保存失败：" + query.lastError().text());
                qDebug() << "情绪记录 SQL 错误:" << query.lastError().text();
                return;
            }
        }

        // 更新爱心颜色
        double moodValue = level / 5.0;
        ui->heartContainer->setMoodLevel(moodValue);

        // 请求 AI 语录
        requestAIInspiration();

        QMessageBox::information(this, "记录成功",
                                 QString("心情已记录：%1 级\n愿你今天有个好心情 💕").arg(level));
    }
}

void Widget::on_btnHealthData_clicked()
{
    HealthStatsDialog dialog(healthCalc, this);

    // 🔥 检查是否已有缓存的 AI 建议
    static QString cachedAIAdvice;
    static bool hasAICached = false;

    if (hasAICached && !cachedAIAdvice.isEmpty()) {
        dialog.setAIAdvice(cachedAIAdvice);
    } else {
        dialog.setAILoading();

        // 异步请求 AI 建议
        AIManager *tempAI = new AIManager(this);
        connect(tempAI, &AIManager::adviceReady, &dialog, [&dialog, &cachedAIAdvice, &hasAICached, tempAI](const QString &advice) {
            cachedAIAdvice = advice;
            hasAICached = true;
            dialog.setAIAdvice(advice);
            tempAI->deleteLater();
        });
        connect(tempAI, &AIManager::error, &dialog, [&dialog, tempAI](const QString &error) {
            dialog.setAIError(error);
            tempAI->deleteLater();
        });

        // 获取数据并请求
        QVector<QPair<QDate, int>> recentMoods;
        QSqlDatabase db = DatabaseManager::instance().getDatabase();
        QSqlQuery query(db);
        query.exec("SELECT date, mood_level FROM mood_history ORDER BY date DESC LIMIT 7");
        while (query.next()) {
            QDate date = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
            int level = query.value(1).toInt();
            recentMoods.prepend(qMakePair(date, level));
        }

        tempAI->generatePersonalizedAdvice(
            currentCycleData.lastPeriodStart,
            currentCycleData.cycleLength,
            currentCycleData.periodLength,
            recentMoods
            );
    }

    // 连接刷新信号
    connect(&dialog, &HealthStatsDialog::refreshAIRequested, this, [this, &dialog]() {
        dialog.setAILoading();

        AIManager *tempAI = new AIManager(this);
        connect(tempAI, &AIManager::adviceReady, &dialog, [&dialog, tempAI](const QString &advice) {
            dialog.setAIAdvice(advice);
            tempAI->deleteLater();
        });
        connect(tempAI, &AIManager::error, &dialog, [&dialog, tempAI](const QString &error) {
            dialog.setAIError(error);
            tempAI->deleteLater();
        });

        QVector<QPair<QDate, int>> recentMoods;
        QSqlDatabase db = DatabaseManager::instance().getDatabase();
        QSqlQuery query(db);
        query.exec("SELECT date, mood_level FROM mood_history ORDER BY date DESC LIMIT 7");
        while (query.next()) {
            QDate date = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
            int level = query.value(1).toInt();
            recentMoods.prepend(qMakePair(date, level));
        }

        tempAI->generatePersonalizedAdvice(
            currentCycleData.lastPeriodStart,
            currentCycleData.cycleLength,
            currentCycleData.periodLength,
            recentMoods
            );
    });

    dialog.exec();
}

void Widget::on_btnSettings_clicked()
{
    currentCycleData = healthCalc->getLatestCycleData();

    SettingsDialog dialog(healthCalc, this);
    dialog.setCycleData(currentCycleData);

    QSqlDatabase db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);
    query.exec("SELECT COUNT(*) FROM period_history");
    if (query.next()) {
        dialog.setRecordCount(query.value(0).toInt());
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
    QSqlDatabase db = DatabaseManager::instance().getDatabase();

    // ==================== 第一部分：经期结束确认 ====================
    // 🔥 1. 检查是否正在经期中，是否需要询问经期结束
    if (isInPeriod && periodStartDate.isValid()) {
        int daysInPeriod = periodStartDate.daysTo(today);

        // 在此期间每天询问一次是否结束
        if (daysInPeriod >= 2 && daysInPeriod <= 10) {
            // 检查今天是否已经询问过
            QSqlQuery askedQuery(db);
            askedQuery.prepare("SELECT id FROM period_confirm_asked WHERE date = ?");
            askedQuery.bindValue(0, today.toString("yyyy-MM-dd"));
            askedQuery.exec();

            if (!askedQuery.next()) {
                // 记录今天已询问，避免重复弹窗
                QSqlQuery insertAsked(db);
                insertAsked.prepare("INSERT INTO period_confirm_asked (date) VALUES (?)");
                insertAsked.bindValue(0, today.toString("yyyy-MM-dd"));
                insertAsked.exec();

                QMessageBox::StandardButton reply = QMessageBox::question(this, "经期确认",
                                                                          QString("您的经期已持续 %1 天，是否已经结束了？").arg(daysInPeriod),
                                                                          QMessageBox::Yes | QMessageBox::No);

                if (reply == QMessageBox::Yes) {
                    // 更新 period_history 中的 duration 和 end_date
                    QSqlQuery updateQuery(db);
                    updateQuery.prepare("UPDATE period_history SET duration = ?, end_date = ? WHERE start_date = ?");
                    updateQuery.bindValue(0, daysInPeriod);
                    updateQuery.bindValue(1, today.toString("yyyy-MM-dd"));
                    updateQuery.bindValue(2, periodStartDate.toString("yyyy-MM-dd"));

                    if (updateQuery.exec()) {
                        isInPeriod = false;

                        // 重新计算平均值（包括本次经期持续天数）
                        recalculateAverages();
                        currentCycleData = healthCalc->getLatestCycleData();
                        refreshHeartWidget();
                        updateCycleInfoLabel();
                        updateUIForSelectedDate(ui->calendarWidget->selectedDate());

                        QMessageBox::information(this, "记录成功",
                                                 QString("经期持续 %1 天已记录。感谢您的反馈！").arg(daysInPeriod));

                        // 请求 AI 生成新的健康建议
                        requestAIAdvice();
                    } else {
                        qDebug() << "更新经期持续天数失败:" << updateQuery.lastError().text();
                    }
                }
            }
        }
        else if (daysInPeriod > 7) {
            // 超过 7 天，直接强制询问（不检查是否已询问）
            QMessageBox::StandardButton reply = QMessageBox::question(this, "经期确认",
                                                                      QString("您的经期已持续 %1 天，是否已经结束了？\n（经期超过 7 天建议关注身体状况）").arg(daysInPeriod),
                                                                      QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                QSqlQuery updateQuery(db);
                updateQuery.prepare("UPDATE period_history SET duration = ?, end_date = ? WHERE start_date = ?");
                updateQuery.bindValue(0, daysInPeriod);
                updateQuery.bindValue(1, today.toString("yyyy-MM-dd"));
                updateQuery.bindValue(2, periodStartDate.toString("yyyy-MM-dd"));

                if (updateQuery.exec()) {
                    isInPeriod = false;
                    recalculateAverages();
                    currentCycleData = healthCalc->getLatestCycleData();
                    refreshHeartWidget();
                    updateCycleInfoLabel();
                    updateUIForSelectedDate(ui->calendarWidget->selectedDate());

                    QMessageBox::information(this, "记录成功",
                                             QString("经期持续 %1 天已记录。").arg(daysInPeriod));

                    requestAIAdvice();
                }
            }
        }

        // 如果正在经期中，跳过后续的“是否来了”的弹窗逻辑
        if (isInPeriod) {
            return;
        }
    }

    // ==================== 第二部分：经期开始确认 ====================
    // 如果没有历史记录，不弹窗
    QSqlQuery countQuery(db);
    countQuery.exec("SELECT COUNT(*) FROM period_history");
    countQuery.next();
    if (countQuery.value(0).toInt() == 0) {
        qDebug() << "无历史记录，跳过经期确认检查";
        return;
    }

    // 获取最近一次经期开始日期
    QSqlQuery lastQuery(db);
    lastQuery.exec("SELECT start_date FROM period_history ORDER BY start_date DESC LIMIT 1");
    if (!lastQuery.next()) {
        return;
    }
    QDate lastPeriod = QDate::fromString(lastQuery.value(0).toString(), "yyyy-MM-dd");

    // 预测下一次经期
    QDate expectedStart = lastPeriod.addDays(currentCycleData.cycleLength);
    int daysDiff = expectedStart.daysTo(today);

    qDebug() << "=== 经期确认检查 ===";
    qDebug() << "上次经期:" << lastPeriod.toString("yyyy-MM-dd");
    qDebug() << "预测开始:" << expectedStart.toString("yyyy-MM-dd");
    qDebug() << "今天:" << today.toString("yyyy-MM-dd");
    qDebug() << "天数差:" << daysDiff;

    // 检查今天是否已有记录
    QSqlQuery todayQuery(db);
    todayQuery.prepare("SELECT id FROM period_history WHERE start_date = ?");
    todayQuery.bindValue(0, today.toString("yyyy-MM-dd"));
    todayQuery.exec();
    bool hasTodayRecord = todayQuery.next();

    qDebug() << "今日是否有记录:" << hasTodayRecord;

    // 弹窗条件：推迟0-14天，或提前0-3天，且今天没有记录
    bool shouldPopup = false;
    if (!hasTodayRecord) {
        if (daysDiff >= 0 && daysDiff <= 14) {
            shouldPopup = true;
        } else if (daysDiff >= -3 && daysDiff < 0) {
            shouldPopup = true;
        }
    }

    qDebug() << "是否应该弹窗:" << shouldPopup;

    if (shouldPopup) {
        PeriodConfirmDialog dialog(healthCalc, expectedStart, this);
        if (dialog.exec() == QDialog::Accepted) {
            if (dialog.result() == PeriodConfirmDialog::Came) {
                // 记录今天为经期开始
                healthCalc->recordPeriodStart(today);

                // 设置经期状态
                isInPeriod = true;
                periodStartDate = today;

                // 重新加载数据
                currentCycleData = healthCalc->getLatestCycleData();
                currentCycleData.lastPeriodStart = today;
                healthCalc->saveCycleData(currentCycleData);

                recalculateAverages();
                currentCycleData = healthCalc->getLatestCycleData();

                refreshHeartWidget();
                updateCycleInfoLabel();
                updateInspireQuote();      // 🔥 新增：立即刷新语录
                updateUIForSelectedDate(ui->calendarWidget->selectedDate());

                QMessageBox::information(this, "记录成功",
                                         "经期已于今天开始，已为您记录。\n好好休息哦 💕\n\n我们会在经期结束后提醒您记录持续天数。");

                requestAIAdvice();
                requestAIInspiration();
            }
            else if (dialog.result() == PeriodConfirmDialog::Early) {
                QDate earlyDate = dialog.earlyDate();
                healthCalc->recordPeriodStart(earlyDate);

                isInPeriod = true;
                periodStartDate = earlyDate;

                currentCycleData = healthCalc->getLatestCycleData();
                currentCycleData.lastPeriodStart = earlyDate;
                healthCalc->saveCycleData(currentCycleData);

                recalculateAverages();
                currentCycleData = healthCalc->getLatestCycleData();

                refreshHeartWidget();
                updateCycleInfoLabel();
                updateUIForSelectedDate(ui->calendarWidget->selectedDate());

                QMessageBox::information(this, "记录成功",
                                         QString("经期已于 %1 开始，已为您记录。").arg(earlyDate.toString("yyyy-MM-dd")));

                requestAIAdvice();
            }
            else {
                // 还没来，显示暖心建议
                int daysLate = daysDiff;
                QString advice;

                if (daysLate <= 3) {
                    advice = "别担心，1-3天的波动完全正常。\n\n"
                             "压力、作息变化、饮食调整都可能导致经期轻微推迟。\n"
                             "放松心情，保持规律作息，她会如约而至的。";
                } else if (daysLate <= 7) {
                    advice = "经期推迟一周以内是常见现象。\n\n"
                             "可能的原因包括：近期压力大、睡眠不足、运动量变化等。\n"
                             "试着做一些舒缓的运动，如瑜伽或散步，帮助身体放松。";
                } else {
                    advice = "经期已推迟超过一周。\n\n"
                             "如果排除了怀孕可能，建议关注近期的生活状态：\n"
                             "• 是否压力过大？\n"
                             "• 作息是否规律？\n"
                             "• 饮食是否有大变化？\n\n"
                             "如果持续推迟超过两周，建议咨询医生哦。";
                }

                QMessageBox::information(this, "贴心小建议 🌸", advice);
                requestAIAdvice();  // 推迟时也可以请求 AI 建议
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
    QSqlDatabase db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);

    // 先查询有多少条记录
    query.exec("SELECT COUNT(*) FROM period_history");
    int recordCount = 0;
    if (query.next()) {
        recordCount = query.value(0).toInt();
    }

    qDebug() << "recalculateAverages: 记录数 =" << recordCount;

    // 计算平均经期持续天数
    query.exec("SELECT AVG(duration) FROM period_history WHERE duration IS NOT NULL AND duration > 0");
    if (query.next() && !query.value(0).isNull()) {
        double avgDuration = query.value(0).toDouble();
        if (avgDuration > 0) {
            currentCycleData.periodLength = qRound(avgDuration);
        }
    }

    // 只有当记录数 >= 2 时才计算平均周期
    if (recordCount >= 2) {
        QVector<QDate> dates;
        query.exec("SELECT start_date FROM period_history ORDER BY start_date ASC");
        while (query.next()) {
            QDate date = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
            if (date.isValid()) {
                dates.append(date);
            }
        }

        QVector<int> cycles;
        for (int i = 1; i < dates.size(); i++) {
            int cycle = dates[i-1].daysTo(dates[i]);
            if (cycle > 0 && cycle < 100) {
                cycles.append(cycle);
            }
        }

        if (!cycles.isEmpty()) {
            double sum = 0;
            for (int cycle : cycles) {
                sum += cycle;
            }
            double avgCycle = sum / cycles.size();
            currentCycleData.cycleLength = qRound(avgCycle);
        }
    }

    healthCalc->saveCycleData(currentCycleData);
}

// 创建"经期来了"记录对话框
void Widget::on_btnPeriodStart_clicked()
{
    PeriodStartDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QDate start = dialog.getStartDate();
        int duration = dialog.getDuration();
        bool ended = dialog.isEnded();

        QSqlDatabase db = DatabaseManager::instance().getDatabase();
        QSqlQuery query(db);

        if (ended && duration > 0) {
            // 已结束，有持续天数
            query.prepare("INSERT OR REPLACE INTO period_history (start_date, duration, end_date) VALUES (?, ?, ?)");
            query.bindValue(0, start.toString("yyyy-MM-dd"));
            query.bindValue(1, duration);
            query.bindValue(2, start.addDays(duration - 1).toString("yyyy-MM-dd"));
        } else {
            // 未结束，只记录开始日期，duration 字段留空或为0
            query.prepare("INSERT OR REPLACE INTO period_history (start_date, duration) VALUES (?, ?)");
            query.bindValue(0, start.toString("yyyy-MM-dd"));
            query.bindValue(1, 0);  // 0 表示尚未结束
        }

        if (query.exec()) {
            // 更新当前周期数据中的最近一次经期
            currentCycleData.lastPeriodStart = start;
            if (ended && duration > 0) {
                currentCycleData.periodLength = duration;
                isInPeriod = false;  // 已结束，退出经期状态
            } else {
                // 未结束，设置经期状态标志
                isInPeriod = true;
                periodStartDate = start;
            }

            // 保存到 cycle_data
            healthCalc->saveCycleData(currentCycleData);

            // 重新计算平均值
            recalculateAverages();

            // 重新从数据库加载，确保数据同步
            currentCycleData = healthCalc->getLatestCycleData();

            // 刷新界面
            refreshHeartWidget();
            updateCycleInfoLabel();
            updateInspireQuote();      // 🔥 新增
            updateUIForSelectedDate(ui->calendarWidget->selectedDate());

            QString msg;
            if (ended) {
                msg = QString("经期开始于 %1，持续 %2 天，已记录。").arg(start.toString("yyyy-MM-dd")).arg(duration);
            } else {
                msg = QString("经期开始于 %1 已记录。\n我们会在这几天关注您的经期情况，结束后请记得记录持续天数哦。").arg(start.toString("yyyy-MM-dd"));
            }
            QMessageBox::information(this, "记录成功", msg);
        } else {
            QMessageBox::warning(this, "错误", "保存失败：" + query.lastError().text());
            qDebug() << "经期记录 SQL 错误:" << query.lastError().text();
        }
    }
}

// 添加退出按钮功能
void Widget::on_btnExit_clicked()
{
    close();
}

void Widget::requestAIAdvice()
{
    // 获取最近心情记录
    QVector<QPair<QDate, int>> recentMoods;
    QSqlDatabase db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);
    query.exec("SELECT date, mood_level FROM mood_history ORDER BY date DESC LIMIT 7");
    while (query.next()) {
        QDate date = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
        int level = query.value(1).toInt();
        recentMoods.prepend(qMakePair(date, level));
    }

    // 保存生成的结果到 aiGeneratedAdvice
    connect(aiManager, &AIManager::adviceReady, this, [this](const QString &advice) {
        aiGeneratedAdvice = advice;
    });

    aiManager->generatePersonalizedAdvice(
        currentCycleData.lastPeriodStart,
        currentCycleData.cycleLength,
        currentCycleData.periodLength,
        recentMoods
        );
}

void Widget::requestAIInspiration()
{
    int phase = healthCalc->getCyclePhase(currentCycleData);
    int day = healthCalc->getCurrentDayInCycle(currentCycleData);

    QVector<int> moodLevels;
    QSqlDatabase db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);
    query.exec("SELECT mood_level FROM mood_history ORDER BY date DESC LIMIT 5");
    while (query.next()) {
        moodLevels.append(query.value(0).toInt());
    }

    aiManager->generateDailyInspiration(phase, day, moodLevels);
}

// v5.0新增
void Widget::onAIAdviceReady(const QString &advice)
{
    aiGeneratedAdvice = advice;
    // 可以显示在健康数据界面，或者新增一个"AI建议"区域
    qDebug() << "AI 建议已生成:" << advice;
}

void Widget::onAIInspirationReady(const QString &inspiration)
{
    // 替换或补充原有的固定语录
    ui->labelInspireQuote->setText("✨ " + inspiration + " ✨");
}

void Widget::onAIError(const QString &message)
{
    qDebug() << "AI 请求失败:" << message;
    // 失败时使用本地语录
    updateInspireQuote();
}

// 情绪历史管理
void Widget::on_btnMoodHistory_clicked()
{
    MoodHistoryDialog dialog(this);

    // 连接数据变化信号，刷新 AI 语录
    connect(&dialog, &MoodHistoryDialog::dataChanged, this, [this]() {
        requestAIInspiration();
    });

    dialog.exec();
}