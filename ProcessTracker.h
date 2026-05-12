#ifndef PROCESSTRACKER_H
#define PROCESSTRACKER_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QDateTime>
#include <QString>
#include <windows.h>

struct RunningProcess {
    QString processName;
    DWORD pid;
    QDateTime startTime;
    qint64 persistedSeconds;
};

class ProcessTracker : public QObject
{
    Q_OBJECT
public:
    explicit ProcessTracker(QObject *parent = nullptr);
    void start(int scanIntervalMs = 3000, int persistIntervalMs = 60000);
    void stop();

    QHash<DWORD, RunningProcess> currentProcesses() const;

signals:
    void processDataChanged();

private:
    void onScanTimer();
    void onPersistTimer();
    void finalizeProcess(DWORD pid);
    void persistRunningProcesses(bool isFinal = false);           // ← 修复：加上 bool 参数
    void writeTimeSlices(const QString &processName,              // ← 新增：声明辅助函数
                         const QDateTime &start,
                         const QDateTime &end);
    QHash<DWORD, QString> getCurrentProcessSnapshot();
    QDateTime getProcessCreationTime(DWORD pid);

    QHash<DWORD, RunningProcess> m_processes;
    QTimer *m_scanTimer;
    QTimer *m_persistTimer;
};

#endif
