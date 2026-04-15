#include "inspiremanager.h"
#include <QRandomGenerator>

InspireManager::InspireManager()
{
    initQuotes();
}

void InspireManager::initQuotes()
{
    // 月经期语录
    phaseQuotes[0] = {
        "给自己一杯热茶，好好休息，你值得被温柔对待。",
        "这几天，允许自己慢下来，身体在悄悄蓄力。",
        "倾听身体的声音，它正在完成一次美丽的更新。"
    };

    // 卵泡期语录
    phaseQuotes[1] = {
        "新的一周，能量满满，去追逐你的小目标吧！",
        "现在的你像春天的新芽，充满生机与可能。",
        "把握这段精力旺盛的时光，做一件让自己开心的事。"
    };

    // 排卵期语录
    phaseQuotes[2] = {
        "你的光芒正在闪耀，记得多微笑哦。",
        "创造力与魅力值 up！适合表达自己。",
        "享受此刻的活力，也别忘了补充水分。"
    };

    // 黄体期语录
    phaseQuotes[3] = {
        "放慢脚步，给自己一个温暖的拥抱。",
        "偶尔的疲惫是身体在提醒你：该宠爱自己了。",
        "做一些舒缓的运动，比如瑜伽或散步，会舒服很多。"
    };

    // 通用语录（当阶段未知时使用）
    generalQuotes = {
        "每一天，都是爱自己的开始。",
        "你比你想象的更坚强，更美丽。",
        "关注自己的感受，是最高级的自律。",
        "小小的仪式感，能让平凡的日子发光。"
    };
}

QString InspireManager::getQuoteForPhase(int phase) const
{
    if (phaseQuotes.contains(phase) && !phaseQuotes[phase].isEmpty()) {
        const QList<QString>& list = phaseQuotes[phase];
        int idx = QRandomGenerator::global()->bounded(list.size());
        return list[idx];
    }
    return getRandomQuote();
}

QString InspireManager::getRandomQuote() const
{
    if (generalQuotes.isEmpty()) return "愿你今天有好心情。";
    int idx = QRandomGenerator::global()->bounded(generalQuotes.size());
    return generalQuotes[idx];
}