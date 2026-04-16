#include "periodconfirmdialog.h"
#include "ui_periodconfirmdialog.h"
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>

PeriodConfirmDialog::PeriodConfirmDialog(HealthCalculator *hc, const QDate &expectedDate, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PeriodConfirmDialog)
    , healthCalc(hc)
    , m_result(NotYet)
    , m_expectedDate(expectedDate)
{
    ui->setupUi(this);

    ui->labelPrompt->setText(QString("预计经期开始于 %1，您的情况是？")
                                 .arg(expectedDate.toString("yyyy年MM月dd日")));
}

PeriodConfirmDialog::~PeriodConfirmDialog()
{
    delete ui;
}

void PeriodConfirmDialog::on_btnCame_clicked()
{
    m_result = Came;
    accept();
}

void PeriodConfirmDialog::on_btnNotYet_clicked()
{
    m_result = NotYet;
    accept();
}

void PeriodConfirmDialog::on_btnEarly_clicked()
{
    QDialog dateDialog(this);
    dateDialog.setWindowTitle("选择实际开始日期");
    dateDialog.setMinimumWidth(350);

    QVBoxLayout layout(&dateDialog);

    QLabel *label = new QLabel("请选择经期实际开始的日期：", &dateDialog);
    label->setAlignment(Qt::AlignCenter);
    layout.addWidget(label);

    QDateEdit *dateEdit = new QDateEdit(&dateDialog);
    dateEdit->setDate(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    layout.addWidget(dateEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dateDialog);
    layout.addWidget(&buttonBox);

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dateDialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dateDialog, &QDialog::reject);

    if (dateDialog.exec() == QDialog::Accepted) {
        m_earlyDate = dateEdit->date();
        m_result = Early;
        accept();
    }
}