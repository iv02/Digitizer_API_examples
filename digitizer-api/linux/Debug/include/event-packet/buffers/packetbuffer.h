#pragma once

#include "packetparser.h"
#include "packets/eventpackettype.h"
#include "packets/waveformnetworkpacket.h"

#include <QObject>
#include <QPair>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <QVector>

#include <any>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace network
{

class BufferProcessor;
class SplitUpPacketAssembler;

using ParserPairKey = QPair<EventPacketType, EventPacketType>;

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
    ~PacketBuffer() override;

    void setParserPoolSizeForTests(int poolSize);

    void setMeasurementStopped(bool stopped);
    void clearIncomingBuffer();

    template <typename T> void addParser(PacketParser<T> *parser);
    template <typename InfoT, typename WaveT> void addParserPair(PacketParser<InfoT> *infoParser, PacketParser<WaveT> *waveParser);

    template <typename T> std::optional<EventError> processParserResult(std::expected<std::pair<T, QByteArray>, EventError> result) const;

    std::optional<EventError> processParserSplitUpResult(std::expected<std::pair<WaveformNetworkPacket, QByteArray>, EventError> result) const;
    std::optional<EventError> processParserInterleavedResult(std::expected<std::pair<WaveformNetworkPacket, QByteArray>, EventError> result) const;

    void processData(QTcpSocket *socket) const;

  private:
    struct ParserPool
    {
        std::vector<void *> workers;
        std::vector<std::unique_ptr<QThread>> threads;
        mutable int nextIndex{};
    };

    void flushBrokenData(QByteArray &buffer) const;
    void onParsed(const std::any &packet, EventPacketType type, const QByteArray &raw) const;
    void onParseFailed(EventError error, EventPacketType type);
    void enqueueParsed(const std::any &packet) const;
    void flushPendingPackets() const;

    template <typename T> std::expected<QByteArray, EventError> readPacketBytes(QByteArray &buffer, EventPacketType type) const;
    template <typename T>     void dispatchToWorker(EventPacketType type, const QByteArray &raw) const;
    void dispatchToWorkerSlice(EventPacketType type, const QSharedPointer<QByteArray> &buffer, int offset, int length) const;
    void dispatchToWorkerSlices(EventPacketType type, const QSharedPointer<QByteArray> &buffer, const QVector<QPair<int, int>> &slices) const;

    bool hasParserForType(EventPacketType type) const;
    bool hasParserPair(EventPacketType infoType, EventPacketType waveType) const;
    std::optional<ParserPairKey> getParserPairForType(EventPacketType type) const;
    void dispatchToPairWorker(EventPacketType infoType, EventPacketType waveType, const QSharedPointer<QByteArray> &buffer, int infoOffset, int infoLength,
                              int waveOffset, int waveLength) const;
    void dispatchToPairWorkerSingle(EventPacketType infoType, EventPacketType waveType, const QSharedPointer<QByteArray> &buffer, int offset, int length,
                                    bool isInfo) const;

    quint32 m_deviceId{};
    bool m_measurementStopped{false};
    std::unique_ptr<SplitUpPacketAssembler> m_splitUpPacketAssembler;
    std::unique_ptr<QThread> m_processingThread;
    std::unique_ptr<BufferProcessor> m_bufferWorker;
    QTimer *m_emitTimer{nullptr};
    int m_parserPoolSize{16};

    std::map<EventPacketType, ParserPool> m_parserPools;
    std::map<ParserPairKey, ParserPool> m_parserPairPools;

    mutable std::vector<std::any> m_parsedPending;
};

using SliceVecMeta = QVector<QPair<int, int>>;

} // namespace network

Q_DECLARE_METATYPE(network::SliceVecMeta)

#include "packetbuffer.inl"
