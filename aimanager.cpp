#include "aimanager.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

AIManager::AIManager(QObject *parent)
    : QObject(parent)
    , networkManager(new QNetworkAccessManager(this))
{
    // 🔥 请替换为你的 DeepSeek API Key
    apiKey = "sk-acb863704ba547d3ab23f8310381d242";
    apiUrl = "https://api.deepseek.com/v1/chat/completions";
}

void AIManager::generatePersonalizedAdvice(
    const QDate &lastPeriodStart,
    int cycleLength,
    int periodLength,
    const QVector<QPair<QDate, int>> &recentMoods,
    const QString &userNote)
{
    // 1. 计算当前周期信息
    QDate today = QDate::currentDate();
    int daysSinceLast = lastPeriodStart.daysTo(today);

    QString cycleStatus;
    QString phaseName;
    int currentDay = 0;
    bool isDelayed = false;
    int daysLate = 0;

    if (daysSinceLast < 0) {
        cycleStatus = "数据异常（上次经期在未来）";
        phaseName = "未知";
    } else {
        // 检查是否推迟
        QDate expectedStart = lastPeriodStart.addDays(cycleLength);
        daysLate = expectedStart.daysTo(today);

        if (daysLate > 0) {
            isDelayed = true;
            cycleStatus = QString("经期已推迟 %1 天").arg(daysLate);
            phaseName = "推迟中";
        } else {
            // 正常周期内
            currentDay = (daysSinceLast % cycleLength) + 1;
            if (currentDay <= periodLength) {
                phaseName = "月经期";
            } else if (currentDay <= 14) {
                phaseName = "卵泡期";
            } else if (currentDay <= 16) {
                phaseName = "排卵期";
            } else {
                phaseName = "黄体期";
            }
            cycleStatus = QString("处于%1第%2天").arg(phaseName).arg(currentDay);
        }
    }

    // 2. 分析近期心情趋势
    QString moodTrend;
    if (!recentMoods.isEmpty()) {
        int sum = 0;
        for (const auto &mood : recentMoods) {
            sum += mood.second;
        }
        double avg = static_cast<double>(sum) / recentMoods.size();

        if (avg >= 4.0) moodTrend = "近期心情愉悦，状态良好";
        else if (avg >= 3.0) moodTrend = "近期心情平稳，偶有小波动";
        else if (avg >= 2.0) moodTrend = "近期心情略显低落，需要更多关爱";
        else moodTrend = "近期情绪不佳，可能需要倾诉和放松";

        // 检查趋势（上升/下降）
        if (recentMoods.size() >= 3) {
            int lastIdx = recentMoods.size() - 1;
            if (recentMoods[lastIdx].second > recentMoods[lastIdx-1].second)
                moodTrend += "，情绪正在回升";
            else if (recentMoods[lastIdx].second < recentMoods[lastIdx-1].second)
                moodTrend += "，情绪略有下滑";
        }
    } else {
        moodTrend = "暂无近期心情记录";
    }

    // 3. 构建详细的提示词
    QString prompt = QString(
                         "你是一位专业、温柔、贴心的女性健康顾问，名叫「小暖」。\n"
                         "你的服务对象是一位使用 HerSpace 女性健康 App 的用户。\n\n"
                         "请根据以下用户数据，生成一段 200-300 字的个性化健康建议。\n"
                         "语气要温暖、鼓励、专业，像一位知心闺蜜。\n"
                         "内容可以包括：饮食建议、运动提示、情绪调节小技巧、周期小知识等。\n\n"
                         "【用户数据】\n"
                         "- 上次经期开始日期：%1\n"
                         "- 平均周期长度：%2 天\n"
                         "- 平均经期持续：%3 天\n"
                         "- 今天日期：%4\n"
                         "- 当前状态：%5\n"
                         "- 心情概况：%6\n"
                         ).arg(lastPeriodStart.toString("yyyy-MM-dd"))
                         .arg(cycleLength).arg(periodLength)
                         .arg(today.toString("yyyy-MM-dd"))
                         .arg(cycleStatus)
                         .arg(moodTrend);

    // 添加最近的具体心情记录（最多5条）
    if (!recentMoods.isEmpty()) {
        prompt += "\n- 最近心情记录：";
        int start = qMax(0, recentMoods.size() - 5);
        for (int i = start; i < recentMoods.size(); ++i) {
            prompt += QString("\n  %1: %2级")
                          .arg(recentMoods[i].first.toString("MM-dd"))
                          .arg(recentMoods[i].second);
        }
    }

    // 如果有用户额外备注，加入提示词
    if (!userNote.isEmpty()) {
        prompt += QString("\n- 用户备注：%1").arg(userNote);
    }

    // 根据推迟情况加入专业指导
    if (isDelayed) {
        if (daysLate <= 7) {
            prompt += "\n\n注：用户经期稍有推迟，属于正常波动，请给予温和安抚。";
        } else {
            prompt += "\n\n注：用户经期推迟已超过一周，请从专业角度提醒关注身体健康，必要时建议就医。";
        }
    }

    prompt += "\n\n请输出你的建议（直接输出建议内容，不要有「小暖：」之类的前缀，自然分段）：";

    QJsonObject requestBody = buildRequestBody(prompt);

    // 4. 发送网络请求
    // 🔥 修复：使用赋值初始化避免 most vexing parse
    QNetworkRequest request = QNetworkRequest(QUrl(apiUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(requestBody).toJson());
    reply->setProperty("type", "advice");
    connect(reply, &QNetworkReply::finished, this, &AIManager::onAdviceReplyFinished);
}

void AIManager::generateDailyInspiration(
    int cyclePhase,
    int currentDay,
    const QVector<int> &recentMoodLevels)
{
    QString phaseName;
    switch(cyclePhase) {
    case 0: phaseName = "月经期"; break;
    case 1: phaseName = "卵泡期"; break;
    case 2: phaseName = "排卵期"; break;
    case 3: phaseName = "黄体期"; break;
    default: phaseName = "未知";
    }

    // 计算平均心情
    double avgMood = 3.0;
    if (!recentMoodLevels.isEmpty()) {
        int sum = 0;
        for (int level : recentMoodLevels) {
            sum += level;
        }
        avgMood = static_cast<double>(sum) / recentMoodLevels.size();
    }

    QString moodDesc;
    if (avgMood >= 4.0) moodDesc = "心情愉悦";
    else if (avgMood >= 3.0) moodDesc = "心情平稳";
    else if (avgMood >= 2.0) moodDesc = "心情有些低落";
    else moodDesc = "心情不太好";

    QString prompt = QString(
                         "你是一位温柔贴心的女性健康伙伴「小暖」。\n"
                         "用户当前处于%1的第%2天，近期%3。\n"
                         "请用一句话（不超过40字）给她一句暖心鼓励或正能量语录。"
                         "语气要温暖、治愈，像闺蜜的鼓励。\n\n"
                         "直接输出语录内容，不要有任何前缀："
                         ).arg(phaseName).arg(currentDay).arg(moodDesc);

    QJsonObject requestBody = buildRequestBody(prompt);

    // 🔥 修复：使用赋值初始化
    QNetworkRequest request = QNetworkRequest(QUrl(apiUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(requestBody).toJson());
    reply->setProperty("type", "inspiration");
    connect(reply, &QNetworkReply::finished, this, &AIManager::onInspirationReplyFinished);
}

QJsonObject AIManager::buildRequestBody(const QString &prompt)
{
    QJsonObject requestBody;
    requestBody["model"] = "deepseek-chat";
    requestBody["temperature"] = 0.7;
    requestBody["max_tokens"] = 500;

    QJsonArray messages;
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = prompt;
    messages.append(userMessage);

    requestBody["messages"] = messages;

    return requestBody;
}

void AIManager::onAdviceReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        QJsonArray choices = obj["choices"].toArray();
        if (!choices.isEmpty()) {
            QString content = choices[0].toObject()["message"].toObject()["content"].toString();
            emit adviceReady(content);
        } else {
            emit error("API 返回数据格式错误");
        }
    } else {
        emit error("网络错误：" + reply->errorString());
    }

    reply->deleteLater();
}

void AIManager::onInspirationReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        QJsonArray choices = obj["choices"].toArray();
        if (!choices.isEmpty()) {
            QString content = choices[0].toObject()["message"].toObject()["content"].toString();
            emit inspirationReady(content);
        } else {
            emit error("API 返回数据格式错误");
        }
    } else {
        emit error("网络错误：" + reply->errorString());
    }

    reply->deleteLater();
}