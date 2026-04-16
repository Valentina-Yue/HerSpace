#include "databasemanager.h"
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>

QMutex DatabaseManager::mutex;

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager()
{
    db = QSqlDatabase::addDatabase("QSQLITE", "HerSpaceConnection");
    db.setDatabaseName("her_space.db");

    if (!db.open()) {
        qDebug() << "DatabaseManager: 无法打开数据库:" << db.lastError().text();
    } else {
        qDebug() << "DatabaseManager: 数据库连接成功";

        // 创建表
        QSqlQuery query(db);

        if (!query.exec("CREATE TABLE IF NOT EXISTS cycle_data ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "last_period_start TEXT, "
                        "cycle_length INTEGER, "
                        "period_length INTEGER)")) {
            qDebug() << "创建 cycle_data 表失败:" << query.lastError().text();
        }

        if (!query.exec("CREATE TABLE IF NOT EXISTS period_history ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "start_date TEXT UNIQUE, "
                        "end_date TEXT, "
                        "duration INTEGER)")) {
            qDebug() << "创建 period_history 表失败:" << query.lastError().text();
        }
    }
}

DatabaseManager::~DatabaseManager()
{
    if (db.isOpen()) {
        db.close();
    }
}

QSqlDatabase DatabaseManager::getDatabase()
{
    return db;
}

bool DatabaseManager::isOpen() const
{
    return db.isOpen();
}