#include "historymanagerdialog.h"
#include "ui_historymanagerdialog.h"
#include "databasemanager.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QDateEdit>
#include <QSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

HistoryManagerDialog::HistoryManagerDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::HistoryManagerDialog)
{
    ui->setupUi(this);
    setWindowTitle("经期历史管理");
    setMinimumSize(550, 480);

    healthCalc = new HealthCalculator(this);

    // 设置表格列
    ui->tableHistory->setColumnCount(2);
    ui->tableHistory->setHorizontalHeaderLabels({"开始日期", "持续天数"});
    ui->tableHistory->setColumnWidth(0, 200);
    ui->tableHistory->setColumnWidth(1, 150);
    ui->tableHistory->horizontalHeader()->setStretchLastSection(true);
    ui->tableHistory->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableHistory->setEditTriggers(QAbstractItemView::NoEditTriggers);

    refreshTable();
}

HistoryManagerDialog::~HistoryManagerDialog()
{
    delete ui;
}

void HistoryManagerDialog::refreshTable()
{
    ui->tableHistory->setRowCount(0);

    QSqlQuery query("SELECT start_date, duration FROM period_history ORDER BY start_date DESC");
    int row = 0;
    while (query.next()) {
        ui->tableHistory->insertRow(row);

        QTableWidgetItem *dateItem = new QTableWidgetItem(query.value(0).toString());
        QTableWidgetItem *durationItem = new QTableWidgetItem(query.value(1).toString() + " 天");

        ui->tableHistory->setItem(row, 0, dateItem);
        ui->tableHistory->setItem(row, 1, durationItem);

        row++;
    }

    qDebug() << "refreshTable: 加载了" << row << "条记录";
}

void HistoryManagerDialog::on_btnAdd_clicked()
{
    QDialog dialog(this);
    dialog.setWindowTitle("添加经期记录");
    dialog.setMinimumWidth(350);

    QFormLayout form(&dialog);

    QDateEdit *dateEdit = new QDateEdit(&dialog);
    dateEdit->setDate(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    form.addRow("开始日期:", dateEdit);

    QSpinBox *spinDuration = new QSpinBox(&dialog);
    spinDuration->setRange(2, 10);
    spinDuration->setValue(5);
    form.addRow("持续天数:", spinDuration);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QDate start = dateEdit->date();
        int duration = spinDuration->value();

        QSqlQuery query(DatabaseManager::instance().getDatabase());
        query.prepare("INSERT OR REPLACE INTO period_history (start_date, duration, end_date) VALUES (?, ?, ?)");
        query.bindValue(0, start.toString("yyyy-MM-dd"));
        query.bindValue(1, duration);
        query.bindValue(2, start.addDays(duration - 1).toString("yyyy-MM-dd"));

        if (query.exec()) {
            refreshTable();

            // 更新 cycle_data 中的最新经期
            CycleData current = healthCalc->getLatestCycleData();
            if (!current.lastPeriodStart.isValid() || start > current.lastPeriodStart) {
                current.lastPeriodStart = start;
                current.periodLength = duration;
                healthCalc->saveCycleData(current);
                qDebug() << "更新 cycle_data: lastPeriodStart =" << start.toString("yyyy-MM-dd");
            }

            emit dataChanged();
            QMessageBox::information(this, "成功", "记录已添加！");
        } else {
            QMessageBox::warning(this, "错误", "保存失败：" + query.lastError().text());
            qDebug() << "SQL错误:" << query.lastError().text();
        }
    }
}

void HistoryManagerDialog::on_btnDelete_clicked()
{
    int row = ui->tableHistory->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "提示", "请先选中要删除的记录");
        return;
    }

    QString dateStr = ui->tableHistory->item(row, 0)->text();

    int ret = QMessageBox::question(this, "确认删除",
                                    QString("确定删除 %1 的记录吗？").arg(dateStr),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        QSqlQuery query(DatabaseManager::instance().getDatabase());
        query.prepare("DELETE FROM period_history WHERE start_date = ?");
        query.bindValue(0, dateStr);

        if (query.exec()) {
            refreshTable();
            emit dataChanged();
            QMessageBox::information(this, "成功", "记录已删除！");
        } else {
            QMessageBox::warning(this, "错误", "删除失败：" + query.lastError().text());
        }
    }
}

void HistoryManagerDialog::on_btnClose_clicked()
{
    accept();
}