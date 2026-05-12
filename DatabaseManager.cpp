#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFile>
#include <QTextStream>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("timetracker.db");
}

bool DatabaseManager::initialize()
{
    if (!m_db.open()) {
        qCritical() << "无法打开数据库:" << m_db.lastError().text();
        return false;
    }
    QSqlQuery query(m_db);
    bool ok = query.exec(
        "CREATE TABLE IF NOT EXISTS daily_usage ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  date TEXT NOT NULL,"
        "  process_name TEXT NOT NULL,"
        "  total_seconds INTEGER DEFAULT 0"
        ");"
        );
    if (!ok) {
        qCritical() << "建表失败:" << query.lastError().text();
        return false;
    }
    query.exec("CREATE UNIQUE INDEX IF NOT EXISTS idx_date_process "
               "ON daily_usage(date, process_name);");
    return true;
}

void DatabaseManager::addUsage(const QString &date, const QString &processName, int seconds)
{
    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO daily_usage (date, process_name, total_seconds) "
        "VALUES (:date, :name, :seconds) "
        "ON CONFLICT(date, process_name) DO UPDATE SET "
        "total_seconds = total_seconds + :seconds"
        );
    query.bindValue(":date", date);
    query.bindValue(":name", processName);
    query.bindValue(":seconds", seconds);
    if (!query.exec())
        qWarning() << "写入使用记录失败:" << query.lastError().text();
}

QMap<QString, int> DatabaseManager::getUsageByDate(const QString &date)
{
    QMap<QString, int> map;
    QSqlQuery query(m_db);
    query.prepare("SELECT process_name, total_seconds FROM daily_usage WHERE date = :date");
    query.bindValue(":date", date);
    if (query.exec()) {
        while (query.next()) {
            map[query.value(0).toString()] = query.value(1).toInt();
        }
    }
    return map;
}

int DatabaseManager::getTotalUptimeByDate(const QString &date)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT SUM(total_seconds) FROM daily_usage WHERE date = :date");
    query.bindValue(":date", date);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

void DatabaseManager::exportCSV(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "无法写入文件:" << filePath;
        return;
    }
    QTextStream out(&file);
    out << "date,process_name,total_seconds\n";
    QSqlQuery query(m_db);
    query.exec("SELECT date, process_name, total_seconds FROM daily_usage "
               "ORDER BY date, total_seconds DESC");
    while (query.next()) {
        out << query.value(0).toString() << ","
            << query.value(1).toString() << ","
            << query.value(2).toInt() << "\n";
    }
    file.close();
}

