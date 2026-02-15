#pragma once

#include "packetparser.h"
#include "packetparserworker.h"
#include "packets/eventpackettype.h"

#include <QMetaObject>
#include <QObject>
#include <QQueue>
#include <QSharedPointer>

namespace network
{

template <typename InfoT, typename WaveT> class ParserPairWorker final : public PacketParserWorkerBase
{
  public:
    explicit ParserPairWorker(std::unique_ptr<PacketParser<InfoT>> infoParser, std::unique_ptr<PacketParser<WaveT>> waveParser, QObject *parent = nullptr)
        : PacketParserWorkerBase(parent), m_infoParser(std::move(infoParser)), m_waveParser(std::move(waveParser))
    {
    }

    ~ParserPairWorker() override = default;

    void parseBytes(const QByteArray &bytes) override
    {
        auto shared = QSharedPointer<QByteArray>::create(bytes);
        QVector<QPair<int, int>> slices;
        slices.push_back(qMakePair(0, static_cast<int>(shared->size())));
        enqueueSlices(shared, slices);
    }

    void enqueueSlices(const QSharedPointer<QByteArray> &buffer, const QVector<QPair<int, int>> &slices) override
    {
        if (!buffer || slices.isEmpty())
            return;

        if (slices.size() >= 2)
        {
            const auto &[o1, l1] = slices[0];
            const auto &[o2, l2] = slices[1];
            enqueuePairJob(buffer, o1, l1, o2, l2);
        }
        else
        {
            const auto &[offset, length] = slices.front();
            enqueueSingleJob(buffer, offset, length, true);
        }
    }

    void enqueuePairJob(const QSharedPointer<QByteArray> &buffer, int infoOffset, int infoLength, int waveOffset, int waveLength) override
    {
        if (!buffer)
            return;

        m_queue.enqueue(Job{buffer, infoOffset, infoLength, waveOffset, waveLength});

        if (!m_busy && !m_queue.isEmpty())
        {
            m_busy = true;
            QMetaObject::invokeMethod(this, [this] { processNext(); }, Qt::QueuedConnection);
        }
    }

    void enqueueSingleJob(const QSharedPointer<QByteArray> &buffer, int offset, int length, bool isInfo) override
    {
        if (!buffer || offset < 0 || length <= 0)
            return;

        if (isInfo)
            m_queue.enqueue(Job{buffer, offset, length, -1, -1});
        else
            m_queue.enqueue(Job{buffer, -1, -1, offset, length});

        if (!m_busy && !m_queue.isEmpty())
        {
            m_busy = true;
            QMetaObject::invokeMethod(this, [this] { processNext(); }, Qt::QueuedConnection);
        }
    }

    EventPacketType infoType() const
    {
        return m_infoParser->packetType();
    }

    EventPacketType waveType() const
    {
        return m_waveParser->packetType();
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
        const bool hasInfo = job.infoOffset >= 0 && job.infoLength > 0;
        const bool hasWave = job.waveOffset >= 0 && job.waveLength > 0;

        if (hasInfo && hasWave)
        {
            const QByteArrayView infoView(job.buffer->constData() + job.infoOffset, job.infoLength);
            const QByteArrayView waveView(job.buffer->constData() + job.waveOffset, job.waveLength);

            const auto infoResult = m_infoParser->parsePacket(infoView);
            if (!infoResult.has_value())
            {
                emit parseFailed(infoResult.error(), m_infoParser->packetType());
                emit parseFailed(infoResult.error(), m_waveParser->packetType());
                QMetaObject::invokeMethod(this, [this] { processNext(); }, Qt::QueuedConnection);
                return;
            }

            const auto waveResult = m_waveParser->parsePacket(waveView);
            if (!waveResult.has_value())
            {
                emit parseFailed(waveResult.error(), m_infoParser->packetType());
                emit parseFailed(waveResult.error(), m_waveParser->packetType());
                QMetaObject::invokeMethod(this, [this] { processNext(); }, Qt::QueuedConnection);
                return;
            }

            const auto &infoPacket = infoResult->first;
            const auto &wavePacket = waveResult->first;
            if (infoPacket.rtc != wavePacket.rtc)
            {
                emit parseFailed(EventError::RtcMismatch, m_infoParser->packetType());
                emit parseFailed(EventError::RtcMismatch, m_waveParser->packetType());
                QMetaObject::invokeMethod(this, [this] { processNext(); }, Qt::QueuedConnection);
                return;
            }

            emit parsed(std::any(infoPacket), m_infoParser->packetType(), infoResult->second);
            emit parsed(std::any(wavePacket), m_waveParser->packetType(), waveResult->second);
        }
        else if (hasInfo)
        {
            const QByteArrayView view(job.buffer->constData() + job.infoOffset, job.infoLength);
            const auto result = m_infoParser->parsePacket(view);
            if (!result.has_value())
            {
                emit parseFailed(result.error(), m_infoParser->packetType());
            }
            else
            {
                const auto &parseResult = *result;
                emit parsed(std::any(parseResult.first), m_infoParser->packetType(), parseResult.second);
            }
        }
        else if (hasWave)
        {
            const QByteArrayView view(job.buffer->constData() + job.waveOffset, job.waveLength);
            const auto result = m_waveParser->parsePacket(view);
            if (!result.has_value())
            {
                emit parseFailed(result.error(), m_waveParser->packetType());
            }
            else
            {
                const auto &parseResult = *result;
                emit parsed(std::any(parseResult.first), m_waveParser->packetType(), parseResult.second);
            }
        }

        QMetaObject::invokeMethod(this, [this] { processNext(); }, Qt::QueuedConnection);
    }

    struct Job
    {
        QSharedPointer<QByteArray> buffer;
        int infoOffset{-1};
        int infoLength{0};
        int waveOffset{-1};
        int waveLength{0};
    };

    std::unique_ptr<PacketParser<InfoT>> m_infoParser;
    std::unique_ptr<PacketParser<WaveT>> m_waveParser;
    QQueue<Job> m_queue{};
    bool m_busy{false};
};

} // namespace network
