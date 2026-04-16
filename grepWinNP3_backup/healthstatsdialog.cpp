#include "healthstatsdialog.h"
#include "ui_healthstatsdialog.h"
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QSqlQuery>
#include <QDebug>

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
    // 从 period_history 计算平均持续天数
    QSqlQuery query(DatabaseManager::instance().getDatabase());
    query.exec("SELECT AVG(duration) FROM period_history WHERE duration IS NOT NULL");
    double avgDuration = 5.0;
    if (query.next() && !query.value(0).isNull()) {
        avgDuration = query.value(0).toDouble();
    }

    // 计算平均周期（两次经期开始日期的间隔）
    query.exec("SELECT AVG(julianday(p2.start_date) - julianday(p1.start_date)) "
               "FROM period_history p1 "
               "JOIN period_history p2 ON p2.id = p1.id + 1 "
               "WHERE p1.start_date IS NOT NULL AND p2.start_date IS NOT NULL");
    double avgCycle = 28.0;
    if (query.next() && !query.value(0).isNull()) {
        avgCycle = query.value(0).toDouble();
    }

    QString summary = QString("📊 平均周期：%1 天\n📅 平均经期持续：%2 天")
                          .arg(avgCycle, 0, 'f', 1).arg(avgDuration, 0, 'f', 1);
    ui->labelStatsSummary->setText(summary);

    // 创建折线图
    QChartView *chartView = new QChartView(createCycleChart());
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartView->setMinimumHeight(300);   // 确保图表有足够高度
    ui->chartLayout->addWidget(chartView);
}

QChart* HealthStatsDialog::createCycleChart()
{
    QLineSeries *series = new QLineSeries();
    series->setName("周期长度（天）");

    // 获取最近6次周期长度数据
    QSqlQuery query("SELECT start_date, "
                    "(SELECT julianday(p2.start_date) - julianday(p1.start_date) "
                    " FROM period_history p2 WHERE p2.start_date > p1.start_date "
                    " ORDER BY p2.start_date LIMIT 1) as cycle_len "
                    "FROM period_history p1 ORDER BY start_date DESC LIMIT 6");
    QStringList categories;
    int idx = 0;
    while (query.next()) {
        double len = query.value(1).toDouble();
        if (len > 0) {
            series->append(idx, len);
            categories << query.value(0).toDate().toString("MM-dd");
            idx++;
        }
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("近期周期波动");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setTheme(QChart::ChartThemeLight);
    chart->setBackgroundVisible(false);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("天数");
    axisY->setRange(20, 40);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    return chart;
}