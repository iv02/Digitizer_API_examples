#pragma once

#include "packetparser.h"
#include "packetparserworker.h"
#include "packets/eventpackettype.h"
#include "packets/waveformnetworkpacket.h"

#include <QObject>
#include <QTcpSocket>
#include <QPair>
#include <QThread>
#include <QMetaType>
#include <QTimer>

#include <any>
#include <memory>
#include <vector>

namespace network
{
class BufferProcessor;
class SplitUpPacketAssembler;

using Slice = QPair<int, int>;
using SliceVec = QVector<Slice>;

class PacketBuffer final : public QObject
{
    Q_OBJECT
    friend class BufferProcessor;
  signals:
    void packetParsed(const std::vector<std::any> &packets) const;
    void packetParsedRaw(const std::vector<std::pair<EventPacketType, QByteArray>> &packets) const;
    void dataChunkReady(const QByteArray &chunk) const;

  public:
    explicit PacketBuffer(quint32 deviceId, QObject *parent = nullptr);
    void setParserPoolSizeForTests(int poolSize);

    PacketBuffer(const PacketBuffer &other) = delete;
    PacketBuffer(PacketBuffer &&other) = delete;
    PacketBuffer &operator=(const PacketBuffer &other) = delete;
    PacketBuffer &operator=(PacketBuffer &&other) = delete;

    ~PacketBuffer() override;

    template <typename T> void addParser(PacketParser<T> *parser);

    template <typename T> std::optional<EventError> processParserResult(std::expected<std::pair<T, QByteArray>, EventError> result) const;

    std::optional<EventError> processParserSplitUpResult(std::expected<std::pair<WaveformNetworkPacket, QByteArray>, EventError> result) const;
    std::optional<EventError> processParserInterleavedResult(std::expected<std::pair<WaveformNetworkPacket, QByteArray>, EventError> result) const;

    void processData(QTcpSocket *socket) const;

  private:
    void flushBrokenData(QByteArray &buffer) const;
    template <typename T> std::expected<QByteArray, EventError> readPacketBytes(QByteArray &buffer, EventPacketType type) const;
    template <typename T> void dispatchToWorker(EventPacketType type, const QByteArray &raw) const;
    void dispatchToWorkerRaw(EventPacketType type, const QByteArray &raw) const;
    void dispatchToWorkerSlice(EventPacketType type, const QSharedPointer<QByteArray> &buffer, int offset, int length) const;
    void dispatchToWorkerSlices(EventPacketType type, const QSharedPointer<QByteArray> &buffer, const SliceVec &slices) const;
    void onParsed(const std::any &packet, EventPacketType type, const QByteArray &raw) const;
    static void onParseFailed(EventError error, EventPacketType type);
    void enqueueParsed(const std::any &packet) const;
    void flushPendingPackets() const;

  private:
    quint32 m_deviceId{};
    struct ParserPool
    {
        std::vector<void *> workers;
        std::vector<std::unique_ptr<QThread>> threads;
        mutable int nextIndex{0};
    };

    std::map<EventPacketType, ParserPool> m_parserPools;
    std::unique_ptr<QThread> m_processingThread;
    std::unique_ptr<BufferProcessor> m_bufferWorker;
    std::unique_ptr<SplitUpPacketAssembler> m_splitUpPacketAssembler;
    mutable std::vector<std::any> m_parsedPending{};
    mutable QTimer *m_emitTimer{nullptr};
    int m_parserPoolSize{32};
};

} // namespace network

Q_DECLARE_METATYPE(network::Slice)
Q_DECLARE_METATYPE(network::SliceVec)

#include "packetbuffer.inl"