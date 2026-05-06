#pragma once
#include <QtSql/QSqlDatabase>
#include <QString>

class DatabaseManager {
public:
    // Constructor takes a connection name so multiple instances
    // can coexist without colliding in Qt's connection registry
    explicit DatabaseManager(const QString& connectionName = "main_connection");
    ~DatabaseManager();

    // Non-copyable — each instance owns a unique DB connection
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool connect(const QString& host, const QString& dbName,
                 const QString& user, const QString& password);

    bool runMigrations();
    bool isConnected() const;
    QSqlDatabase database() const;  // exposed so workers can reference it

private:
    QString m_connectionName;
    bool runSqlFile(const QString& path, QSqlDatabase& db);
};