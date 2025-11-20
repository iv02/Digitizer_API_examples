#pragma once
#include "networkenums.h"
#include "packets/eventpackettype.h"
#include "packetwrappers/eventdata.h"
#include "threaded.h"

#include "buffers/packetbuffer.h"

#include <QObject>
#include <QUuid>
#include <map>

class DiscoverBroadcastMessage;
class QUdpSocket;
class QTcpServer;
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
enum class EventPacketType : uint8_t;
class EventPacket;
class MaintainingDeviceConnector;
class CommandDeviceConnector;
class PacketPendingBuffer;

class NetworkWorker : public QObject
{
    Q_OBJECT
    using CallbackBuildDevice = std::function<QVariant(const DiscoverBroadcastMessage &message, quint16 port)>;

  signals:
    void deviceNetworkEvent(int64_t id, NETWORK_DEVICE_EVENT event, QVariantList parameters) const;
    void dataReceivedEvent(client::DataSource source, QSharedPointer<EventPacket> info, QSharedPointer<EventPacket> waveform) const;
    void dataReceivedBatch(client::DataSource source, const QVector<EventData> &batch) const;
    void buildDevice(const uint32_t &deviceId, const DiscoverBroadcastMessage &message, const quint16 &port);

  public:
    NetworkWorker(std::optional<unsigned int> flushInterval = std::nullopt, QObject *parent = nullptr);
    ~NetworkWorker() override = default;

    void sendCommand(int64_t id, NETWORK_DEVICE_COMMAND command, const QVariantList &parameters);

  private slots:
    void onConnectionLost(int64_t id) noexcept;
    void onDiscoverLostTimeout() noexcept;
    void onDataReceivedEvent(const std::any &packet) const;

  private:
    void addToBuffer(QSharedPointer<EventPacket> info, QSharedPointer<EventPacket> waveform) const;
    std::optional<quint16> deviceDataPort(int64_t deviceId) const;
    void setupSockets();
    void setupConnections();
    void processPendingDiscoverData();
    void processPendingConnection();
    void flushBuffer() const;

  private:
    QUdpSocket *m_discover{nullptr};
    QTcpServer *m_data{nullptr};

    QUuid m_softwareId{};
    std::map<int64_t, client::Threaded<MaintainingDeviceConnector>> m_maintainConnectors{};
    std::map<int64_t, client::Threaded<CommandDeviceConnector>> m_commandConnectors{};
    std::map<int64_t, QSharedPointer<PacketBuffer>> m_buffers{};
    std::map<quint16, int64_t> m_ports{};

    std::map<int64_t, std::chrono::time_point<std::chrono::steady_clock>> m_discoverTimes{};
    QTimer *m_discoverLostTimer;

    std::map<int64_t, QSharedPointer<EventPacket>> m_packetStorage{};

    mutable QVector<EventData> m_dataBuffer{};
    QTimer *m_flushTimer{nullptr};
    bool m_useBatchMode{false};
};
} // namespace network
