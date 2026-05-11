#include "healthstatsdialog.h"
#include "ui_healthstatsdialog.h"
#include "databasemanager.h"
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QSqlQuery>
#include <QDebug>
#include <algorithm>
#include <QVBoxLayout>
#include <QTimer>
#include <QGroupBox>
#include <QTextEdit>
#include <QPushButton>

HealthStatsDialog::HealthStatsDialog(HealthCalculator *hc, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::HealthStatsDialog)
    , healthCalc(hc)  // 🔥 初始化
{
    ui->setupUi(this);
    setWindowTitle("健康统计");
    setMinimumSize(700, 800);  // 增大尺寸以容纳更多内容

    // 🔥 每次都重新加载
    loadStatistics();

    // 连接信号
    connect(ui->btnClose, &QPushButton::clicked, this, &HealthStatsDialog::on_btnClose_clicked);
    connect(ui->btnRefreshAI, &QPushButton::clicked, this, &HealthStatsDialog::on_btnRefreshAI_clicked);
}

HealthStatsDialog::~HealthStatsDialog()
{
    delete ui;
}

void HealthStatsDialog::setAIAdvice(const QString &advice)
{
    if (ui->textAIAdvice) {
        QString processedAdvice = advice;
        processedAdvice.replace("\n", "<br>");
        ui->textAIAdvice->setHtml(QString("<p style='color:#5a4a42; line-height:1.6;'>%1</p>")
                                      .arg(processedAdvice));
    }
    if (ui->btnRefreshAI) {
        ui->btnRefreshAI->setEnabled(true);
    }
}

void HealthStatsDialog::setAILoading()
{
    if (ui->textAIAdvice) {
        ui->textAIAdvice->setHtml("<p style='color:#7d6b7a; font-style:italic;'>🤔 小暖正在为您生成个性化建议，请稍候...</p>");
    }
    if (ui->btnRefreshAI) {
        ui->btnRefreshAI->setEnabled(false);
    }
}

void HealthStatsDialog::setAIError(const QString &message)
{
    if (ui->btnRefreshAI) {
        ui->btnRefreshAI->setEnabled(true);
    }
    if (ui->textAIAdvice) {
        ui->textAIAdvice->setHtml(QString("<p style='color:#c96b7e;'>⚠️ 无法获取 AI 建议：%1</p>"
                                              "<p style='color:#7d6b7a;'>请检查网络连接或稍后重试。</p>")
                                          .arg(message));
    }
}

void HealthStatsDialog::enableRefreshButton(bool enable)
{
    if (ui->btnRefreshAI) {
        ui->btnRefreshAI->setEnabled(enable);
    }
}

