#pragma once

#include "networkworker.h"
#include "eventdata.h"
#include "packetwrappers/eventpacket.h"

#include <QObject>

namespace network
{

class NetworkMediator : public QObject
{
    Q_OBJECT

  public:
    NetworkMediator(QObject *parent = nullptr);

  signals:
    void deviceNetworkEvent(int64_t id, NETWORK_DEVICE_EVENT event, QVariantList parameters) const;
    void dataReceivedEvent(client::DataSource source, QSharedPointer<EventPacket> info, QSharedPointer<EventPacket> waveform) const;
    void dataReceivedBatch(client::DataSource source, const QVector<EventData> &batch) const;

  public slots:
    void initialize();
    void onBuildDevice(const uint32_t &deviceId, const DiscoverBroadcastMessage &message, const quint16 &port) const;
    void sendCommand(int64_t id, NETWORK_DEVICE_COMMAND command, const QVariantList &parameters) const;

  private:
    NetworkWorker *m_networkWorker{nullptr};
};

} // namespace network
