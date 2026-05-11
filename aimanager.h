#ifndef AIMANAGER_H
#define AIMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QDate>

class AIManager : public QObject
{
    Q_OBJECT

public:
    explicit AIManager(QObject *parent = nullptr);

    // 生成个性化建议
    void generatePersonalizedAdvice(
        const QDate &lastPeriodStart,
        int cycleLength,
        int periodLength,
        const QVector<QPair<QDate, int>> &recentMoods,  // 最近心情记录
        const QString &userNote = QString()
        );

    // 生成每日暖心语录
    void generateDailyInspiration(
        int cyclePhase,
        int currentDay,
        const QVector<int> &recentMoodLevels
        );

signals:
    void adviceReady(const QString &advice);
    void inspirationReady(const QString &inspiration);
    void error(const QString &message);

private slots:
    void onAdviceReplyFinished();
    void onInspirationReplyFinished();

private:
    QNetworkAccessManager *networkManager;
    QString apiKey;
    QString apiUrl;

    QString buildPrompt(const QJsonObject &data);
    QJsonObject buildRequestBody(const QString &prompt);
};

#endif // AIMANAGER_H