#pragma once

#include <QMainWindow>

namespace digi
{
class DigitizerInteractor;
}

class DeviceControlPanel;
class SettingsPanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

  private slots:
    void onDeviceSelectionChanged(int64_t deviceId);

  private:
    void setupUi();
    void setupConnections();
    void setStyle();

    digi::DigitizerInteractor *m_digitizerInteractor;
    DeviceControlPanel *m_deviceControlPanel;
    SettingsPanel *m_settingsPanel;
};
