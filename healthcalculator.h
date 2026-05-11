#ifndef HEALTHCALCULATOR_H
#define HEALTHCALCULATOR_H

#include <QObject>
#include <QDate>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

struct CycleData {
    QDate lastPeriodStart;
    int cycleLength = 28;
    int periodLength = 5;
};

class HealthCalculator : public QObject {
    Q_OBJECT
public:
    explicit HealthCalculator(QObject *parent = nullptr);
    ~HealthCalculator();

    // 不需要 initDatabase，由单例管理
    QDate predictNextPeriod(const CycleData& data);
    int getCurrentDayInCycle(const CycleData& data);
    // 新增可选参数 isDelayed，用于返回是否处于推迟状态
    int getCyclePhase(const CycleData& data, bool *isDelayed = nullptr);
    QString getAdvice(int day, int phase);
    bool saveCycleData(const CycleData& data);
    CycleData getLatestCycleData();
    bool recordPeriodStart(const QDate &startDate);

    // v5.0新增
    // 保存情绪记录
    bool saveMoodRecord(const QDate &date, int moodLevel, const QString &diary = "");
    // 获取指定日期范围的情绪记录
    QList<QPair<QDate, int>> getMoodHistory(const QDate &startDate, const QDate &endDate);
    // 获取最近N条情绪记录
    QList<QPair<QDate, int>> getRecentMoodRecords(int count = 30);

private:
    QSqlDatabase getDB() const;  // 辅助函数
};

#endif // HEALTHCALCULATOR_H