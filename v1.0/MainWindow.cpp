#include "MainWindow.h"
#include "DatabaseManager.h"
#include "SettingsDialog.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QtCharts>
#include <QProcess>
#include <windows.h>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    setupTrayIcon();
    registerGlobalHotkey();

    m_tracker = new ProcessTracker(this);
    m_tracker->start(3000, 60000);

    m_uiRefreshTimer = new QTimer(this);
    connect(m_uiRefreshTimer, &QTimer::timeout, this, &MainWindow::refreshUI);
    m_uiRefreshTimer->start(3000);

    m_selectedDate = QDate::currentDate();
    m_calendar->setSelectedDate(m_selectedDate);
    connect(m_calendar, &QCalendarWidget::clicked, this, &MainWindow::onDateSelected);

    refreshUI();
    setupCharts(m_selectedDate);
}

MainWindow::~MainWindow()
{
    unregisterGlobalHotkey();
    m_tracker->stop();
}

void MainWindow::setupUI()
{
    setWindowTitle("TimeTracker - 南开C++大作业");
    resize(900, 600);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    QHBoxLayout *topLayout = new QHBoxLayout();
    m_totalUptimeLabel = new QLabel("今日开机总时长: --:--:--");
    m_totalUptimeLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    m_runningCountLabel = new QLabel("运行进程: 0");
    topLayout->addWidget(m_totalUptimeLabel);
    topLayout->addStretch();
    topLayout->addWidget(m_runningCountLabel);
    mainLayout->addLayout(topLayout);

    QHBoxLayout *midLayout = new QHBoxLayout();
    m_processTable = new QTableWidget(0, 2);
    m_processTable->setHorizontalHeaderLabels({"进程名", "本次运行时长"});
    m_processTable->horizontalHeader()->setStretchLastSection(true);
    midLayout->addWidget(m_processTable, 1);

    m_chartView = new QChartView();
    m_chartView->setRenderHint(QPainter::Antialiasing);
    midLayout->addWidget(m_chartView, 1);
    mainLayout->addLayout(midLayout);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    m_calendar = new QCalendarWidget();
    m_calendar->setMaximumWidth(300);
    bottomLayout->addWidget(m_calendar);

    QVBoxLayout *btnLayout = new QVBoxLayout();
    QPushButton *exportBtn = new QPushButton("导出 CSV");
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::exportData);
    QPushButton *settingsBtn = new QPushButton("设置");
    connect(settingsBtn, &QPushButton::clicked, this, &MainWindow::openSettings);
    btnLayout->addWidget(exportBtn);
    btnLayout->addWidget(settingsBtn);
    btnLayout->addStretch();
    bottomLayout->addLayout(btnLayout);
    mainLayout->addLayout(bottomLayout);
}

void MainWindow::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(QIcon(":/icons/app.png"), this);
    m_trayMenu = new QMenu(this);
    m_trayMenu->addAction("显示主窗口", this, &MainWindow::showMainWindow);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction("退出", qApp, &QApplication::quit);
    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::DoubleClick)
                    showMainWindow();
            });
    m_trayIcon->show();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::showMainWindow()
{
    show();
    raise();
    activateWindow();
}

void MainWindow::registerGlobalHotkey()
{
    m_hotkeyAtom = GlobalAddAtomW(L"TimeTrackerHideHotkey");
    if (!RegisterHotKey((HWND)winId(), m_hotkeyAtom,
                        MOD_CONTROL | MOD_ALT | MOD_SHIFT, 'H'))
        qWarning() << "注册全局热键失败";
}

void MainWindow::unregisterGlobalHotkey()
{
    if (m_hotkeyAtom) {
        UnregisterHotKey((HWND)winId(), m_hotkeyAtom);
        GlobalDeleteAtom(m_hotkeyAtom);
    }
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_HOTKEY && msg->wParam == m_hotkeyAtom) {
        toggleVisibility();
        return true;
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}

void MainWindow::toggleVisibility()
{
    bool visible = !this->isVisible();
    this->setVisible(visible);
    m_trayIcon->setVisible(visible);
}

void MainWindow::refreshUI()
{
    QHash<DWORD, RunningProcess> procs = m_tracker->currentProcesses();
    m_runningCountLabel->setText(QString("运行进程: %1").arg(procs.size()));

    m_processTable->setRowCount(procs.size());
    int row = 0;
    QDateTime now = QDateTime::currentDateTime();
    for (auto it = procs.cbegin(); it != procs.cend(); ++it, ++row) {
        const RunningProcess &rp = it.value();
        m_processTable->setItem(row, 0, new QTableWidgetItem(rp.processName));
        qint64 runtime = rp.startTime.secsTo(now);
        QTime t = QTime(0, 0).addSecs(runtime);
        m_processTable->setItem(row, 1, new QTableWidgetItem(t.toString("hh:mm:ss")));
    }

    // 今日实际使用时长（剔除关机间隙）
    int totalSecs = getTodaysActualUptimeSeconds();
    QTime totalTime = QTime(0, 0).addSecs(totalSecs);
    m_totalUptimeLabel->setText(QString("今日开机总时长: %1").arg(totalTime.toString("hh:mm:ss")));
}

