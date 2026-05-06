#include "DatabaseManager.hpp"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

DatabaseManager::DatabaseManager(const QString& connectionName) : m_connectionName(connectionName){
    // Nothing to initialize yet
}

DatabaseManager::~DatabaseManager() {
    if (QSqlDatabase::contains(m_connectionName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(m_connectionName);
            if (db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool DatabaseManager::connect(const QString& host, const QString& dbName,
                              const QString& user, const QString& password) {
    if (QSqlDatabase::contains(m_connectionName)) {
        qWarning() << "DatabaseManager: connection" << m_connectionName << "already exists.";
            return isConnected();
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", m_connectionName);
    db.setHostName(host);
    db.setDatabaseName(dbName);
    db.setUserName(user);
    db.setPassword(password);
    db.setPort(5432);

    if (!db.open()) {
        qCritical() << "Database Manager failed to connect to" << dbName << "on" << host;
        qCritical() << "Error:" << db.lastError().text();
        return false;
    }

    qDebug() << "DatabaseManager connected to" << dbName << "on" << host << "(" << m_connectionName << ")";

    return true;
}

bool DatabaseManager::isConnected() const {
    if (!QSqlDatabase::contains(m_connectionName))
        return false;

    return QSqlDatabase::database(m_connectionName).isOpen();
}

QSqlDatabase DatabaseManager::database() const {
    return QSqlDatabase::database(m_connectionName);
}

bool DatabaseManager::runMigrations() {
    if (!isConnected()) {
        qCritical() << "DatabaseManager: cannot run migrations, not connected.";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);

    // Create the migrations tracking table if it doesn't exist yet.
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS schema_migrations (
            version     INT PRIMARY KEY,
            filename    TEXT NOT NULL,
            applied_at  TIMESTAMPTZ DEFAULT now()
        )
    )")) {
        qCritical() << "DatabaseManager: failed to create schema_migrations table.";
        qCritical() << q.lastError().text();
        return false;
    }

    // Load which migrations have already been applied
    QSet<int> applied;
    if (!q.exec("SELECT version FROM schema_migrations")) {
        qCritical() << "DatabaseManager: failed to query schema_migrations.";
        return false;
    }
    while (q.next())
        applied.insert(q.value(0).toInt());

    // Find migration files bundled in Qt resources under :/migrations/
    QDir migrationsDir("db/migrations");
    const QStringList files = migrationsDir.entryList(
        QStringList() << "*.sql",
        QDir::Files,
        QDir::Name   // sorted alphabetically = sorted numerically by prefix
        );

    if (files.isEmpty()) {
        qWarning() << "DatabaseManager: no migration files found in :/migrations";
        return true;  // not a hard failure — just nothing to do
    }

    for (const QString& filename : files) {
        // Extract version number from filename prefix e.g. "001_initial_schema.sql" -> 1
        int version = filename.split("_").first().toInt();

        if (version == 0) {
            qWarning() << "DatabaseManager: could not parse version from filename:"
                       << filename << "— skipping.";
            continue;
        }

        if (applied.contains(version)) {
            qDebug() << "DatabaseManager: migration" << filename << "already applied, skipping.";
            continue;
        }

        qDebug() << "DatabaseManager: applying migration" << filename;

        // Wrap each migration in a transaction so a half-applied
        // migration doesn't leave the schema in a broken state
        db.transaction();

        if (!runSqlFile(migrationsDir.filePath(filename), db)) {
            db.rollback();
            qCritical() << "DatabaseManager: migration" << filename
                        << "failed — rolling back.";
            return false;
        }

        // Record this migration as applied
        q.prepare(
            "INSERT INTO schema_migrations (version, filename) VALUES (:version, :filename)"
            );
        q.bindValue(":version",  version);
        q.bindValue(":filename", filename);

        if (!q.exec()) {
            db.rollback();
            qCritical() << "DatabaseManager: failed to record migration"
                        << filename << "— rolling back.";
            return false;
        }

        db.commit();
        qDebug() << "DatabaseManager: migration" << filename << "applied successfully.";
    }

    return true;
}

bool DatabaseManager::runSqlFile(const QString& path, QSqlDatabase& db) {
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "DatabaseManager: could not open SQL file:" << path;
        return false;
    }

    QTextStream stream(&file);
    QString fullSql = stream.readAll();
    file.close();

    // Split on semicolons to execute statements individually.
    // Qt's QSqlQuery can only execute one statement at a time.
    const QStringList statements = fullSql.split(';', Qt::SkipEmptyParts);

    QSqlQuery q(db);
    for (QString statement : statements) {
        statement = statement.trimmed();

        // Skip blank entries that result from trailing semicolons
        // or comment-only blocks
        if (statement.isEmpty())
            continue;

        if (!q.exec(statement)) {
            qCritical() << "DatabaseManager: SQL execution failed.";
            qCritical() << "Statement:" << statement;
            qCritical() << "Error:"     << q.lastError().text();
            return false;
        }
    }

    return true;
}
