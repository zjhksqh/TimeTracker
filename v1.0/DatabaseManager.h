#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QMap>
#include <QString>

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager& instance();

    bool initialize();
    void addUsage(const QString &date, const QString &processName, int seconds);
    QMap<QString, int> getUsageByDate(const QString &date);
    int getTotalUptimeByDate(const QString &date);
    void exportCSV(const QString &filePath);

private:
    DatabaseManager(QObject *parent = nullptr);
    QSqlDatabase m_db;
};

#endif
