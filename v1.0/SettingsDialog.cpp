#include "SettingsDialog.h"
#include <QFormLayout>
#include <QPushButton>
#include <QSettings>
#include <QApplication>
#include <QDir>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("设置");
    QFormLayout *form = new QFormLayout(this);

    m_autoStartCheck = new QCheckBox("开机自启");
    form->addRow(m_autoStartCheck);

    m_scanIntervalSpin = new QSpinBox();
    m_scanIntervalSpin->setRange(1, 30);
    m_scanIntervalSpin->setValue(3);
    m_scanIntervalSpin->setSuffix(" 秒");
    form->addRow("扫描间隔", m_scanIntervalSpin);

    m_persistIntervalSpin = new QSpinBox();
    m_persistIntervalSpin->setRange(10, 600);
    m_persistIntervalSpin->setValue(60);
    m_persistIntervalSpin->setSuffix(" 秒");
    form->addRow("持久化间隔", m_persistIntervalSpin);

    m_floatWindowCheck = new QCheckBox("启用悬浮窗");
    form->addRow(m_floatWindowCheck);

    QPushButton *saveBtn = new QPushButton("保存");
    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::saveSettings);
    form->addRow(saveBtn);

    QSettings settings("TimeTracker", "TimeTracker");
    m_autoStartCheck->setChecked(settings.value("autoStart", false).toBool());
    m_scanIntervalSpin->setValue(settings.value("scanInterval", 3).toInt());
    m_persistIntervalSpin->setValue(settings.value("persistInterval", 60).toInt());
    m_floatWindowCheck->setChecked(settings.value("floatWindow", false).toBool());
}

void SettingsDialog::saveSettings()
{
    QSettings settings("TimeTracker", "TimeTracker");
    settings.setValue("autoStart", m_autoStartCheck->isChecked());
    settings.setValue("scanInterval", m_scanIntervalSpin->value());
    settings.setValue("persistInterval", m_persistIntervalSpin->value());
    settings.setValue("floatWindow", m_floatWindowCheck->isChecked());

    QString appPath = QApplication::applicationFilePath();
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                  QSettings::NativeFormat);
    if (m_autoStartCheck->isChecked()) {
        reg.setValue("TimeTracker", QDir::toNativeSeparators(appPath));
    } else {
        reg.remove("TimeTracker");
    }

    accept();
}

