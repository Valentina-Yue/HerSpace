#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QMutex>

class DatabaseManager
{
public:
    static DatabaseManager& instance();

    QSqlDatabase getDatabase();
    bool isOpen() const;

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase db;
    static QMutex mutex;
};

#endif // DATABASEMANAGER_H