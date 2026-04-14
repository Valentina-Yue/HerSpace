#include "mooddialog.h"
#include "ui_mooddialog.h"

MoodDialog::MoodDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MoodDialog)
{
    ui->setupUi(this);
    ui->labelMoodValue->setText(QString::number(ui->sliderMood->value()));
}

MoodDialog::~MoodDialog()
{
    delete ui;
}

int MoodDialog::getMoodLevel() const
{
    return ui->sliderMood->value();
}

QString MoodDialog::getDiaryText() const
{
    return ui->textEditDiary->toPlainText();
}

void MoodDialog::on_sliderMood_valueChanged(int value)
{
    ui->labelMoodValue->setText(QString::number(value));
}

void MoodDialog::on_btnSave_clicked()
{
    accept();
}

void MoodDialog::on_btnCancel_clicked()
{
    reject();
}