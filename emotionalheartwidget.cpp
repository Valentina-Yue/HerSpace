#include "emotionalheartwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QTime>
#include <cmath>

EmotionalHeartWidget::EmotionalHeartWidget(QWidget *parent)
    : QWidget(parent), m_scale(1.0), m_moodLevel(0.5), m_cyclePhase(0) {
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

    // 根据周期阶段设置基础颜色
    QColor baseColor;
    switch(m_cyclePhase) {
    case 0: baseColor = QColor(255, 105, 180, 200); break; // 月经期 - 粉色
    case 1: baseColor = QColor(100, 149, 237, 200); break; // 卵泡期 - 蓝色
    case 2: baseColor = QColor(255, 215, 0, 200); break;   // 排卵期 - 金色
    case 3: baseColor = QColor(148, 0, 211, 200); break;   // 黄体期 - 紫色
    default: baseColor = QColor(255, 105, 180, 200);
    }

    // 根据情绪值调整颜色
    int red = static_cast<int>(baseColor.red() * m_moodLevel + 255 * (1 - m_moodLevel) * 0.2);
    int green = static_cast<int>(baseColor.green() * m_moodLevel + 255 * (1 - m_moodLevel) * 0.2);
    int blue = static_cast<int>(baseColor.blue() * m_moodLevel + 255 * (1 - m_moodLevel) * 0.2);
    QColor heartColor(red, green, blue, 200);

    painter.setBrush(QBrush(heartColor));
    painter.setPen(Qt::NoPen);

    QPointF center = rect().center();
    qreal size = 50 * m_scale;

    // 绘制爱心形状
    QPainterPath path;
    path.moveTo(center.x(), center.y() + size * 0.3);
    path.cubicTo(center.x() - size, center.y() - size,
                 center.x() - size, center.y() - size * 0.5,
                 center.x(), center.y() + size * 0.5);
    path.cubicTo(center.x() + size, center.y() - size * 0.5,
                 center.x() + size, center.y() - size,
                 center.x(), center.y() + size * 0.3);

    painter.drawPath(path);

    // 绘制周期阶段文字
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    QString phaseText;
    switch(m_cyclePhase) {
    case 0: phaseText = "月经期"; break;
    case 1: phaseText = "卵泡期"; break;
    case 2: phaseText = "排卵期"; break;
    case 3: phaseText = "黄体期"; break;
    default: phaseText = "未知期";
    }
    painter.drawText(rect(), Qt::AlignCenter, phaseText);
}