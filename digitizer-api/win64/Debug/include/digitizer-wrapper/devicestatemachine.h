#pragma once

#include "networkenums.h"

#include <QStateMachine>
#include <QTimer>

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
    void commandConnect();
    void commandDisconnect();
    void commandMeasure();
    void commandStop();
    void commandStartUploadFirmware();

    void eventConnected();
    void eventConnectionError();
    void eventDisconnected();
    void eventDisconnectionError();
    void eventMeasureStarted();
    void eventStartMeasureError();
    void eventMeasureStopped();
    void eventStopMeasureError();
    void eventFirmwareUploadSuccess();
    void eventFirmwareUploadError();

    // outside API
    void stateChanged(QFlags<DeviceStateFlag>);
    void deviceCommandEmit(network::NETWORK_DEVICE_COMMAND command, QVariantList parameters);

  public:
    DeviceStateMachine(QObject *parent = nullptr);
    ~DeviceStateMachine() override = default;

    QFlags<DeviceStateFlag> stateFlags() const;

  public slots:
    void onDeviceNetworkEvent(network::NETWORK_DEVICE_EVENT event, const QVariantList &parameters);
    void onDeviceCommandReceived(network::NETWORK_DEVICE_COMMAND command, QVariantList parameters);

  private:
    void setupStates();
    void setupTransitions();
    void setupStateChangeActions();

    void buildState(State state);
    void buildTransition(State stateFrom, State stateTo, void (DeviceStateMachine::*signal)()) const;

  private:
    bool m_hasShemaAndValues{false};

    mutable network::NETWORK_DEVICE_COMMAND m_pendingUploadCommand{network::NETWORK_DEVICE_COMMAND::UPDATE_DEVICE_FIRMWARE};
    mutable network::NETWORK_DEVICE_COMMAND m_pendingMeasureCommand{network::NETWORK_DEVICE_COMMAND::START_DEVICE_MEASUREMENT};

    mutable QVariantList m_pendingMeasureParameters{};
    mutable QVariantList m_pendingUploadParameters{};

    QTimer *m_measureTimer{nullptr};
    mutable int m_measurementDurationMs{0};

    std::map<State, QState *> m_states{};
    QFlags<DeviceStateFlag> m_stateFlags{};
};

} // namespace device
