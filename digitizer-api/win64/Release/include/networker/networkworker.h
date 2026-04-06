#pragma once
#include "networkenums.h"
#include "frameparse/packets/eventpackettype.h"
#include "frameparse/packetwrappers/eventdata.h"
#include "threaded.h"

#include "measurement_data_pipeline.h"

#include <QObject>
#include <QSharedPointer>
#include <QUuid>
#include <chrono>
#include <deque>
#include <map>
#include <vector>

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
    void onEventPairsReady(const std::vector<network::EventData> &pairs) const;
    void onCommandDeviceNetworkEvent(int64_t id, NETWORK_DEVICE_EVENT event, const QVariantList &parameters);

  private:
    void addToBuffer(const QSharedPointer<EventPacket> &info, const QSharedPointer<EventPacket> &waveform) const;
    std::optional<quint16> deviceDataPort(int64_t deviceId) const;
    void setupSockets();
    void setupConnections();
    void processPendingDiscoverData();
    void processPendingConnection();
    void flushBuffer() const;
    void cancelMeasurementWithTimeTimer(int64_t id);
    void onMeasurementStopped(int64_t id);
    void onMeasurementStarted(int64_t id);
    void startMeasurementWithTimeTimer(int64_t id, uint durationMs);

  private:
    QUdpSocket *m_discover{nullptr};
    QTcpServer *m_data{nullptr};

    QUuid m_softwareId{};
    std::map<int64_t, client::Threaded<MaintainingDeviceConnector>> m_maintainConnectors{};
    std::map<int64_t, client::Threaded<CommandDeviceConnector>> m_commandConnectors{};
    std::map<int64_t, QSharedPointer<MeasurementDataPipeline>> m_pipelines{};
    std::map<quint16, int64_t> m_ports{};
    std::map<int64_t, QTimer *> m_measurementWithTimeTimers{};

    std::map<int64_t, std::chrono::time_point<std::chrono::steady_clock>> m_discoverTimes{};
    QTimer *m_discoverLostTimer;

    mutable std::deque<EventData> m_dataBuffer{};
    QTimer *m_flushTimer{nullptr};
    bool m_useBatchMode{false};
};
} // namespace network
