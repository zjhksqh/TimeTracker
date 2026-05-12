#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTableWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCalendarWidget>
#include <QChartView>
#include <QShortcut>

#include "ProcessTracker.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private slots:
    void refreshUI();
    void onDateSelected(const QDate &date);
    void showMainWindow();
    void exportData();
    void openSettings();
    void toggleVisibility();

private:
    void setupUI();
    void setupTrayIcon();
    void setupCharts(const QDate &date);
    void registerGlobalHotkey();
    void unregisterGlobalHotkey();
    QDateTime getTodaysFirstWakeTime();
    int getTodaysActualUptimeSeconds();
    ProcessTracker *m_tracker;
    QTimer *m_uiRefreshTimer;

    QLabel *m_totalUptimeLabel;
    QLabel *m_runningCountLabel;
    QTableWidget *m_processTable;
    QChartView *m_chartView;
    QCalendarWidget *m_calendar;

    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;

    QDate m_selectedDate;
    ATOM m_hotkeyAtom = 0;
};

#endif
