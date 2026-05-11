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

    int getMoodLevel() const;
    QString getDiaryText() const;

private slots:
    void on_sliderMood_valueChanged(int value);

private:
    Ui::MoodDialog *ui;
};

#endif // MOODDIALOG_H