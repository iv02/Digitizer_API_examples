#pragma once

#include <QMainWindow>
#include "packetwrappers/eventdata.h"

class QTimer;

namespace digi
{
class DigitizerInteractor;
}

class DeviceControlPanel;
class SettingsPanel;
class WaveformSpectrumWidget;
class DataTableWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

  private slots:
    void onDeviceSelectionChanged(int64_t deviceId);
    void onUpdateTimer();

  private:
    void setupUi();
    void setupConnections();
    void setStyle();

    digi::DigitizerInteractor *m_digitizerInteractor;
    DeviceControlPanel *m_deviceControlPanel;
    SettingsPanel *m_settingsPanel;
    WaveformSpectrumWidget *m_waveformSpectrumWidget;
    DataTableWidget *m_dataTableWidget;
    
    QTimer *m_updateTimer;
    network::EventData m_pendingEventData;
    bool m_hasPendingData;
};
