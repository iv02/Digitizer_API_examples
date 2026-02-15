#pragma once

#include "bufferprocessor.h"
#include "parserpairworker.h"

#include "packets/detectron2dnetworkpacket.h"
#include "packets/detectronstatisticnetworkpacket.h"
#include "packets/devicespectrum16.h"
#include "packets/devicespectrum32.h"
#include "packets/phanetworkpacket.h"
#include "packets/psdnetworkpacket.h"
#include "packets/psdnetworkpacketv2.h"
#include "packets/waveformnetworkpacket.h"

#include <QDebug>
#include <limits>
#include <utility>

namespace network
{

template <typename T> void PacketBuffer::addParser(PacketParser<T> *parser)
{
    if (!parser)
        return;

    parser->setDeviceId(m_deviceId);

    const int poolSize = m_parserPoolSize;
    auto &pool = m_parserPools[parser->packetType()];

    for (int i = 0; i < poolSize; ++i)
    {
        auto parserInstance = (i == 0) ? parser : new PacketParser<T>(parser->packetType());
        parserInstance->setDeviceId(m_deviceId);

        auto worker = new PacketParserWorker<T>(std::unique_ptr<PacketParser<T>>(parserInstance));
        auto thread = std::make_unique<QThread>();

        worker->moveToThread(thread.get());

        connect(worker, &PacketParserWorkerBase::parsed, this, &PacketBuffer::onParsed, Qt::QueuedConnection);
        connect(worker, &PacketParserWorkerBase::parseFailed, this, &PacketBuffer::onParseFailed, Qt::QueuedConnection);
        if (m_bufferWorker)
        {
            connect(worker, &PacketParserWorkerBase::parsed, m_bufferWorker.get(), &BufferProcessor::onWorkerParsed, Qt::QueuedConnection);
            connect(worker, &PacketParserWorkerBase::parseFailed, m_bufferWorker.get(), &BufferProcessor::onWorkerFailed, Qt::QueuedConnection);
        }
        else
        {
            connect(worker, &PacketParserWorkerBase::parsed, this, &PacketBuffer::onParsed, Qt::QueuedConnection);
            connect(worker, &PacketParserWorkerBase::parseFailed, this, &PacketBuffer::onParseFailed, Qt::QueuedConnection);
        }
        connect(thread.get(), &QThread::finished, worker, &QObject::deleteLater);

        thread->start();

        pool.workers.push_back(static_cast<void *>(worker));
        pool.threads.push_back(std::move(thread));
    }
}

template <typename InfoT, typename WaveT> void PacketBuffer::addParserPair(PacketParser<InfoT> *infoParser, PacketParser<WaveT> *waveParser)
{
    if (!infoParser || !waveParser)
        return;

    infoParser->setDeviceId(m_deviceId);
    waveParser->setDeviceId(m_deviceId);

    const auto infoType = infoParser->packetType();
    const auto waveType = waveParser->packetType();
    const ParserPairKey key(infoType, waveType);
    const int poolSize = m_parserPoolSize;
    auto &pool = m_parserPairPools[key];

    for (int i = 0; i < poolSize; ++i)
    {
        auto infoParserInstance = (i == 0) ? infoParser : new PacketParser<InfoT>(infoType);
        auto waveParserInstance = (i == 0) ? waveParser : new PacketParser<WaveT>(waveType);
        infoParserInstance->setDeviceId(m_deviceId);
        waveParserInstance->setDeviceId(m_deviceId);

        auto worker = new ParserPairWorker<InfoT, WaveT>(std::unique_ptr<PacketParser<InfoT>>(infoParserInstance),
                                                         std::unique_ptr<PacketParser<WaveT>>(waveParserInstance));

        auto thread = std::make_unique<QThread>();
        worker->moveToThread(thread.get());

        connect(worker, &PacketParserWorkerBase::parsed, this, &PacketBuffer::onParsed, Qt::QueuedConnection);
        connect(worker, &PacketParserWorkerBase::parseFailed, this, &PacketBuffer::onParseFailed, Qt::QueuedConnection);
        if (m_bufferWorker)
        {
            connect(worker, &PacketParserWorkerBase::parsed, m_bufferWorker.get(), &BufferProcessor::onWorkerParsed, Qt::QueuedConnection);
            connect(worker, &PacketParserWorkerBase::parseFailed, m_bufferWorker.get(), &BufferProcessor::onWorkerFailed, Qt::QueuedConnection);
        }
        connect(thread.get(), &QThread::finished, worker, &QObject::deleteLater);

        thread->start();

        pool.workers.push_back(static_cast<void *>(worker));
        pool.threads.push_back(std::move(thread));
    }
}

template <typename T> std::optional<EventError> PacketBuffer::processParserResult(std::expected<std::pair<T, QByteArray>, EventError> result) const
{
    if (!result.has_value())
        return result.error();

    const auto &parsedResult = *result;
    enqueueParsed(std::any(parsedResult.first));

    constexpr std::optional<EventPacketType> packetType = [] -> std::optional<EventPacketType> {
        if constexpr (std::is_same_v<T, PsdNetworkPacket>)
            return EventPacketType::PsdEventInfo;
        if constexpr (std::is_same_v<T, PsdNetworkPacketV2>)
            return EventPacketType::PsdEventInfoV2;
        if constexpr (std::is_same_v<T, PhaNetworkPacket>)
            return EventPacketType::PhaEventInfo;
        if constexpr (std::is_same_v<T, Detectron2dNetworkPacket>)
            return EventPacketType::Detectron2DData;
        if constexpr (std::is_same_v<T, DetectronStatisticNetworkPacket>)
            return EventPacketType::DetectronStatisticData;
        if constexpr (std::is_same_v<T, DeviceSpectrum16>)
            return EventPacketType::DeviceSpectrum16;
        if constexpr (std::is_same_v<T, DeviceSpectrum32>)
            return EventPacketType::DeviceSpectrum32;

        return std::nullopt;
    }();

    if constexpr (std::is_same_v<T, WaveformNetworkPacket>)
    {
        std::vector<std::pair<EventPacketType, QByteArray>> batch;
        if (parsedResult.first.packetType == EventPacketType::PsdWaveform)
            batch.emplace_back(EventPacketType::PsdWaveform, parsedResult.second);

        if (parsedResult.first.packetType == EventPacketType::PhaWaveform)
            batch.emplace_back(EventPacketType::PhaWaveform, parsedResult.second);

        if (!batch.empty())
            emit packetParsedRaw(batch);

        return std::nullopt;
    }

    if constexpr (packetType.has_value())
    {
        std::vector<std::pair<EventPacketType, QByteArray>> batch;
        batch.emplace_back(*packetType, parsedResult.second);
        emit packetParsedRaw(batch);
    }

    return std::nullopt;
}

template <typename T> std::expected<QByteArray, EventError> PacketBuffer::readPacketBytes(QByteArray &buffer, EventPacketType type) const
{
    Q_UNUSED(type)

    if constexpr (FixedSizeStructure<T>)
    {
        if (buffer.size() < static_cast<int>(T::size()))
            return std::unexpected(EventError::NotEnoughBytes);

        const auto packetArray = buffer.left(T::size());
        buffer.remove(0, T::size());
        return packetArray;
    }
    else if constexpr (KnownSizeStructure<T>)
    {
        if (buffer.size() < static_cast<int>(T::fixedPartSize()))
            return std::unexpected(EventError::NotEnoughBytes);

        const auto arrayLength = *reinterpret_cast<const quint32 *>(buffer.constData() + T::arrayLengthOffset());
        const auto paddingLength = *reinterpret_cast<const quint16 *>(buffer.constData() + T::paddingLengthOffset());
        const quint64 totalSize64 = static_cast<quint64>(T::fixedPartSize()) + static_cast<quint64>(arrayLength) * static_cast<quint64>(T::arrayItemSize()) +
                                    static_cast<quint64>(paddingLength) * static_cast<quint64>(sizeof(qint16)) + static_cast<quint64>(sizeof(qint16));

        if (totalSize64 == 0 || totalSize64 > static_cast<quint64>(std::numeric_limits<int>::max()))
            return std::unexpected(EventError::ParseError);

        const int totalSize = static_cast<int>(totalSize64);

        if (buffer.size() < totalSize)
            return std::unexpected(EventError::NotEnoughBytes);

        auto packetArray = buffer.left(totalSize);
        buffer.remove(0, totalSize);
        return packetArray;
    }
    else if constexpr (UnknownSizeStructure<T>)
    {
        if (buffer.size() < static_cast<int>(T::fixedPartSize()))
            return std::unexpected(EventError::NotEnoughBytes);

        for (quint32 xyCounter = 0; xyCounter < T::arrayLimit(); ++xyCounter)
        {
            const quint64 mayBeSignaturePos64 =
                static_cast<quint64>(T::fixedPartSize()) + static_cast<quint64>(xyCounter) * static_cast<quint64>(T::arrayPartSize());
            const quint64 mayBePacketEnd64 = mayBeSignaturePos64 + static_cast<quint64>(T::signature().size()) + static_cast<quint64>(sizeof(quint16));

            if (std::cmp_greater(mayBePacketEnd64, buffer.size()))
                return std::unexpected(EventError::NotEnoughBytes);

            if (mayBeSignaturePos64 > static_cast<quint64>(std::numeric_limits<int>::max()) ||
                T::signature().size() > static_cast<size_t>(std::numeric_limits<int>::max()))
                return std::unexpected(EventError::ParseError);

            const int mayBeSignaturePos = static_cast<int>(mayBeSignaturePos64);
            const int signatureSize = static_cast<int>(T::signature().size());

            if (buffer.sliced(mayBeSignaturePos, signatureSize) != T::signature())
                continue;

            if (mayBePacketEnd64 > static_cast<quint64>(std::numeric_limits<int>::max()))
                return std::unexpected(EventError::ParseError);

            const int mayBePacketEnd = static_cast<int>(mayBePacketEnd64);
            auto packetArray = buffer.left(mayBePacketEnd);
            buffer.remove(0, mayBePacketEnd);
            return packetArray;
        }

        qWarning() << "No valid packet found in the buffer for type" << static_cast<int>(type);
        return std::unexpected(EventError::ParseError);
    }
    else
    {
        return std::unexpected(EventError::ParseError);
    }
}

template <typename T> void PacketBuffer::dispatchToWorker(EventPacketType type, const QByteArray &raw) const
{
    const auto poolIt = m_parserPools.find(type);
    if (poolIt == m_parserPools.end() || poolIt->second.workers.empty())
    {
        qWarning() << "Parser pool has no entry for type" << static_cast<int>(type);
        return;
    }

    auto &pool = poolIt->second;
    const auto idx = pool.nextIndex % static_cast<int>(pool.workers.size());
    pool.nextIndex = (pool.nextIndex + 1) % static_cast<int>(pool.workers.size());
    auto *worker = static_cast<PacketParserWorkerBase *>(pool.workers[idx]);
    if (!worker)
    {
        qWarning() << "Parser worker is null for type" << static_cast<int>(type);
        return;
    }

    QMetaObject::invokeMethod(worker, "parseBytes", Qt::QueuedConnection, Q_ARG(QByteArray, raw));
}

inline void PacketBuffer::dispatchToWorkerSlice(EventPacketType type, const QSharedPointer<QByteArray> &buffer, int offset, int length) const
{
    const auto poolIt = m_parserPools.find(type);
    if (poolIt == m_parserPools.end() || poolIt->second.workers.empty())
    {
        qWarning() << "Parser pool has no entry for type" << static_cast<int>(type);
        return;
    }

    auto &pool = poolIt->second;
    const auto idx = pool.nextIndex % static_cast<int>(pool.workers.size());
    pool.nextIndex = (pool.nextIndex + 1) % static_cast<int>(pool.workers.size());
    auto *worker = static_cast<PacketParserWorkerBase *>(pool.workers[idx]);
    if (!worker)
    {
        qWarning() << "Parser worker is null for type" << static_cast<int>(type);
        return;
    }

    QVector<QPair<int, int>> one;
    one.push_back(qMakePair(offset, length));
    QMetaObject::invokeMethod(worker, "enqueueSlices", Qt::QueuedConnection, Q_ARG(QSharedPointer<QByteArray>, buffer), Q_ARG(network::SliceVecMeta, one));
}

inline void PacketBuffer::dispatchToWorkerSlices(EventPacketType type, const QSharedPointer<QByteArray> &buffer, const QVector<QPair<int, int>> &slices) const
{
    const auto poolIt = m_parserPools.find(type);
    if (poolIt == m_parserPools.end() || poolIt->second.workers.empty())
    {
        qWarning() << "Parser pool has no entry for type" << static_cast<int>(type);
        return;
    }

    auto &pool = poolIt->second;
    const int workerCount = static_cast<int>(pool.workers.size());
    if (workerCount <= 0)
        return;

    QVector<QVector<QPair<int, int>>> buckets(workerCount);
    buckets.fill(QVector<QPair<int, int>>{});
    for (const auto &p : slices)
    {
        const int idx = pool.nextIndex % workerCount;
        pool.nextIndex = (pool.nextIndex + 1) % workerCount;
        buckets[idx].push_back(p);
    }

    for (int i = 0; i < workerCount; ++i)
    {
        if (buckets[i].isEmpty())
            continue;
        auto *worker = static_cast<PacketParserWorkerBase *>(pool.workers[i]);
        if (!worker)
            continue;

        QMetaObject::invokeMethod(worker, "enqueueSlices", Qt::QueuedConnection, Q_ARG(QSharedPointer<QByteArray>, buffer),
                                  Q_ARG(network::SliceVecMeta, buckets[i]));
    }
}

inline bool PacketBuffer::hasParserForType(EventPacketType type) const
{
    if (m_parserPools.contains(type))
        return true;
    for (auto it = m_parserPairPools.cbegin(); it != m_parserPairPools.cend(); ++it)
    {
        if (it->first.first == type || it->first.second == type)
            return true;
    }
    return false;
}

inline bool PacketBuffer::hasParserPair(EventPacketType infoType, EventPacketType waveType) const
{
    return m_parserPairPools.contains(ParserPairKey(infoType, waveType));
}

inline std::optional<ParserPairKey> PacketBuffer::getParserPairForType(EventPacketType type) const
{
    for (auto it = m_parserPairPools.cbegin(); it != m_parserPairPools.cend(); ++it)
    {
        if (it->first.first == type || it->first.second == type)
            return it->first;
    }
    return std::nullopt;
}

inline void PacketBuffer::dispatchToPairWorker(EventPacketType infoType, EventPacketType waveType, const QSharedPointer<QByteArray> &buffer, int infoOffset,
                                               int infoLength, int waveOffset, int waveLength) const
{
    const auto key = ParserPairKey(infoType, waveType);
    const auto poolIt = m_parserPairPools.find(key);
    if (poolIt == m_parserPairPools.end() || poolIt->second.workers.empty())
    {
        qWarning() << "Parser pair pool has no entry for" << static_cast<int>(infoType) << static_cast<int>(waveType);
        return;
    }

    auto &pool = poolIt->second;
    const auto idx = pool.nextIndex % static_cast<int>(pool.workers.size());
    pool.nextIndex = (pool.nextIndex + 1) % static_cast<int>(pool.workers.size());
    auto *worker = static_cast<PacketParserWorkerBase *>(pool.workers[idx]);
    if (!worker)
    {
        qWarning() << "Parser pair worker is null";
        return;
    }

    QMetaObject::invokeMethod(worker, "enqueuePairJob", Qt::QueuedConnection, Q_ARG(QSharedPointer<QByteArray>, buffer), Q_ARG(int, infoOffset),
                              Q_ARG(int, infoLength), Q_ARG(int, waveOffset), Q_ARG(int, waveLength));
}

inline void PacketBuffer::dispatchToPairWorkerSingle(EventPacketType infoType, EventPacketType waveType, const QSharedPointer<QByteArray> &buffer, int offset,
                                                     int length, bool isInfo) const
{
    const auto key = ParserPairKey(infoType, waveType);
    const auto poolIt = m_parserPairPools.find(key);
    if (poolIt == m_parserPairPools.end() || poolIt->second.workers.empty())
    {
        qWarning() << "Parser pair pool has no entry for" << static_cast<int>(infoType) << static_cast<int>(waveType);
        return;
    }

    auto &pool = poolIt->second;
    const auto idx = pool.nextIndex % static_cast<int>(pool.workers.size());
    pool.nextIndex = (pool.nextIndex + 1) % static_cast<int>(pool.workers.size());
    auto *worker = static_cast<PacketParserWorkerBase *>(pool.workers[idx]);
    if (!worker)
    {
        qWarning() << "Parser pair worker is null";
        return;
    }

    QMetaObject::invokeMethod(worker, "enqueueSingleJob", Qt::QueuedConnection, Q_ARG(QSharedPointer<QByteArray>, buffer), Q_ARG(int, offset),
                              Q_ARG(int, length), Q_ARG(bool, isInfo));
}

} // namespace network