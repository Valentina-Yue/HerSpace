#ifndef EMOTIONALHEARTWIDGET_H
#define EMOTIONALHEARTWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QTimer>

class EmotionalHeartWidget : public QWidget {
    Q_OBJECT
public:
    explicit EmotionalHeartWidget(QWidget *parent = nullptr);
    void setMoodLevel(double level);
    void setCyclePhase(int phase); // 0:月经期, 1:卵泡期, 2:排卵期, 3:黄体期

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateAnimation();

private:
    double m_scale;
    double m_moodLevel;
    int m_cyclePhase;
    QTimer *timer;
};

#endif // EMOTIONALHEARTWIDGET_H