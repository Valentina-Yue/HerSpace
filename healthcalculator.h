#ifndef HEALTHCALCULATOR_H
#define HEALTHCALCULATOR_H

#include <QObject>
#include <QDate>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

struct CycleData {
    QDate lastPeriodStart;
    int cycleLength = 28; // 平均周期
    int periodLength = 5; // 平均持续天数
};

class HealthCalculator : public QObject {
    Q_OBJECT
public:
    explicit HealthCalculator(QObject *parent = nullptr);
    ~HealthCalculator();

    // 初始化数据库
    bool initDatabase();

    // 预测下一次生理期开始时间
    QDate predictNextPeriod(const CycleData& data);

    // 计算当前处于周期的第几天
    int getCurrentDayInCycle(const CycleData& data);

    // 获取周期阶段
    int getCyclePhase(const CycleData& data);

    // 智能建议
    QString getAdvice(int day, int phase);

    // 保存周期数据
    bool saveCycleData(const CycleData& data);

    // 获取最新周期数据
    CycleData getLatestCycleData();

private:
    QSqlDatabase db;
};

#endif // HEALTHCALCULATOR_H