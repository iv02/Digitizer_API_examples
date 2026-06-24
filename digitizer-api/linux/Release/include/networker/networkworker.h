#pragma once

#include "frameparse/packets/eventpackettype.h"
#include "frameparse/packetwrappers/eventdata.h"
#include "networkenums.h"
#include "threaded.h"

#include <QObject>
#include <QSharedPointer>
#include <QTcpServer>
#include <QUuid>

#include <chrono>
#include <deque>
#include <map>
#include <vector>

class DiscoverBroadcastMessage;
class QUdpSocket;
class QTimer;

namespace client
{
enum class DataSource;
class SettingsRepository;
} // namespace client

namespace device
{
enum class DataType;
}

namespace network
{
class MeasurementDataPipeline;
enum class EventPacketType : uint8_t;
class EventPacket;
class MaintainingDeviceConnector;
class CommandDeviceConnector;

class NetworkWorker final : public QObject
{
    Q_OBJECT

  signals:
    void deviceNetworkEvent(int64_t id, NETWORK_DEVICE_EVENT event, QVariantList parameters) const;
    void dataReceivedEvent(client::DataSource source, QSharedPointer<EventPacket> info, QSharedPointer<EventPacket> waveform) const;
    void dataReceivedBatch(client::DataSource source, const QSharedPointer<QVector<EventData>> &batch) const;
    void buildDevice(const uint32_t &deviceId, const DiscoverBroadcastMessage &message, const quint16 &port);

  public:
    NetworkWorker(std::optional<unsigned int> flushInterval = std::nullopt, QObject *parent = nullptr);
    ~NetworkWorker() override = default;

    void sendCommand(int64_t id, NETWORK_DEVICE_COMMAND command, const QVariantList &parameters);

  private slots:
    void onConnectionLost(int64_t id) noexcept;
    void onDiscoverLostTimeout() noexcept;
    void onEventPairsReady(const std::vector<EventData> &pairs) const;
    void onCommandDeviceNetworkEvent(int64_t id, NETWORK_DEVICE_EVENT event, const QVariantList &parameters);

  private:
    void addToBuffer(const QSharedPointer<EventPacket> &info, const QSharedPointer<EventPacket> &waveform) const;
    std::optional<quint16> deviceDataPort(int64_t deviceId) const;
    void setupSockets();
    void setupConnections();
    void processPendingDiscoverData();
    void flushBuffer() const;

  private:
    QUdpSocket *m_discover{nullptr};
    std::map<int64_t, std::unique_ptr<QTcpServer>> m_dataServers{};

    QUuid m_softwareId{};
    std::map<int64_t, client::Threaded<MaintainingDeviceConnector>> m_maintainConnectors{};
    std::map<int64_t, client::Threaded<CommandDeviceConnector>> m_commandConnectors{};
    std::map<int64_t, QSharedPointer<MeasurementDataPipeline>> m_pipelines{};
    std::map<quint16, int64_t> m_ports{};

    std::map<int64_t, std::chrono::time_point<std::chrono::steady_clock>> m_discoverTimes{};
    QTimer *m_discoverLostTimer;

    mutable std::map<int64_t, std::deque<EventData>> m_dataBuffers{};

    QTimer *m_flushTimer{nullptr};
    bool m_useBatchMode{false};
};

} // namespace network
