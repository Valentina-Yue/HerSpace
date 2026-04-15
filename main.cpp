#include "widget.h"

#include <QApplication>
#include <QFontDatabase>
#include <QFile>
#include <QDebug>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 加载资源文件中的字体
    QFontDatabase::addApplicationFont(":/fonts/Quicksand-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Quicksand-Bold.ttf");

    // 设置默认字体
    QFont defaultFont("Quicksand", 11);
    a.setFont(defaultFont);

    // 加载样式表
    QFile styleFile(":/styles/style.qss"); // 稍后加入资源
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleFile.readAll());
        a.setStyleSheet(style);
        qDebug() << "样式表加载成功";
    } else {
        qDebug() << "无法加载样式表，请检查资源文件";
    }

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "HerSpace_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    Widget w;
    w.show();
    return QCoreApplication::exec();
}
