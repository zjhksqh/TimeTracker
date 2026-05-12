#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    void saveSettings();

private:
    QCheckBox *m_autoStartCheck;
    QSpinBox *m_scanIntervalSpin;
    QSpinBox *m_persistIntervalSpin;
    QCheckBox *m_floatWindowCheck;
};

#endif
