#pragma once

#include "buffer_view.h"
#include "spsc_byte_ring.h"

#include <QTcpSocket>

#include <atomic>

namespace network
{

inline constexpr size_t kDefaultRingCapacity = 1ull * 1024ull * 1024ull * 1024ull;

class SocketReader final : public QObject
{
    Q_OBJECT

  signals:
    void dataAvailable(network::BufferView view);

  public:
    explicit SocketReader(size_t capacity = kDefaultRingCapacity, QObject *parent = nullptr);
    ~SocketReader() override;

    SocketReader(const SocketReader &) = delete;
    SocketReader &operator=(const SocketReader &) = delete;

    [[nodiscard]] size_t capacity() const noexcept;
    [[nodiscard]] size_t available() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;

  public slots:
    void onDataReady(QTcpSocket *socket);
    void clearTail(qsizetype bytes);

  private:
    void drainSocket(QTcpSocket *socket);
    void tryEmitAvailable();

    SPSCByteRing m_ring;
    std::atomic_bool m_leaseActive{false};
};

} // namespace network
