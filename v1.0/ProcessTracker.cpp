#include "ProcessTracker.h"
#include "DatabaseManager.h"
#include <QDebug>
#include <tlhelp32.h>
#include <Psapi.h>

ProcessTracker::ProcessTracker(QObject *parent)
    : QObject(parent),
    m_scanTimer(new QTimer(this)),
    m_persistTimer(new QTimer(this))
{
    connect(m_scanTimer, &QTimer::timeout, this, &ProcessTracker::onScanTimer);
    connect(m_persistTimer, &QTimer::timeout, this, &ProcessTracker::onPersistTimer);
}

void ProcessTracker::start(int scanIntervalMs, int persistIntervalMs)
{
    m_scanTimer->start(scanIntervalMs);
    m_persistTimer->start(persistIntervalMs);
}

void ProcessTracker::stop()
{
    m_scanTimer->stop();
    m_persistTimer->stop();
    // 程序退出前，把所有还在跑的进程的最后一段生命写进数据库
    persistRunningProcesses(true);
}

QHash<DWORD, RunningProcess> ProcessTracker::currentProcesses() const
{
    return m_processes;
}

// ========== 辅助：把一段时间按天切分，写入数据库 ==========
void ProcessTracker::writeTimeSlices(const QString &processName,
                                     const QDateTime &start, const QDateTime &end)
{
    if (start >= end) return;

    QDateTime segStart = start;
    QDateTime segEnd = end;

    while (segStart < segEnd) {
        QDate dateOfSeg = segStart.date();
        // 当天23:59:59
        QDateTime endOfDay(dateOfSeg, QTime(23, 59, 59));
        QDateTime currentSegEnd = qMin(endOfDay, segEnd);
        qint64 segSecs = segStart.secsTo(currentSegEnd);
        if (segSecs > 0) {
            DatabaseManager::instance().addUsage(
                dateOfSeg.toString("yyyy-MM-dd"),
                processName,
                static_cast<int>(segSecs));
        }
        segStart = currentSegEnd.addSecs(1);
    }
}

// ========== 获取当前所有进程快照 ==========
QHash<DWORD, QString> ProcessTracker::getCurrentProcessSnapshot()
{
    QHash<DWORD, QString> result;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return result;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            QString name = QString::fromWCharArray(pe.szExeFile);

            // 忽略系统空闲进程
            if (name.compare("System Idle Process", Qt::CaseInsensitive) == 0 ||
                name.compare("System", Qt::CaseInsensitive) == 0) {
                continue;
            }

            result[pe.th32ProcessID] = name;
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return result;
}

// ========== 获取进程真实创建时间 ==========
QDateTime ProcessTracker::getProcessCreationTime(DWORD pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess) {
        FILETIME createTime, exitTime, kernelTime, userTime;
        if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
            CloseHandle(hProcess);
            ULARGE_INTEGER ull;
            ull.LowPart = createTime.dwLowDateTime;
            ull.HighPart = createTime.dwHighDateTime;
            qint64 windowsTicks = ull.QuadPart;
            // 转换为Unix时间戳（毫秒）
            qint64 unixTimeMs = windowsTicks / 10000 - 11644473600000LL;
            return QDateTime::fromMSecsSinceEpoch(unixTimeMs);
        }
        CloseHandle(hProcess);
    }
    // 如果获取失败，返回一个无效的时间
    return QDateTime();
}

// ========== 定时扫描：发现新进程 / 移除退出进程 ==========
void ProcessTracker::onScanTimer()
{
    QHash<DWORD, QString> newSnap = getCurrentProcessSnapshot();
    QDateTime now = QDateTime::currentDateTime();

    // 1. 处理已退出的进程
    for (auto it = m_processes.begin(); it != m_processes.end(); ) {
        if (!newSnap.contains(it.key())) {
            // 进程退出，结算最后一段未记录的生命
            finalizeProcess(it.key());
            it = m_processes.erase(it);
        } else {
            ++it;
        }
    }

    // 2. 处理新出现的进程
    for (auto it = newSnap.constBegin(); it != newSnap.constEnd(); ++it) {
        DWORD pid = it.key();
        if (!m_processes.contains(pid)) {
            RunningProcess rp;
            rp.pid = pid;
            rp.processName = it.value();
            rp.startTime = getProcessCreationTime(pid);
            // 如果获取不到创建时间（罕见情况），用当前时间
            if (!rp.startTime.isValid()) {
                rp.startTime = now;
            }
            rp.persistedSeconds = 0;

            // 【关键修复】首次发现进程，立刻把“从它出生到现在”的完整历史写进数据库
            // 这样即使数据库被删，重新打开也能立刻看到正确的历史总时长
            qint64 totalElapsed = rp.startTime.secsTo(now);
            if (totalElapsed > 0) {
                writeTimeSlices(rp.processName, rp.startTime, now);
                rp.persistedSeconds = totalElapsed;
            }

            m_processes[pid] = rp;
        }
    }

    emit processDataChanged();
}

// ========== 持久化定时器 ==========
void ProcessTracker::onPersistTimer()
{
    // 定期把运行中进程的新增长时间写盘，防止崩溃丢失
    persistRunningProcesses(false);
    emit processDataChanged();
}

// ========== 持久化所有正在运行的进程（只写新增部分） ==========
void ProcessTracker::persistRunningProcesses(bool isFinal)
{
    QDateTime now = QDateTime::currentDateTime();
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        RunningProcess &rp = it.value();

        QDateTime realStart = getProcessCreationTime(rp.pid);
        // 如果获取失败，退回到我们记录的开始时间
        if (!realStart.isValid()) realStart = rp.startTime;

        qint64 totalElapsed = realStart.secsTo(now);
        if (totalElapsed <= 0) continue;

        // 只写入“新增”部分
        qint64 newSeconds = totalElapsed - rp.persistedSeconds;
        if (newSeconds <= 0) continue;

        // 新增部分的时间段：从 (now - newSeconds) 到 now
        QDateTime segStart = now.addSecs(-newSeconds);
        writeTimeSlices(rp.processName, segStart, now);

        // 更新已持久化标记
        rp.persistedSeconds = totalElapsed;
    }
}

// ========== 进程退出时写入最后一段未保存的时长 ==========
void ProcessTracker::finalizeProcess(DWORD pid)
{
    if (!m_processes.contains(pid)) return;
    RunningProcess rp = m_processes[pid];
    QDateTime now = QDateTime::currentDateTime();

    QDateTime realStart = getProcessCreationTime(rp.pid);
    if (!realStart.isValid()) realStart = rp.startTime;

    qint64 totalSeconds = realStart.secsTo(now);
    if (totalSeconds <= 0) return;

    // 最后一段未持久化的时长
    qint64 remaining = totalSeconds - rp.persistedSeconds;
    if (remaining <= 0) return;

    QDateTime segStart = now.addSecs(-remaining);
    writeTimeSlices(rp.processName, segStart, now);
}
