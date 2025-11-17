#pragma once

#include "networkenums.h"

#include <QStateMachine>

class QTimer;

namespace device
{

enum DeviceStateFlag
{
    Disconnect                = 0b00000000,
    TryingToConnectDisconnect = 0b00000001,    // 1 = if disconnected - able to connect, if connected - able to disconnect | 0 - connected or disconnected stable state
    Connected                 = 0b00000010,    // 1 = connected | 0 = disconnected
    HasConnectionError        = 0b00000100,    // 1 = has error message while connect/disconnect | 0 - has no errors
    TryingToStartStopMeasure  = 0b00001000,    // 1 = if not measuring - able to start measure, if measuring - able to stop measuring | 0 - measuring or stopped stable state
    Measuring                 = 0b00010000,    // 1 = measuring | 0 = stopped
    StartFirmwareUpload       = 0b00100000,    // 1 = firmware upload started
    FinishFirmwareUpload      = 0b01000000     // 1 = firmware upload finished
};

class DeviceStateMachine : public QStateMachine
{
    Q_OBJECT

    enum class State
    {
        Disconnected = 0,
        TryingToConnect,
        Connected,
        TryingToDisconnect,
        TryingToMeasure,
        Measuring,
        TryingToStop,
        Uploading
    };

  signals:
    // inside API
    void commandConnect() const;
    void commandDisconnect() const;
    void commandMeasure() const;
    void commandStop() const;
    void commandStartUploadFirmware() const;

    void eventConnected() const;
    void eventConnectionError() const;
    void eventDisconnected() const;
    void eventDisconnectionError() const;
    void eventMeasureStarted() const;
    void eventStartMeasureError() const;
    void eventMeasureStopped() const;
    void eventStopMeasureError() const;
    void eventFirmwareUploadSuccess() const;
    void eventFirmwareUploadError() const;

    // outside API
    void stateChanged(QFlags<DeviceStateFlag>) const;
    void deviceCommandEmit(network::NETWORK_DEVICE_COMMAND command, QVariantList parameters);

  public:
    DeviceStateMachine(QObject *parent = nullptr);
    ~DeviceStateMachine() override = default;

    QFlags<DeviceStateFlag> stateFlags() const;

    static bool isLastAttemtRestartMeasuring(const QVariantList &parameters);

  public slots:
    void onDeviceNetworkEvent(network::NETWORK_DEVICE_EVENT event, QVariantList parameters);
    void onDeviceCommandReceived(network::NETWORK_DEVICE_COMMAND command, QVariantList parameters) const;

  private:
    void setupStates();
    void setupTransitions();
    void setupStateChangeActions();

    void buildState(State state);
    void buildTransition(State stateFrom, State stateTo, void (DeviceStateMachine::*signal)() const) const;

  private:
    bool m_hasShemaAndValues{false};

    std::map<State, QState *> m_states{};
    QFlags<DeviceStateFlag> m_stateFlags{};

    QTimer* m_reconnectTimer{};
    int m_reconnectAttempts{};
};

} // namespace device
