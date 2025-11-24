#pragma once

#include "api.grpc.pb.h"
#include "api.pb.h"

#include <QDateTime>
#include <QObject>
#include <QUuid>

namespace network
{
enum class NETWORK_DEVICE_EVENT;
enum class NETWORK_DEVICE_COMMAND;

class CommandDeviceConnector : public QObject
{
    Q_OBJECT
  signals:
    void deviceNetworkEvent(int64_t deviceId, NETWORK_DEVICE_EVENT event, QVariantList parameters) const;

  public:
    CommandDeviceConnector(QObject *parent = nullptr);
    ~CommandDeviceConnector() override = default;

  public slots:
    void initialize(int64_t deviceId, QUuid softwareId, std::shared_ptr<grpc::Channel> channel, quint16 port);
    void sendCommand(NETWORK_DEVICE_COMMAND command, QVariantList parameters) const;

  private slots:
    void processConnectDevice() const;
    void processDisconnectDevice() const;
    void processUpdateDeviceFirmware(QVariantList parameters) const;
    void processUpdateDeviceRootFS(QVariantList parameters) const;
    void processRebootDevice() const;
    void processGetFirmwareSettingsSchema() const;
    void processGetFirmwareSettingsValues() const;
    void processSetFirmwareSettingsValues(QVariantList parameters) const;
    void processGetProtocolVersion() const;
    void processStartDeviceMeasurement(QVariantList parameters) const;
    void processStartDeviceMeasurementWithTime(QVariantList parameters) const;
    void processStopDeviceMeasurement(QVariantList parameters) const;

  private:
    int64_t m_deviceId{};
    QUuid m_softwareId{};

    std::unique_ptr<CommandApi::Stub> m_stub;

    quint16 m_port{};
};

} // namespace network