void MainWindow::onDateSelected(const QDate &date)
{
    m_selectedDate = date;
    setupCharts(date);
}

void MainWindow::setupCharts(const QDate &date)
{
    QString dateStr = date.toString("yyyy-MM-dd");
    QMap<QString, int> usageMap = DatabaseManager::instance().getUsageByDate(dateStr);

    QList<QPair<QString, int>> items;
    for (auto it = usageMap.cbegin(); it != usageMap.cend(); ++it)
        items.append({it.key(), it.value()});
    std::sort(items.begin(), items.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });
    if (items.size() > 5) items = items.mid(0, 5);

    QBarSet *set = new QBarSet("运行时长(小时)");
    QStringList categories;
    for (const auto &item : items) {
        double hours = item.second / 3600.0;
        *set << hours;
        categories << item.first;
    }

    // set->setLabelVisible(true);
    // set->setLabelFormat(QStringLiteral("%.1f h"));

    QBarSeries *series = new QBarSeries();
    series->append(set);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(dateStr + " 使用排行前5");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsAngle(-45);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("小时");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    QChart *oldChart = m_chartView->chart();
    m_chartView->setChart(chart);
    delete oldChart;
}

void MainWindow::openSettings()
{
    SettingsDialog dlg(this);
    dlg.exec();
}

void MainWindow::exportData()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出 CSV",
                                                    "timetracker_export.csv",
                                                    "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;
    DatabaseManager::instance().exportCSV(fileName);
    QMessageBox::information(this, "导出成功", "已导出至:\n" + fileName);
}

// ========== 系统日志相关：获取今天实际使用时长 ==========

QDateTime MainWindow::getTodaysFirstWakeTime()
{
    QProcess process;
    process.start("powershell", QStringList() << "-Command"
                                              << "Get-WinEvent -FilterHashtable @{LogName='System'; ID=1; StartTime='" +
                                                     QDate::currentDate().toString("yyyy-MM-dd") + "T00:00:00'} | "
                                                                                                   "Select-Object -First 1 -ExpandProperty TimeCreated");

    if (!process.waitForFinished(5000)) return QDateTime();
    QString output = process.readAllStandardOutput().trimmed();
    if (output.isEmpty()) return QDateTime();
    return QDateTime::fromString(output.left(19), "yyyy-MM-dd HH:mm:ss");
}

int MainWindow::getTodaysActualUptimeSeconds()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime firstWake = getTodaysFirstWakeTime();
    if (!firstWake.isValid()) {
        ULONGLONG uptimeMs = GetTickCount64();
        return static_cast<int>(uptimeMs / 1000);
    }

    QProcess process;
    process.start("powershell", QStringList() << "-Command"
                                              << "$events = Get-WinEvent -FilterHashtable @{LogName='System'; StartTime='" +
                                                     QDate::currentDate().toString("yyyy-MM-dd") + "T00:00:00'} | "
                                                                                                   "Where-Object { $_.Id -eq 1 -or $_.Id -eq 42 -or $_.Id -eq 13 -or $_.Id -eq 1074 } | "
                                                                                                   "Select-Object Id, TimeCreated | Sort-Object TimeCreated; "
                                                                                                   "$events | ForEach-Object { Write-Output \\\"$($_.Id)|$($_.TimeCreated.ToString('yyyy-MM-dd HH:mm:ss'))\\\" }");

    if (!process.waitForFinished(5000)) {
        return static_cast<int>(firstWake.secsTo(now));
    }

    QString output = process.readAllStandardOutput().trimmed();
    if (output.isEmpty()) {
        return static_cast<int>(firstWake.secsTo(now));
    }

    QList<QPair<bool, QDateTime>> events; // true=开机, false=关机
    QStringList lines = output.split("\n", Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QStringList parts = line.split("|");
        if (parts.size() < 2) continue;
        int eventId = parts[0].trimmed().toInt();
        QDateTime time = QDateTime::fromString(parts[1].trimmed().left(19), "yyyy-MM-dd HH:mm:ss");
        if (!time.isValid()) continue;

        bool isWake = (eventId == 1);
        events.append({isWake, time});
    }

    if (events.isEmpty()) {
        return static_cast<int>(firstWake.secsTo(now));
    }

    int totalSecs = 0;
    QDateTime lastWake;
    bool isAwake = false;

    for (const auto &event : events) {
        if (event.first) { // 开机
            lastWake = event.second;
            isAwake = true;
        } else { // 关机
            if (isAwake && lastWake.isValid()) {
                totalSecs += static_cast<int>(lastWake.secsTo(event.second));
                isAwake = false;
            }
        }
    }

    if (isAwake && lastWake.isValid()) {
        totalSecs += static_cast<int>(lastWake.secsTo(now));
    }

    return totalSecs;
}
