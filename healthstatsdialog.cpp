#include "healthstatsdialog.h"
#include "ui_healthstatsdialog.h"
#include "databasemanager.h"
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QSqlQuery>
#include <QDebug>
#include <algorithm>

HealthStatsDialog::HealthStatsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::HealthStatsDialog)
{
    ui->setupUi(this);
    setWindowTitle("健康统计");
    setMinimumSize(650, 550);

    healthCalc = new HealthCalculator(this);

    // 🔥 每次都重新加载
    loadStatistics();

    connect(ui->btnClose, &QPushButton::clicked, this, &QDialog::accept);
}

HealthStatsDialog::~HealthStatsDialog()
{
    delete ui;
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
    ui->labelStatsSummary->setText(summary);

    // 创建折线图
    QChartView *chartView = new QChartView(createCycleChart());
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartView->setMinimumHeight(250);

    // 清除旧图表
    QLayoutItem *item;
    while ((item = ui->chartLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    ui->chartLayout->addWidget(chartView);
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