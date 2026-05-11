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

         // 创建周期数据 cycle_data 表（存储用户的基本设置和平均值）
        if (!query.exec("CREATE TABLE IF NOT EXISTS cycle_data ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "last_period_start TEXT, "
                        "cycle_length INTEGER, "
                        "period_length INTEGER)")) {
            qDebug() << "创建 cycle_data 表失败:" << query.lastError().text();
        }

        // 创建实际经期历史记录 period_history 表：
        if (!query.exec("CREATE TABLE IF NOT EXISTS period_history ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "start_date TEXT UNIQUE, "
                        "end_date TEXT, "
                        "duration INTEGER)")) {   // 没有 NOT NULL 约束
            qDebug() << "创建 period_history 表失败:" << query.lastError().text();
        }

        // 创建情绪记录表
        if (!query.exec("CREATE TABLE IF NOT EXISTS mood_history ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "date TEXT UNIQUE, "
                        "mood_level INTEGER, "
                        "diary_text TEXT)")) {
            qDebug() << "创建 mood_history 表失败:" << query.lastError().text();
        }

        // 🔥 创建经期确认询问记录表（防止重复询问）
        if (!query.exec("CREATE TABLE IF NOT EXISTS period_confirm_asked ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "date TEXT UNIQUE)")) {
            qDebug() << "创建 period_confirm_asked 表失败:" << query.lastError().text();
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