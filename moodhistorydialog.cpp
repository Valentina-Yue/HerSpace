#include "moodhistorydialog.h"
#include "ui_moodhistorydialog.h"
#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QInputDialog>
#include <QFormLayout>
#include <QSlider>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QDebug>

MoodHistoryDialog::MoodHistoryDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MoodHistoryDialog)
{
    ui->setupUi(this);
    setWindowTitle("情绪历史管理");
    setMinimumSize(600, 500);

    // 设置表格列
    ui->tableMoodHistory->setColumnCount(3);
    ui->tableMoodHistory->setHorizontalHeaderLabels({"日期", "心情", "日记"});
    ui->tableMoodHistory->setColumnWidth(0, 150);
    ui->tableMoodHistory->setColumnWidth(1, 120);
    ui->tableMoodHistory->setColumnWidth(2, 250);
    ui->tableMoodHistory->horizontalHeader()->setStretchLastSection(true);
    ui->tableMoodHistory->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableMoodHistory->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 连接双击信号
    connect(ui->tableMoodHistory, &QTableWidget::doubleClicked,
            this, &MoodHistoryDialog::on_tableMoodHistory_doubleClicked);

    refreshTable();
}

MoodHistoryDialog::~MoodHistoryDialog()
{
    delete ui;
}

QString MoodHistoryDialog::getMoodEmoji(int level)
{
    switch(level) {
    case 1: return "😢 很差";
    case 2: return "😕 不太好";
    case 3: return "😐 一般";
    case 4: return "🙂 不错";
    case 5: return "😊 很棒";
    default: return "😐 未知";
    }
}

void MoodHistoryDialog::refreshTable()
{
    ui->tableMoodHistory->setRowCount(0);

    QSqlDatabase db = DatabaseManager::instance().getDatabase();
    QSqlQuery query("SELECT date, mood_level, diary_text FROM mood_history ORDER BY date DESC", db);

    int row = 0;
    while (query.next()) {
        ui->tableMoodHistory->insertRow(row);

        QString dateStr = query.value(0).toString();
        int level = query.value(1).toInt();
        QString diary = query.value(2).toString();

        ui->tableMoodHistory->setItem(row, 0, new QTableWidgetItem(dateStr));
        ui->tableMoodHistory->setItem(row, 1, new QTableWidgetItem(getMoodEmoji(level)));
        ui->tableMoodHistory->setItem(row, 2, new QTableWidgetItem(diary));

        row++;
    }

    qDebug() << "refreshTable: 加载了" << row << "条情绪记录";
}

void MoodHistoryDialog::on_btnEdit_clicked()
{
    int row = ui->tableMoodHistory->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "提示", "请先选中要编辑的记录");
        return;
    }

    // 获取当前记录的数据
    QString dateStr = ui->tableMoodHistory->item(row, 0)->text();
    QString moodStr = ui->tableMoodHistory->item(row, 1)->text();
    QString diary = ui->tableMoodHistory->item(row, 2)->text();

    // 从显示文本中提取等级
    int currentLevel = 3;
    if (moodStr.contains("很差")) currentLevel = 1;
    else if (moodStr.contains("不太好")) currentLevel = 2;
    else if (moodStr.contains("一般")) currentLevel = 3;
    else if (moodStr.contains("不错")) currentLevel = 4;
    else if (moodStr.contains("很棒")) currentLevel = 5;

    // 弹出编辑对话框
    QDialog dialog(this);
    dialog.setWindowTitle("编辑情绪记录");
    dialog.setMinimumWidth(450);

    QFormLayout form(&dialog);

    // 日期显示（不可编辑）
    QLabel *dateLabel = new QLabel(dateStr, &dialog);
    dateLabel->setStyleSheet("font-weight: bold; color: #c96b7e;");
    form.addRow("日期:", dateLabel);

    // 心情滑块
    QSlider *sliderMood = new QSlider(Qt::Horizontal, &dialog);
    sliderMood->setRange(1, 5);
    sliderMood->setValue(currentLevel);
    sliderMood->setTickPosition(QSlider::TicksBelow);
    sliderMood->setTickInterval(1);
    form.addRow("心情等级:", sliderMood);

    // 显示当前值
    QLabel *labelMoodValue = new QLabel(getMoodEmoji(currentLevel), &dialog);
    labelMoodValue->setAlignment(Qt::AlignCenter);
    labelMoodValue->setStyleSheet("font-size: 14pt; font-weight: bold;");
    form.addRow("", labelMoodValue);

    QObject::connect(sliderMood, &QSlider::valueChanged, [labelMoodValue, this](int value) {
        labelMoodValue->setText(getMoodEmoji(value));
    });

    // 日记编辑
    QTextEdit *textEditDiary = new QTextEdit(&dialog);
    textEditDiary->setText(diary);
    textEditDiary->setPlaceholderText("写点什么吧...");
    textEditDiary->setMaximumHeight(150);
    form.addRow("心情日记:", textEditDiary);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        int newLevel = sliderMood->value();
        QString newDiary = textEditDiary->toPlainText();

        QSqlDatabase db = DatabaseManager::instance().getDatabase();
        QSqlQuery query(db);
        query.prepare("UPDATE mood_history SET mood_level = ?, diary_text = ? WHERE date = ?");
        query.bindValue(0, newLevel);
        query.bindValue(1, newDiary);
        query.bindValue(2, dateStr);

        if (query.exec()) {
            refreshTable();
            emit dataChanged();
            QMessageBox::information(this, "成功", "情绪记录已更新！");
        } else {
            QMessageBox::warning(this, "错误", "更新失败：" + query.lastError().text());
        }
    }
}

void MoodHistoryDialog::on_btnDelete_clicked()
{
    int row = ui->tableMoodHistory->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "提示", "请先选中要删除的记录");
        return;
    }

    QString dateStr = ui->tableMoodHistory->item(row, 0)->text();

    int ret = QMessageBox::question(this, "确认删除",
                                    QString("确定删除 %1 的情绪记录吗？").arg(dateStr),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        QSqlDatabase db = DatabaseManager::instance().getDatabase();
        QSqlQuery query(db);
        query.prepare("DELETE FROM mood_history WHERE date = ?");
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

void MoodHistoryDialog::on_btnClose_clicked()
{
    accept();
}

void MoodHistoryDialog::on_tableMoodHistory_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    on_btnEdit_clicked();
}