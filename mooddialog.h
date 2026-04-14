#ifndef MOODDIALOG_H
#define MOODDIALOG_H

#include <QDialog>

namespace Ui {
class MoodDialog;
}

class MoodDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MoodDialog(QWidget *parent = nullptr);
    ~MoodDialog();

    // 获取心情等级 (1-5)
    int getMoodLevel() const;
    // 获取日记文本
    QString getDiaryText() const;

private slots:
    void on_sliderMood_valueChanged(int value);
    void on_btnSave_clicked();
    void on_btnCancel_clicked();

private:
    Ui::MoodDialog *ui;
};

#endif // MOODDIALOG_H