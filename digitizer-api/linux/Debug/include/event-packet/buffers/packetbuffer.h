#pragma once

#include "packetparser.h"

#include "wavewaveformseparator.h"

#include "packets/consistentchannelspectrum16.h"
#include "packets/consistentchannelspectrum32.h"
#include "packets/detectron2dnetworkpacket.h"
#include "packets/detectronstatisticnetworkpacket.h"
#include "packets/eventpackettype.h"
#include "packets/phanetworkpacket.h"
#include "packets/psdnetworkpacket.h"
#include "packets/waveformnetworkpacket.h"

#include <QObject>
#include <any>

namespace network
{
class SplitUpPacketAssembler;

class PacketBuffer final : public QObject
{
    Q_OBJECT
  signals:
    void packetParsed(const std::any &packet) const;
    void packetParsedRaw(EventPacketType type, const QByteArray &packet) const;

  public:
    explicit PacketBuffer(quint32 deviceId, QObject *parent = nullptr);

    PacketBuffer(const PacketBuffer &other) = delete;
    PacketBuffer(PacketBuffer &&other) = delete;
    PacketBuffer &operator=(const PacketBuffer &other) = delete;
    PacketBuffer &operator=(PacketBuffer &&other) = delete;

    ~PacketBuffer() override;

    template <typename T> void addParser(PacketParser<T> *parser)
    {
        parser->setDeviceId(m_deviceId);
        m_parsers.emplace(parser->packetType(), parser);
    }

    template <typename T> std::optional<EventError> processParserResult(std::expected<std::pair<T, QByteArray>, EventError> result) const
    {
        if (!result.has_value())
            return result.error();

        emit packetParsed(result.value().first);

        constexpr std::optional<EventPacketType> packetType = [] -> std::optional<EventPacketType> {
            if constexpr (std::is_same_v<T, PsdNetworkPacket>)
                return EventPacketType::PsdEventInfo;
            if constexpr (std::is_same_v<T, PhaNetworkPacket>)
                return EventPacketType::PhaEventInfo;
            if constexpr (std::is_same_v<T, Detectron2dNetworkPacket>)
                return EventPacketType::Detectron2DData;
            if constexpr (std::is_same_v<T, DetectronStatisticNetworkPacket>)
                return EventPacketType::DetectronStatisticData;
            if constexpr (std::is_same_v<T, ConsistentChannelSpectrum16>)
                return EventPacketType::ConsistentChannelSpectrum16;
            if constexpr (std::is_same_v<T, ConsistentChannelSpectrum32>)
                return EventPacketType::ConsistentChannelSpectrum32;

            return std::nullopt;
        }();

        if constexpr (std::is_same_v<T, WaveformNetworkPacket>)
        {
            if (result.value().first.packetType == EventPacketType::PsdWaveform)
                emit packetParsedRaw(EventPacketType::PsdWaveform, result.value().second);

            if (result.value().first.packetType == EventPacketType::PhaWaveform)

                emit packetParsedRaw(EventPacketType::PhaWaveform, result.value().second);
            return std::nullopt;
        }

        if constexpr (packetType.has_value())
            emit packetParsedRaw(*packetType, result.value().second);

        return std::nullopt;
    }

    template <typename T> std::optional<EventError> parseAndProcess(QTcpSocket *socket, EventPacketType packetType) const
    {
        auto parser = static_cast<PacketParser<T> *>(m_parsers.at(packetType));
        const auto result = parser->parsePacket(socket);

        if constexpr (std::is_same_v<T, WaveformNetworkPacket>)
        {
            if (packetType == EventPacketType::SplitUpWaveform)
                return processParserSplitUpResult(result);
            if (packetType == EventPacketType::InterleavedWaveform)
                return processParserInterleavedResult(result);

            return processParserResult(result);
        }

        if constexpr (ConsistentChannelSpectrumType<T>)
            return processParserConsistentResult(result);

        return processParserResult(result);
    }

    template <ConsistentChannelSpectrumType T>
    std::optional<EventError> processParserConsistentResult(std::expected<std::pair<T, QByteArray>, EventError> result) const
    {
        if (!result.has_value())
            return result.error();

        const auto separatedPackets = WaveWaveformSeparator::separateConsistentChannels(result.value().first);
        for (const auto &waveform : separatedPackets)
        {
            emit packetParsed(waveform);

            if constexpr (std::is_same_v<T, ConsistentChannelSpectrum16>)
                emit packetParsedRaw(EventPacketType::ConsistentChannelSpectrum16, result.value().second);

            if constexpr (std::is_same_v<T, ConsistentChannelSpectrum32>)
                emit packetParsedRaw(EventPacketType::ConsistentChannelSpectrum32, result.value().second);
        }

        return std::nullopt;
    }

    std::optional<EventError> processParserSplitUpResult(std::expected<std::pair<WaveformNetworkPacket, QByteArray>, EventError> result) const;
    std::optional<EventError> processParserInterleavedResult(std::expected<std::pair<WaveformNetworkPacket, QByteArray>, EventError> result) const;

    void processData(QTcpSocket *socket) const;

  private:
    void flushBrokenData(QTcpSocket *socket) const;

  private:
    SplitUpPacketAssembler *m_splitUpPacketAssembler{nullptr};
    quint32 m_deviceId{};
    std::map<EventPacketType, void *> m_parsers;
};

} // namespace network