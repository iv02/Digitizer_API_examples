#pragma once

#include <QByteArray>
#include <QObject>
#include <QTcpSocket>

#include <memory>
#include <mutex>
#include <optional>
#include <queue>

namespace network
{

class SocketReader : public QObject
{
    Q_OBJECT

  signals:
    void dataAvailable();

  public:
    explicit SocketReader(QObject *parent = nullptr);
    ~SocketReader() override = default;

  public slots:
    std::optional<std::shared_ptr<QByteArray>> tryTake();
    bool isEmpty() const;
    void onDataReady(QTcpSocket *socket);

  private:
    mutable std::mutex m_mutex;
    std::queue<std::shared_ptr<QByteArray>> m_queue;
};

} // namespace network
