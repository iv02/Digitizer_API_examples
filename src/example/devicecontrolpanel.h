#pragma once

#include <QWidget>

class QTextEdit;
class QPushButton;
class QStandardItemModel;
class QTableView;
class QMenuBar;
class QMenu;
class QAction;

namespace digi
{
class DigitizerInteractor;
}

class DeviceControlPanel : public QWidget
{
    Q_OBJECT

  public:
    explicit DeviceControlPanel(digi::DigitizerInteractor *interactor, QWidget *parent = nullptr);
    ~DeviceControlPanel() override = default;

    int64_t currentDeviceId() const;
    void refreshDevices();
    void appendLog(const QString &text);
    void clearLog();

  signals:
    void deviceSelectionChanged(int64_t deviceId);
    void deviceConnected(int64_t deviceId);

  private slots:
    void onDeviceDiscovered(int64_t deviceId);
    void onConnectDevice();
    void onDisconnectDevice();
    void onStartMeasure();
    void onStopMeasure();
    void onClearLog();
    void onDeviceSelectionChanged();
    void onLogFirmwareSettings();
    void onUploadSettings();
    void onDownloadSettings();

  private:
    void setupUi();
    void setupConnections();
    void setupTableStyle();
    void logMessage(const QString &message);

    digi::DigitizerInteractor *m_interactor;
    QTextEdit *m_logEdit;
    QStandardItemModel *m_devicesModel;
    QTableView *m_devicesTable;
    QMap<int64_t, QList<QString>> m_devices;

    QMenuBar *m_menuBar;
    QMenu *m_deviceMenu;
    QMenu *m_settingsMenu;
    QMenu *m_logMenu;
    
    QAction *m_actionConnectDevice;
    QAction *m_actionDisconnectDevice;
    QAction *m_actionStartMeasure;
    QAction *m_actionStopMeasure;
    QAction *m_actionShowFirmwareSettings;
    QAction *m_actionUploadSettings;
    QAction *m_actionDownloadSettings;
    QAction *m_actionClearLog;
};

