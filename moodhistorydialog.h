#ifndef MOODHISTORYDIALOG_H
#define MOODHISTORYDIALOG_H

#include <QDialog>

namespace Ui {
class MoodHistoryDialog;
}

class MoodHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MoodHistoryDialog(QWidget *parent = nullptr);
    ~MoodHistoryDialog();

signals:
    void dataChanged();  // 数据变化信号

private slots:
    void on_btnEdit_clicked();
    void on_btnDelete_clicked();
    void on_btnClose_clicked();
    void on_tableMoodHistory_doubleClicked(const QModelIndex &index);

private:
    Ui::MoodHistoryDialog *ui;
    void refreshTable();
    QString getMoodEmoji(int level);
};

#endif // MOODHISTORYDIALOG_H