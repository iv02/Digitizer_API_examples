#pragma once

#include "packetparser.h"

#include <QByteArray>
#include <QObject>
#include <QPromise>
#include <QTimer>

#include <QSharedPointer>
#include <any>
#include <atomic>
#include <expected>
#include <memory>

namespace network
{

class PacketBuffer;

struct PacketSlice
{
    EventPacketType type;
    int offset{};
    int length{};
};

class BufferProcessor final : public QObject
{
    Q_OBJECT

    enum class ScanStatus : uint8_t
    {
        Ready = 0,
        NeedMore = 1,
        Flush = 2
    };

  public:
    explicit BufferProcessor(PacketBuffer *owner);
    ~BufferProcessor() override = default;

    template <typename T> static std::expected<void, EventError> dispatchPacket(BufferProcessor *self, EventPacketType type);

  public slots:
    void initialize();
    void processChunk(const QByteArray &chunk);

  private:
    void parseBuffer();
    void finishActiveBatch();
    void dispatchSlice(EventPacketType type, int offset, int length) const;
    ScanStatus buildSlice(int offset, PacketSlice &outSlice);

  public slots:
    void onWorkerParsed();
    void onWorkerFailed();

  private:
    struct Batch
    {
        int consumedBytes{};
        QSharedPointer<QByteArray> bytes;
        std::shared_ptr<QPromise<void>> promise;
        std::shared_ptr<std::atomic<int>> remaining;
    };

    PacketBuffer *m_owner{};
    QByteArray m_buffer;
    QTimer *m_flushTimer{nullptr};
    std::shared_ptr<Batch> m_activeBatch;
};

} // namespace network
