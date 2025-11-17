#pragma once

#include <QWidget>

class QTextEdit;
class QPushButton;
class QStandardItemModel;
class QTableView;

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

  private slots:
    void onDiscoverDevices();
    void onConnectDevice();
    void onDisconnectDevice();
    void onStartMeasure();
    void onStopMeasure();
    void onClearLog();
    void onDeviceSelectionChanged();

  private:
    void setupUi();
    void setupConnections();
    void setupTableStyle();

    digi::DigitizerInteractor *m_interactor;
    QTextEdit *m_logEdit;
    QStandardItemModel *m_devicesModel;
    QTableView *m_devicesTable;
    QMap<int64_t, QList<QString>> m_devices;

    QPushButton *m_pbDiscoverDevices;
    QPushButton *m_pbConnectDevice;
    QPushButton *m_pbDisConnectDevice;
    QPushButton *m_pbStartMeasure;
    QPushButton *m_pbStopMeasure;
    QPushButton *m_pbClearLog;
};

