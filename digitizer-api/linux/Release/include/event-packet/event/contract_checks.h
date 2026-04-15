#pragma once

#include "../parsing_mode.h"
#include "frameparse/packetwrappers/eventpacket.h"
#include "frameparse/packetwrappers/eventpacketheader.h"
#include "frameparse/packets/eventpackettype.h"
#include "frameparse/packets/spectrumtype.h"
#include "frameparse/parsed_packet.h"

#include <QDebug>

#include <map>
#include <optional>
#include <type_traits>

namespace network
{

template <ParsingMode Mode> class ContractChecks;

template <> class ContractChecks<ParsingMode::Normal>
{
  public:
    void reset() noexcept {}
    void inspect(const ParsedPacket &, EventPacketType) noexcept {}
    void logOrphanInfoPacket(const QSharedPointer<EventPacket> &) const noexcept {}
    void logOrphanWaveformPacket(const QSharedPointer<EventPacket> &) const noexcept {}

    template <typename PacketRange> void logOrphanInfoPackets(const PacketRange &) const noexcept {}
    template <typename PacketRange> void logOrphanWaveformPackets(const PacketRange &) const noexcept {}
};

template <> class ContractChecks<ParsingMode::Paranoid>
{
  public:
    void reset()
    {
        m_channelStates.clear();
        m_inferredMode = StreamMode::Unknown;
    }

    void inspect(const ParsedPacket &packet, EventPacketType packetType)
    {
        const ContractContext context = makeContractContext(packet);

        if (packetType == EventPacketType::DeviceSpectrum16 || packetType == EventPacketType::DeviceSpectrum32)
        {
            if (!context.spectrumType.has_value())
            {
                qWarning() << "[PARANOID][Contract] Spectrum packet without spectrumType metadata. packetType:"
                           << static_cast<int>(packetType);
            }
            else if (*context.spectrumType != SpectrumType::PSDHeight && *context.spectrumType != SpectrumType::PSDQLong &&
                     *context.spectrumType != SpectrumType::PHAMean && *context.spectrumType != SpectrumType::PHAMax)
            {
                qWarning() << "[PARANOID][Contract] Invalid spectrumType value:" << static_cast<int>(*context.spectrumType)
                           << "packetType:" << static_cast<int>(packetType);
            }
        }

        const StreamMode packetMode = streamModeFor(packetType, context.spectrumType);
        if (packetMode != StreamMode::Unknown)
        {
            if (m_inferredMode == StreamMode::Unknown)
            {
                m_inferredMode = packetMode;
                qInfo() << "[PARANOID][Contract] Inferred stream mode:" << toString(m_inferredMode);
            }
            else if (m_inferredMode != packetMode)
            {
                qWarning() << "[PARANOID][Contract] Stream mode deviation. expected:" << toString(m_inferredMode)
                           << "actualFromPacket:" << toString(packetMode) << "packetType:" << static_cast<int>(packetType);
            }
        }

        if (!context.hasHeader || !isAlternatingInfoWaveformPacket(packetType))
            return;

        ChannelState &state = m_channelStates[context.channelId];
        if (packetMode != StreamMode::Unknown)
        {
            if (state.mode == StreamMode::Unknown)
                state.mode = packetMode;
            else if (state.mode != packetMode)
            {
                qWarning() << "[PARANOID][Contract] Channel mode deviation. channel:" << context.channelId
                           << "expected:" << toString(state.mode) << "actual:" << toString(packetMode)
                           << "packetType:" << static_cast<int>(packetType);
            }
        }

        if ((state.mode == StreamMode::Psd && isPhaRelated(packetType)) || (state.mode == StreamMode::Pha && isPsdRelated(packetType)))
        {
                qWarning() << "[PARANOID][Contract] Packet family mismatch with channel mode. channel:" << context.channelId
                       << "mode:" << toString(state.mode) << "packetType:" << static_cast<int>(packetType);
        }

        if (isPsdWaveform(packetType) || isPhaWaveform(packetType))
            state.hasSeenWaveformOnChannel = true;

        if (state.hasSeenWaveformOnChannel && state.lastPacketType.has_value() && *state.lastPacketType == packetType)
        {
            qWarning() << "[PARANOID][Contract] Duplicate packet type for channel (protocol.h: PSD/PHA + Waveform, no two identical types in a row)."
                       << "channel:" << context.channelId << "packetType:" << static_cast<int>(packetType);
        }
        state.lastPacketType = packetType;
    }

    void logOrphanInfoPacket(const QSharedPointer<EventPacket> &infoPacket) const
    {
        if (!infoPacket)
            return;

        const EventPacketHeader header = infoPacket->header();
        if (!shouldLogOrphanInfo(header.packetType))
            return;

        qInfo() << "[PARANOID][Contract] Orphan info packet without waveform. channel:" << header.channelId << "rtc:" << header.rtc
                << "packetType:" << static_cast<int>(header.packetType);
    }

    void logOrphanWaveformPacket(const QSharedPointer<EventPacket> &waveformPacket) const
    {
        if (!waveformPacket)
            return;

        const EventPacketHeader header = waveformPacket->header();
        if (!shouldLogOrphanWaveform(header.packetType))
            return;

        qInfo() << "[PARANOID][Contract] Orphan waveform packet without info. channel:" << header.channelId << "rtc:" << header.rtc
                << "packetType:" << static_cast<int>(header.packetType);
    }

    template <typename PacketRange> void logOrphanInfoPackets(const PacketRange &pendingInfo) const
    {
        for (const auto &infoPacket : pendingInfo)
        {
            if (!infoPacket)
                continue;

            const EventPacketHeader header = infoPacket->header();
            if (!shouldLogOrphanInfo(header.packetType))
                continue;

            qInfo() << "[PARANOID][Contract] Orphan info packet without waveform. channel:" << header.channelId << "rtc:" << header.rtc
                    << "packetType:" << static_cast<int>(header.packetType);
        }
    }

    template <typename PacketRange> void logOrphanWaveformPackets(const PacketRange &pendingWaveform) const
    {
        for (const auto &waveformPacket : pendingWaveform)
        {
            if (!waveformPacket)
                continue;
            const EventPacketHeader header = waveformPacket->header();
            if (!shouldLogOrphanWaveform(header.packetType))
                continue;
            qInfo() << "[PARANOID][Contract] Orphan waveform packet without info. channel:" << header.channelId << "rtc:" << header.rtc
                    << "packetType:" << static_cast<int>(header.packetType);
        }
    }

  private:
    static bool shouldLogOrphanInfo(const EventPacketType packetType)
    {
        switch (packetType)
        {
        case EventPacketType::PsdEventInfo:
        case EventPacketType::PsdEventInfoV2:
        case EventPacketType::PhaEventInfo:
        case EventPacketType::ReducedEventInfoPSD:
        case EventPacketType::ReducedEventInfoPHA:
            return false;
        default:
            return false;
        }
    }

    static bool shouldLogOrphanWaveform(const EventPacketType packetType)
    {
        return packetType != EventPacketType::InterleavedWaveform && packetType != EventPacketType::SplitUpWaveform;
    }

    enum class StreamMode
    {
        Unknown,
        Psd,
        Pha,
        Detectron,
        Waveform
    };

    struct ChannelState
    {
        StreamMode mode{StreamMode::Unknown};
        std::optional<EventPacketType> lastPacketType;
        bool hasSeenWaveformOnChannel{false};
    };

    struct ContractContext
    {
        bool hasHeader{false};
        quint16 channelId{0};
        std::optional<SpectrumType> spectrumType;
    };

    static ContractContext makeContractContext(const ParsedPacket &packet)
    {
        return std::visit(
            []<typename T>(const std::shared_ptr<T> &ptr) -> ContractContext {
                if (!ptr)
                    return {};

                ContractContext ctx{};
                if constexpr (std::is_same_v<T, DetectronStatisticNetworkPacket> || std::is_same_v<T, Detectron2dNetworkPacket>)
                {
                    ctx.hasHeader = false;
                }
                else
                {
                    ctx.hasHeader = true;
                    ctx.channelId = ptr->channelId;
                }

                if constexpr (std::is_same_v<T, DeviceSpectrum16> || std::is_same_v<T, DeviceSpectrum32>)
                    ctx.spectrumType = static_cast<SpectrumType>(ptr->spectrumType);
                return ctx;
            },
            packet);
    }

    static bool isPsdInfo(const EventPacketType packetType)
    {
        return packetType == EventPacketType::PsdEventInfo || packetType == EventPacketType::PsdEventInfoV2 ||
            packetType == EventPacketType::ReducedEventInfoPSD;
    }

    static bool isPhaInfo(const EventPacketType packetType)
    {
        return packetType == EventPacketType::PhaEventInfo || packetType == EventPacketType::ReducedEventInfoPHA;
    }

    static bool isPsdWaveform(const EventPacketType packetType)
    {
        return packetType == EventPacketType::PsdWaveform;
    }

    static bool isPhaWaveform(const EventPacketType packetType)
    {
        return packetType == EventPacketType::PhaWaveform;
    }

    static bool isPsdRelated(const EventPacketType packetType)
    {
        return isPsdInfo(packetType) || isPsdWaveform(packetType);
    }

    static bool isPhaRelated(const EventPacketType packetType)
    {
        return isPhaInfo(packetType) || isPhaWaveform(packetType);
    }

    static bool isAlternatingInfoWaveformPacket(const EventPacketType packetType)
    {
        return isPsdInfo(packetType) || isPhaInfo(packetType) || isPsdWaveform(packetType) || isPhaWaveform(packetType);
    }

    static StreamMode streamModeFor(const EventPacketType packetType, const std::optional<SpectrumType> &spectrumType)
    {
        if (isPsdRelated(packetType))
            return StreamMode::Psd;
        if (isPhaRelated(packetType))
            return StreamMode::Pha;
        if (packetType == EventPacketType::InterleavedWaveform || packetType == EventPacketType::SplitUpWaveform)
            return StreamMode::Waveform;
        if (packetType == EventPacketType::Detectron2DData || packetType == EventPacketType::DetectronStatisticData)
            return StreamMode::Detectron;

        if (packetType == EventPacketType::DeviceSpectrum16 || packetType == EventPacketType::DeviceSpectrum32)
        {
            if (!spectrumType.has_value())
                return StreamMode::Unknown;
            if (*spectrumType == SpectrumType::PSDHeight || *spectrumType == SpectrumType::PSDQLong)
                return StreamMode::Psd;
            if (*spectrumType == SpectrumType::PHAMean || *spectrumType == SpectrumType::PHAMax)
                return StreamMode::Pha;
        }

        return StreamMode::Unknown;
    }

    static const char *toString(StreamMode mode)
    {
        switch (mode)
        {
        case StreamMode::Unknown:
            return "Unknown";
        case StreamMode::Psd:
            return "PSD";
        case StreamMode::Pha:
            return "PHA";
        case StreamMode::Detectron:
            return "Detectron";
        case StreamMode::Waveform:
            return "Waveform";
        }
        return "Unknown";
    }

  private:
    std::map<quint16, ChannelState> m_channelStates;
    StreamMode m_inferredMode{StreamMode::Unknown};
};

} // namespace network
