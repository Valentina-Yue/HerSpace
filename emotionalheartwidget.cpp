#include "emotionalheartwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QTime>
#include <cmath>

EmotionalHeartWidget::EmotionalHeartWidget(QWidget *parent)
    : QWidget(parent), m_scale(1.0), m_moodLevel(0.5), m_cyclePhase(0) {

    setAttribute(Qt::WA_TranslucentBackground);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &EmotionalHeartWidget::updateAnimation);
    timer->start(50);
}

void EmotionalHeartWidget::setMoodLevel(double level) {
    m_moodLevel = level;
    update();
}

void EmotionalHeartWidget::setCyclePhase(int phase) {
    m_cyclePhase = phase;
    update();
}

void EmotionalHeartWidget::updateAnimation() {
    m_scale = 1.0 + 0.1 * sin(QTime::currentTime().msec() / 100.0 * M_PI);
    update();
}

void EmotionalHeartWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 直接填充透明背景（依靠父窗口背景，无需手动清除）
    painter.fillRect(rect(), Qt::transparent);

    // 根据周期阶段设置基础颜色
    QColor baseColor;
    QString phaseText;

    if (m_cyclePhase == -1) {
        // 推迟中 - 柔和灰棕色
        baseColor = QColor(180, 160, 140, 200);
        phaseText = "推迟中";
    } else {
        switch(m_cyclePhase) {
        case 0: baseColor = QColor(255, 105, 180, 200); phaseText = "月经期"; break; // 粉色
        case 1: baseColor = QColor(100, 149, 237, 200); phaseText = "卵泡期"; break; // 蓝色
        case 2: baseColor = QColor(255, 215, 0, 200);   phaseText = "排卵期"; break; // 金色
        case 3: baseColor = QColor(148, 0, 211, 200);   phaseText = "黄体期"; break; // 紫色
        default: baseColor = QColor(255, 105, 180, 200); phaseText = "未知期";
        }
    }

    // 根据情绪值调整颜色亮度（情绪越好越亮）
    int red = static_cast<int>(baseColor.red() * m_moodLevel + 255 * (1 - m_moodLevel) * 0.2);
    int green = static_cast<int>(baseColor.green() * m_moodLevel + 255 * (1 - m_moodLevel) * 0.2);
    int blue = static_cast<int>(baseColor.blue() * m_moodLevel + 255 * (1 - m_moodLevel) * 0.2);
    QColor heartColor(red, green, blue, 200);

    painter.setBrush(QBrush(heartColor));
    painter.setPen(Qt::NoPen);

    QPointF center = rect().center();
    qreal size = qMin(width(), height()) * 0.25 * m_scale;

    // 绘制爱心
    QPainterPath path;
    path.moveTo(center.x(), center.y() + size * 0.3);
    path.cubicTo(center.x() - size, center.y() - size,
                 center.x() - size, center.y() - size * 0.5,
                 center.x(), center.y() + size * 0.5);
    path.cubicTo(center.x() + size, center.y() - size * 0.5,
                 center.x() + size, center.y() - size,
                 center.x(), center.y() + size * 0.3);

    painter.drawPath(path);

    // 绘制阶段文字
    painter.setPen(Qt::white);
    painter.setFont(QFont("Quicksand", 12, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter, phaseText);
}
