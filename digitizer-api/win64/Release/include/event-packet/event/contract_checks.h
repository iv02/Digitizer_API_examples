#pragma once

#include "frameparse/packets/eventpackettype.h"
#include "frameparse/packets/spectrumtype.h"
#include "model/parsed_event.h"
#include "model/payload_arena.h"
#include "parsing_mode.h"

#include <QDebug>

#include <cstring>
#include <map>
#include <optional>

namespace network
{

template <ParsingMode Mode> class ContractChecks;

template <> class ContractChecks<ParsingMode::Normal>
{
  public:
    void inspect(const ParsedEvent &, PayloadArena *) noexcept {}
    void logOrphanWaveform(quint16, quint64, EventPacketType) const noexcept {}
    void reset() noexcept {}
};

template <> class ContractChecks<ParsingMode::Paranoid>
{
  public:
    void inspect(const ParsedEvent &event, PayloadArena *arena)
    {
        std::optional<SpectrumType> spectrumType;
        if ((event.packetType == EventPacketType::DeviceSpectrum16 || event.packetType == EventPacketType::DeviceSpectrum32) && arena)
        {
            if (const char *bytes = arena->data(event.payload); bytes && event.payload.size >= 22)
            {
                quint16 raw{};
                std::memcpy(&raw, bytes + 20, sizeof(raw));
                spectrumType = static_cast<SpectrumType>(raw);
            }
        }

        if (event.packetType == EventPacketType::DeviceSpectrum16 || event.packetType == EventPacketType::DeviceSpectrum32)
        {
            if (!spectrumType.has_value())
            {
                qWarning() << "[PARANOID][Contract] Spectrum packet without spectrumType metadata. packetType:"
                           << static_cast<int>(event.packetType);
            }
            else if (*spectrumType != SpectrumType::PSDHeight && *spectrumType != SpectrumType::PSDQLong &&
                     *spectrumType != SpectrumType::PHAMean && *spectrumType != SpectrumType::PHAMax)
            {
                qWarning() << "[PARANOID][Contract] Invalid spectrumType value:" << static_cast<int>(*spectrumType)
                           << "packetType:" << static_cast<int>(event.packetType);
            }
        }

        const StreamMode packetMode = streamModeFor(event.packetType, spectrumType);
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
                           << "actualFromPacket:" << toString(packetMode) << "packetType:" << static_cast<int>(event.packetType);
            }
        }

        const bool hasHeader = event.packetType != EventPacketType::DetectronStatisticData && event.packetType != EventPacketType::Detectron2DData;
        if (!hasHeader || !isAlternatingInfoWaveformPacket(event.packetType))
            return;

        ChannelState &state = m_channelStates[event.channelId];
        if (packetMode != StreamMode::Unknown)
        {
            if (state.mode == StreamMode::Unknown)
                state.mode = packetMode;
            else if (state.mode != packetMode)
            {
                qWarning() << "[PARANOID][Contract] Channel mode deviation. channel:" << event.channelId << "expected:" << toString(state.mode)
                           << "actual:" << toString(packetMode) << "packetType:" << static_cast<int>(event.packetType);
            }
        }

        if ((state.mode == StreamMode::Psd && isPhaRelated(event.packetType)) || (state.mode == StreamMode::Pha && isPsdRelated(event.packetType)))
        {
            qWarning() << "[PARANOID][Contract] Packet family mismatch with channel mode. channel:" << event.channelId
                       << "mode:" << toString(state.mode) << "packetType:" << static_cast<int>(event.packetType);
        }

        const bool isWaveform = isPsdWaveform(event.packetType) || isPhaWaveform(event.packetType);
        if (isWaveform && state.lastPacketType.has_value() && *state.lastPacketType == event.packetType)
        {
            qWarning() << "[PARANOID][Contract] Duplicate packet type for channel (two identical waveform packets in a row)."
                       << "channel:" << event.channelId << "packetType:" << static_cast<int>(event.packetType);
        }
        state.lastPacketType = event.packetType;
    }

    void logOrphanWaveform(quint16 channelId, quint64 rtc, EventPacketType packetType) const
    {
        if (packetType == EventPacketType::InterleavedWaveform || packetType == EventPacketType::SplitUpWaveform)
            return;
        qInfo() << "[PARANOID][Contract] Orphan waveform packet without info. channel:" << channelId << "rtc:" << rtc
                << "packetType:" << static_cast<int>(packetType);
    }

    void reset() noexcept
    {
        m_channelStates.clear();
        m_inferredMode = StreamMode::Unknown;
    }

  private:
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
    };

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

    std::map<quint16, ChannelState> m_channelStates;
    StreamMode m_inferredMode{StreamMode::Unknown};
};

} // namespace network
