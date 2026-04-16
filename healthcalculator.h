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
    int getCyclePhase(const CycleData& data);
    QString getAdvice(int day, int phase);
    bool saveCycleData(const CycleData& data);
    CycleData getLatestCycleData();
    bool recordPeriodStart(const QDate &startDate);

private:
    QSqlDatabase getDB() const;  // 辅助函数
};

#endif // HEALTHCALCULATOR_H