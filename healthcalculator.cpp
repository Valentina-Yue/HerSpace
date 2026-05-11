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

int HealthCalculator::getCurrentDayInCycle(const CycleData& data) {
    QDate today = QDate::currentDate();
    int daysSinceLast = data.lastPeriodStart.daysTo(today);

    // 处理跨周期情况（选中日期在上次经期之前）
    if (daysSinceLast < 0) {
        return daysSinceLast + data.cycleLength + 1;  // 🔥 +1 保持1-based
    }

    // 🔥 统一使用公式：余数 + 1
    return (daysSinceLast % data.cycleLength) + 1;
}

int HealthCalculator::getCyclePhase(const CycleData& data, bool *isDelayed)
{
    QDate today = QDate::currentDate();
    QDate expectedStart = data.lastPeriodStart.addDays(data.cycleLength);
    int daysDiff = expectedStart.daysTo(today);

    if (isDelayed) {
        *isDelayed = (daysDiff > 0);
    }

    // 如果推迟超过 1 天，返回 -1 表示无法确定阶段
    if (daysDiff > 1) {
        return -1;  // 经期推迟中
    }

    int day = getCurrentDayInCycle(data);

    if (day >= 1 && day <= data.periodLength) {
        return 0;  // 月经期
    } else if (day > data.periodLength && day <= 14) {
        return 1;  // 卵泡期
    } else if (day > 14 && day <= 16) {
        return 2;  // 排卵期
    } else {
        return 3;  // 黄体期
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
        // 在 getLatestCycleData() 中，读取 duration 后检查是否有效
        int histDuration = histQuery.value(1).toInt();  // 如果是 NULL，toInt() 返回 0
        if (histDuration > 0) {
            data.periodLength = histDuration;
        }
        // 否则保留 cycle_data 中的平均值
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
    QSqlDatabase db = getDB();
    QSqlQuery query(db);

    // 🔥 插入时不设置 duration（留空，表示未结束）
    query.prepare("INSERT OR REPLACE INTO period_history (start_date) VALUES (?)");
    query.bindValue(0, startDate.toString("yyyy-MM-dd"));

    if (!query.exec()) {
        qDebug() << "记录经期开始失败:" << query.lastError().text();
        return false;
    }

    // 更新 cycle_data 中的 last_period_start
    CycleData current = getLatestCycleData();
    current.lastPeriodStart = startDate;
    return saveCycleData(current);
}

// v5.0新增：情绪记录方法
bool HealthCalculator::saveMoodRecord(const QDate &date, int moodLevel, const QString &diary)
{
    QSqlDatabase db = getDB();
    QSqlQuery query(db);

    query.prepare("INSERT OR REPLACE INTO mood_history (date, mood_level, diary) VALUES (?, ?, ?)");
    query.bindValue(0, date.toString("yyyy-MM-dd"));
    query.bindValue(1, moodLevel);
    query.bindValue(2, diary);

    if (!query.exec()) {
        qDebug() << "保存情绪记录失败:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<QPair<QDate, int>> HealthCalculator::getMoodHistory(const QDate &startDate, const QDate &endDate)
{
    QList<QPair<QDate, int>> result;
    QSqlDatabase db = getDB();
    QSqlQuery query(db);

    query.prepare("SELECT date, mood_level FROM mood_history WHERE date >= ? AND date <= ? ORDER BY date ASC");
    query.bindValue(0, startDate.toString("yyyy-MM-dd"));
    query.bindValue(1, endDate.toString("yyyy-MM-dd"));
    query.exec();

    while (query.next()) {
        QDate date = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
        int level = query.value(1).toInt();
        result.append({date, level});
    }
    return result;
}

QList<QPair<QDate, int>> HealthCalculator::getRecentMoodRecords(int count)
{
    QList<QPair<QDate, int>> result;
    QSqlDatabase db = getDB();
    QSqlQuery query(db);

    query.prepare("SELECT date, mood_level FROM mood_history ORDER BY date DESC LIMIT ?");
    query.bindValue(0, count);
    query.exec();

    while (query.next()) {
        QDate date = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
        int level = query.value(1).toInt();
        result.prepend({date, level});  // 按日期升序
    }
    return result;
}