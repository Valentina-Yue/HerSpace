#include "healthcalculator.h"
#include "databasemanager.h"
#include <QDebug>

HealthCalculator::HealthCalculator(QObject *parent)
    : QObject(parent)
{
}

HealthCalculator::~HealthCalculator()
{
}

QSqlDatabase HealthCalculator::getDB() const
{
    return DatabaseManager::instance().getDatabase();
}

QDate HealthCalculator::predictNextPeriod(const CycleData& data)
{
    return data.lastPeriodStart.addDays(data.cycleLength);
}

int HealthCalculator::getCurrentDayInCycle(const CycleData& data)
{
    QDate today = QDate::currentDate();
    int daysSinceLast = data.lastPeriodStart.daysTo(today);

    if (daysSinceLast < 0) {
        return daysSinceLast + data.cycleLength;
    }

    return daysSinceLast % data.cycleLength;
}

int HealthCalculator::getCyclePhase(const CycleData& data)
{
    int day = getCurrentDayInCycle(data);

    if (day >= 1 && day <= data.periodLength) {
        return 0;
    } else if (day > data.periodLength && day <= 14) {
        return 1;
    } else if (day > 14 && day <= 16) {
        return 2;
    } else {
        return 3;
    }
}

QString HealthCalculator::getAdvice(int day, int phase)
{
    switch(phase) {
    case 0: return "生理期：注意保暖，多喝热水，避免剧烈运动。";
    case 1: return "卵泡期：精力充沛，适合进行高强度运动和工作。";
    case 2: return "排卵期：情绪可能波动，请保持心情愉快。";
    case 3: return "黄体期：注意休息，避免过度疲劳。";
    default: return "注意休息，保持健康的生活方式。";
    }
}

bool HealthCalculator::saveCycleData(const CycleData& data)
{
    QSqlDatabase db = getDB();
    QSqlQuery query(db);

    query.prepare("INSERT INTO cycle_data (last_period_start, cycle_length, period_length) "
                  "VALUES (?, ?, ?)");
    query.bindValue(0, data.lastPeriodStart.toString("yyyy-MM-dd"));
    query.bindValue(1, data.cycleLength);
    query.bindValue(2, data.periodLength);

    if (!query.exec()) {
        qDebug() << "保存数据失败:" << query.lastError().text();
        return false;
    }

    return true;
}

CycleData HealthCalculator::getLatestCycleData()
{
    CycleData data;
    QSqlDatabase db = getDB();

    // 1. 从 period_history 获取最近一次经期（按日期排序）
    QSqlQuery histQuery(db);
    histQuery.exec("SELECT start_date, duration FROM period_history ORDER BY start_date DESC LIMIT 1");

    if (histQuery.next()) {
        data.lastPeriodStart = QDate::fromString(histQuery.value(0).toString(), "yyyy-MM-dd");
        int histDuration = histQuery.value(1).toInt();
        if (histDuration > 0) {
            data.periodLength = histDuration;
        }
        qDebug() << "getLatestCycleData 从 period_history 读取 lastPeriodStart:"
                 << data.lastPeriodStart.toString("yyyy-MM-dd");
    }

    // 2. 从 cycle_data 读取平均周期和平均持续天数
    QSqlQuery query("SELECT * FROM cycle_data ORDER BY id DESC LIMIT 1", db);

    if (query.next()) {
        // 🔥 周期长度：使用 cycle_data 中保存的平均值
        int savedCycle = query.value("cycle_length").toInt();
        if (savedCycle > 0) {
            data.cycleLength = savedCycle;
        }

        // 🔥 经期持续天数：优先使用 cycle_data 中保存的平均值
        int savedPeriod = query.value("period_length").toInt();
        if (savedPeriod > 0) {
            data.periodLength = savedPeriod;  // 覆盖从 period_history 获取的单次值
        }

        qDebug() << "getLatestCycleData 从 cycle_data 读取:"
                 << "cycleLength:" << data.cycleLength
                 << "periodLength:" << data.periodLength;
    }

    // 3. 设置默认值
    if (!data.lastPeriodStart.isValid()) {
        data.lastPeriodStart = QDate::currentDate();
    }
    if (data.cycleLength <= 0) {
        data.cycleLength = 28;
    }
    if (data.periodLength <= 0) {
        data.periodLength = 5;
    }

    qDebug() << "getLatestCycleData 最终返回:"
             << "lastPeriodStart:" << data.lastPeriodStart.toString("yyyy-MM-dd")
             << "cycleLength:" << data.cycleLength
             << "periodLength:" << data.periodLength;

    return data;
}

bool HealthCalculator::recordPeriodStart(const QDate &startDate)
{
    CycleData current = getLatestCycleData();
    QSqlDatabase db = getDB();
    QSqlQuery query(db);

    query.prepare("INSERT OR REPLACE INTO period_history (start_date, duration) VALUES (?, ?)");
    query.bindValue(0, startDate.toString("yyyy-MM-dd"));
    query.bindValue(1, current.periodLength);

    if (!query.exec()) {
        qDebug() << "记录经期开始失败:" << query.lastError().text();
        return false;
    }

    current.lastPeriodStart = startDate;
    return saveCycleData(current);
}