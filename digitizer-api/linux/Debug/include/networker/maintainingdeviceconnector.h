#pragma once

#include "api.grpc.pb.h"

#include <QDateTime>
#include <QObject>
#include <QUuid>

class QTimer;

namespace network
{

class MaintainingDeviceConnector : public QObject
{
    Q_OBJECT

  signals:
    void connectionEstablished(int64_t id);
    void connectionLost(int64_t id);

  public:
    MaintainingDeviceConnector(QObject *parent = nullptr);
    ~MaintainingDeviceConnector() override = default;

  public slots:
    bool isConnected() const;
    void initialize(int64_t deviceId, QUuid softwareId, std::shared_ptr<grpc::Channel> channel);
    void onDeviceConnected();
    void onDeviceDisconnected();

  private:
    void ping();

  private:
    int64_t m_deviceId{};
    QUuid m_softwareId{};

    std::unique_ptr<MaintainingApi::Stub> m_stub;

    bool m_connectionState{false};
    QTimer *m_pingTimer{nullptr};
    QDateTime m_lastPingTime{};
};

} // namespace network