void HealthStatsDialog::loadStatistics()
{
    QSqlDatabase db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);

    // 计算平均经期持续天数
    double avgDuration = 5.0;
    query.exec("SELECT AVG(duration) FROM period_history WHERE duration IS NOT NULL");
    if (query.next() && !query.value(0).isNull()) {
        avgDuration = query.value(0).toDouble();
    }

    // 计算平均周期长度
    double avgCycle = 28.0;

    // 获取所有日期
    query.exec("SELECT start_date FROM period_history ORDER BY start_date ASC");
    QVector<QDate> dates;
    while (query.next()) {
        QDate date = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
        if (date.isValid()) {
            dates.append(date);
        }
    }

    // 计算周期长度
    if (dates.size() >= 2) {
        QVector<int> cycles;
        for (int i = 1; i < dates.size(); i++) {
            int cycle = dates[i-1].daysTo(dates[i]);
            if (cycle > 0 && cycle < 100) {
                cycles.append(cycle);
            }
        }

        if (!cycles.isEmpty()) {
            double sum = 0;
            for (int cycle : cycles) {
                sum += cycle;
            }
            avgCycle = sum / cycles.size();
        }
    }

    QString summary = QString("📊 平均周期：%1 天     📊 平均经期持续：%2 天")
                          .arg(avgCycle, 0, 'f', 1).arg(avgDuration, 0, 'f', 1);
    if (ui->labelStatsSummary) {
        ui->labelStatsSummary->setText(summary);
    }

    // 创建折线图
    QChartView *chartView = new QChartView(createCycleChart());
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartView->setMinimumHeight(250);

    // 清除旧图表
    if (ui->chartLayout) {
        QLayoutItem *item;
        while ((item = ui->chartLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        ui->chartLayout->addWidget(chartView);
    }

    // 创建情绪图表
    QChartView *moodChartView = new QChartView(createMoodChart());
    moodChartView->setRenderHint(QPainter::Antialiasing);
    moodChartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    moodChartView->setMinimumHeight(250);

    // 清除旧图表
    if (ui->moodChartLayout) {
        QLayoutItem *item;
        while ((item = ui->moodChartLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        ui->moodChartLayout->addWidget(moodChartView);
    }
}

QChart* HealthStatsDialog::createCycleChart()
{
    QLineSeries *series = new QLineSeries();
    series->setName("周期长度（天）");

    QSqlDatabase db = DatabaseManager::instance().getDatabase();

    // 🔥 方法：先获取所有按日期排序的记录，然后在代码中计算周期
    QSqlQuery query(db);
    query.exec("SELECT start_date FROM period_history ORDER BY start_date ASC");

    QVector<QDate> dates;
    while (query.next()) {
        QDate date = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
        if (date.isValid()) {
            dates.append(date);
        }
    }

    // 计算相邻日期的差值（周期长度）
    QVector<int> cycles;
    QStringList categories;

    for (int i = 1; i < dates.size(); i++) {
        int cycleLen = dates[i-1].daysTo(dates[i]);
        if (cycleLen > 0 && cycleLen < 100) {  // 合理范围
            cycles.append(cycleLen);
            categories << dates[i].toString("MM-dd");  // 使用后一个日期作为标签
        }
    }

    // 只取最近6个周期
    int startIdx = qMax(0, cycles.size() - 6);
    for (int i = startIdx; i < cycles.size(); i++) {
        series->append(i - startIdx, cycles[i]);
        qDebug() << "图表数据点:" << categories[i] << "->" << cycles[i] << "天";
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("近期周期波动");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setTheme(QChart::ChartThemeLight);
    chart->setBackgroundVisible(false);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    // Y轴
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("天数");
    axisY->setLabelFormat("%d");

    // 设置Y轴范围
    if (!cycles.isEmpty()) {
        int minCycle = *std::min_element(cycles.begin(), cycles.end());
        int maxCycle = *std::max_element(cycles.begin(), cycles.end());
        axisY->setRange(qMax(15, minCycle - 5), qMin(50, maxCycle + 5));
    } else {
        axisY->setRange(20, 40);
    }
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // X轴
    if (!categories.isEmpty()) {
        QBarCategoryAxis *axisX = new QBarCategoryAxis();

        // 只取最近6个标签
        int catStartIdx = qMax(0, categories.size() - 6);
        QStringList displayCategories;
        for (int i = catStartIdx; i < categories.size(); i++) {
            displayCategories << categories[i];
        }
        axisX->append(displayCategories);

        axisX->setTitleText("经期开始日期");
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
    }

    return chart;
}

// v5.0新增
QChart* HealthStatsDialog::createMoodChart()
{
    QLineSeries *series = new QLineSeries();
    series->setName("心情指数");

    QSqlDatabase db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);
    query.exec("SELECT date, mood_level FROM mood_history ORDER BY date ASC");

    QVector<QDate> dates;
    QVector<int> levels;
    while (query.next()) {
        QDate date = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
        int level = query.value(1).toInt();
        if (date.isValid() && level >= 1 && level <= 5) {
            dates.append(date);
            levels.append(level);
        }
    }

    // 只取最近14条
    int startIdx = qMax(0, dates.size() - 14);
    QStringList categories;
    for (int i = startIdx; i < dates.size(); i++) {
        series->append(i - startIdx, levels[i]);
        categories << dates[i].toString("MM-dd");
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("近期情绪趋势");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setTheme(QChart::ChartThemeLight);
    chart->setBackgroundVisible(false);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    // Y轴：1-5
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("心情等级");
    axisY->setRange(0, 6);
    axisY->setLabelFormat("%d");
    axisY->setTickCount(7);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // X轴
    if (!categories.isEmpty()) {
        QBarCategoryAxis *axisX = new QBarCategoryAxis();
        axisX->append(categories);
        axisX->setTitleText("日期");
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
    }

    return chart;
}

void HealthStatsDialog::on_btnRefreshAI_clicked()
{
    emit refreshAIRequested();
}

void HealthStatsDialog::on_btnClose_clicked()
{
    accept();
}