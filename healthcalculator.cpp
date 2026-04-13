#include "healthcalculator.h"
#include <QDebug>

HealthCalculator::HealthCalculator(QObject *parent)
    : QObject(parent) {
    if (!initDatabase()) {
        qDebug() << "数据库初始化失败";
    }
}

HealthCalculator::~HealthCalculator() {
    if (db.isOpen()) {
        db.close();
    }
}

bool HealthCalculator::initDatabase() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("her_space.db");

    if (!db.open()) {
        qDebug() << "无法打开数据库:" << db.lastError();
        return false;
    }

    // 创建周期数据表
    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS cycle_data ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "last_period_start TEXT, "
                    "cycle_length INTEGER, "
                    "period_length INTEGER)")) {
        qDebug() << "创建表失败:" << query.lastError();
        return false;
    }

    return true;
}

QDate HealthCalculator::predictNextPeriod(const CycleData& data) {
    return data.lastPeriodStart.addDays(data.cycleLength);
}

int HealthCalculator::getCurrentDayInCycle(const CycleData& data) {
    QDate today = QDate::currentDate();
    int daysSinceLast = data.lastPeriodStart.daysTo(today);  // 修正：daysTo 而不是 daysSince

    // 处理跨周期情况
    if (daysSinceLast < 0) {
        return daysSinceLast + data.cycleLength;
    }

    return daysSinceLast % data.cycleLength;
}

int HealthCalculator::getCyclePhase(const CycleData& data) {
    int day = getCurrentDayInCycle(data);

    // 月经期：第1-5天
    if (day >= 1 && day <= data.periodLength) {
        return 0;
    }
    // 卵泡期：第6-14天
    else if (day > data.periodLength && day <= 14) {
        return 1;
    }
    // 排卵期：第15-16天
    else if (day > 14 && day <= 16) {
        return 2;
    }
    // 黄体期：第17-28天
    else {
        return 3;
    }
}

QString HealthCalculator::getAdvice(int day, int phase) {
    switch(phase) {
    case 0: return "生理期：注意保暖，多喝热水，避免剧烈运动。";
    case 1: return "卵泡期：精力充沛，适合进行高强度运动和工作。";
    case 2: return "排卵期：情绪可能波动，请保持心情愉快。";
    case 3: return "黄体期：注意休息，避免过度疲劳。";
    default: return "注意休息，保持健康的生活方式。";
    }
}

bool HealthCalculator::saveCycleData(const CycleData& data) {
    QSqlQuery query;
    query.prepare("INSERT INTO cycle_data (last_period_start, cycle_length, period_length) "
                  "VALUES (:last_period_start, :cycle_length, :period_length)");
    query.bindValue(":last_period_start", data.lastPeriodStart.toString("yyyy-MM-dd"));
    query.bindValue(":cycle_length", data.cycleLength);
    query.bindValue(":period_length", data.periodLength);

    if (!query.exec()) {
        qDebug() << "保存数据失败:" << query.lastError();
        return false;
    }

    return true;
}

CycleData HealthCalculator::getLatestCycleData() {
    CycleData data;
    QSqlQuery query("SELECT * FROM cycle_data ORDER BY id DESC LIMIT 1");

    if (query.next()) {
        data.lastPeriodStart = QDate::fromString(query.value("last_period_start").toString(), "yyyy-MM-dd");
        data.cycleLength = query.value("cycle_length").toInt();
        data.periodLength = query.value("period_length").toInt();
    } else {
        // 如果没有数据，使用默认值
        data.lastPeriodStart = QDate::currentDate();
        data.cycleLength = 28;
        data.periodLength = 5;
    }

    return data;
}