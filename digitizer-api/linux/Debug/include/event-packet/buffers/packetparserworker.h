#pragma once

#include "packetparser.h"

#include <QObject>
#include <QPair>
#include <QQueue>
#include <QSharedPointer>
#include <QVector>
#include <any>
#include <memory>

namespace network
{

using Slice = QPair<int, int>;
using SliceVec = QVector<Slice>;

class PacketParserWorkerBase : public QObject
{
    Q_OBJECT
  public:
    explicit PacketParserWorkerBase(QObject *parent = nullptr) : QObject(parent)
    {
    }
    ~PacketParserWorkerBase() override = default;

  signals:
    void parsed(const std::any &packet, EventPacketType type, const QByteArray &raw) const;
    void parseFailed(EventError error, EventPacketType type) const;

  public slots:
    virtual void parseBytes(const QByteArray &bytes) = 0;
    virtual void enqueueSlices(const QSharedPointer<QByteArray> &buffer, const SliceVec &slices) = 0;
};

template <typename T> class PacketParserWorker final : public PacketParserWorkerBase
{
  public:
    explicit PacketParserWorker(std::unique_ptr<PacketParser<T>> parser, QObject *parent = nullptr)
        : PacketParserWorkerBase(parent), m_parser(std::move(parser))
    {
    }

    ~PacketParserWorker() override = default;

  public:
    void parseBytes(const QByteArray &bytes) override
    {
        auto shared = QSharedPointer<QByteArray>::create(bytes);
        SliceVec slices;
        slices.push_back(qMakePair(0, static_cast<int>(shared->size())));
        enqueueSlices(shared, slices);
    }

    void enqueueSlices(const QSharedPointer<QByteArray> &buffer, const SliceVec &slices) override
    {
        if (!buffer)
        {
            emit parseFailed(EventError::ParseError, m_parser->packetType());
            return;
        }

        for (const auto &[offset, length] : slices)
        {
            if (offset < 0 || length <= 0 || offset + length > buffer->size())
            {
                emit parseFailed(EventError::ParseError, m_parser->packetType());
                continue;
            }
            m_queue.enqueue(Job{buffer, offset, length});
        }

        if (!m_busy && !m_queue.isEmpty())
        {
            m_busy = true;
            QMetaObject::invokeMethod(this, [this] { processNext(); }, Qt::QueuedConnection);
        }
    }

  private:
    void processNext()
    {
        if (m_queue.isEmpty())
        {
            m_busy = false;
            return;
        }

        const auto job = m_queue.dequeue();
        const QByteArrayView view(job.buffer->constData() + job.offset, job.length);

        const auto result = m_parser->parsePacket(view);
        if (!result.has_value())
        {
            emit parseFailed(result.error(), m_parser->packetType());
        }
        else
        {
            const auto &parsedPacket = *result;
            emit parsed(std::any(parsedPacket.first), m_parser->packetType(), parsedPacket.second);
        }

        QMetaObject::invokeMethod(this, [this] { processNext(); }, Qt::QueuedConnection);
    }

    struct Job
    {
        QSharedPointer<QByteArray> buffer;
        int offset{};
        int length{};
    };

    std::unique_ptr<PacketParser<T>> m_parser;
    QQueue<Job> m_queue{};
    bool m_busy{false};
};

} // namespace network
