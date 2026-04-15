#ifndef INSPIREMANAGER_H
#define INSPIREMANAGER_H

#include <QString>
#include <QMap>
#include <QList>

class InspireManager
{
public:
    InspireManager();

    // 根据周期阶段（0-3）获取一句语录
    QString getQuoteForPhase(int phase) const;

    // 获取一句随机通用语录
    QString getRandomQuote() const;

private:
    void initQuotes();

    // 按阶段存储语录列表
    QMap<int, QList<QString>> phaseQuotes;
    QList<QString> generalQuotes;
};

#endif // INSPIREMANAGER_H