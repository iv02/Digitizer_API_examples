#pragma once

#include "eventpacketheader.h"

#include "packets/devicespectrum16.h"
#include "packets/devicespectrum32.h"
#include "packets/phanetworkpacket.h"
#include "packets/psdnetworkpacket.h"
#include "packets/psdnetworkpacketv2.h"
#include "packets/waveformnetworkpacket.h"

#include "packets/spectrumtype.h"

#include <QObject>

#include <ranges>

namespace network
{
class EventPacket : public QObject
{
    Q_OBJECT
  public:
    [[nodiscard]] EventPacketHeader header() const;
    [[nodiscard]] virtual EventPacketType type() const;

    void setHeader(const EventPacketHeader &newHeader);

    friend QDataStream &operator<<(QDataStream &out, const EventPacket &packet)
    {
        out << packet.m_header;
        packet.serialize(out);
        out << packet.m_checksum;
        packet.serializePadding(out);
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, EventPacket &packet)
    {
        in >> packet.m_header;
        packet.deserialize(in);
        in >> packet.m_checksum;
        packet.deserializePadding(in);
        return in;
    }

    bool operator==(EventPacket &other) const
    {
        if (typeid(*this) != typeid(other))
            return false;

        return m_header == other.m_header && m_checksum == other.m_checksum && compare(&other);
    }

    bool operator!=(EventPacket &other) const
    {
        return !(*this == other);
    }

  public:
    virtual void serialize(QDataStream &out) const = 0;
    virtual void serializePadding(QDataStream &out) const
    {
    }
    virtual void deserialize(QDataStream &in) = 0;
    virtual void deserializePadding(QDataStream &in)
    {
    }

  protected:
    [[nodiscard]] virtual bool compare(EventPacket *other) const = 0;

  public:
    quint16 m_checksum{};

  protected:
    EventPacketHeader m_header{};
};

class PsdEventPacket final : public EventPacket
{
    Q_OBJECT
  public:
    PsdEventPacket() = default;
    PsdEventPacket(const PsdNetworkPacket &packet)
    {
        m_header = {
            .deviceId = packet.deviceId, .packetType = EventPacketType::PsdEventInfo, .flags = packet.flags, .channelId = packet.channelId, .rtc = packet.rtc};
        m_qShort = packet.qShort;
        m_qLong = packet.qLong;
        m_cfdY1 = packet.cfdY1;
        m_cfdY2 = packet.cfdY2;
        m_baseline = packet.baseline;
        m_height = packet.height;
        m_eventCounter = packet.eventCounter;
        m_eventCounterPsd = packet.eventCounterPsd;
        m_psdValue = packet.psdValue;
    }
    PsdEventPacket(const PsdNetworkPacketV2 &packet)
    {
        m_header = {
            .deviceId = packet.deviceId, .packetType = EventPacketType::PsdEventInfo, .flags = packet.flags, .channelId = packet.channelId, .rtc = packet.rtc};
        m_qShort = packet.qShort;
        m_qLong = packet.qLong;
        m_cfdY1 = packet.cfdY1;
        m_cfdY2 = packet.cfdY2;
        m_baseline = packet.baseline;
        m_height = packet.height;
        m_eventCounter = packet.eventCounter;
        m_eventCounterPsd = packet.eventCounterPsd;
        m_psdValue = packet.psdValue;
    }
    void serialize(QDataStream &out) const override;
    void deserialize(QDataStream &in) override;

  protected:
    [[nodiscard]] bool compare(EventPacket *other) const override;

  public:
    qint32 m_qShort{};
    qint32 m_qLong{};
    qint16 m_cfdY1{};
    qint16 m_cfdY2{};
    qint16 m_baseline{};
    qint16 m_height{};
    quint32 m_eventCounter{};
    quint32 m_eventCounterPsd{};
    qint16 m_psdValue{};
    quint16 m_reserved[2]{};
};

class PhaEventPacket : public EventPacket
{
    Q_OBJECT
  public:
    PhaEventPacket() = default;
    PhaEventPacket(const PhaNetworkPacket &packet)
    {
        m_header = {.deviceId = packet.deviceId, .packetType = packet.packetType, .flags = packet.flags, .channelId = packet.channelId, .rtc = packet.rtc};
        m_trapBaseline = packet.trapBaseline;
        m_trapHeightMean = packet.trapHeightMean;
        m_trapHeightMax = packet.trapHeightMax;
        m_eventCounter = packet.eventCounter;
        m_rcCr2Y1 = packet.rcCr2Y1;
        m_rcCr2Y2 = packet.rcCr2Y2;
    }
    void serialize(QDataStream &out) const override;
    void deserialize(QDataStream &in) override;

  protected:
    [[nodiscard]] bool compare(EventPacket *other) const override;

  public:
    qint64 m_trapBaseline{};
    qint64 m_trapHeightMean{};
    qint64 m_trapHeightMax{};
    quint32 m_eventCounter{};
    qint16 m_rcCr2Y1{};
    qint16 m_rcCr2Y2{};
    quint16 m_reserved[3]{};
};

class WaveformEventPacket : public EventPacket
{
    Q_OBJECT
  public:
    WaveformEventPacket() = default;
    WaveformEventPacket(const WaveformNetworkPacket &packet)
    {
        m_header = {.deviceId = packet.deviceId, .packetType = packet.packetType, .flags = packet.flags, .channelId = packet.channelId, .rtc = packet.rtc};
        m_decimationFactor = packet.decimationFactor;
        m_paddingLength = packet.paddingLength;
        m_waveform = packet.array;
    }

    void serialize(QDataStream &out) const override;
    void serializePadding(QDataStream &out) const override;
    void deserialize(QDataStream &in) override;
    void deserializePadding(QDataStream &in) override;

  protected:
    [[nodiscard]] bool compare(EventPacket *other) const override;

  public:
    quint16 m_decimationFactor{};
    quint16 m_paddingLength{};
    std::vector<qint16> m_waveform{};
};

class SpectrumEventPacket : public EventPacket
{
    Q_OBJECT
  public:
    SpectrumEventPacket() = default;
    SpectrumEventPacket(const DeviceSpectrum16 &packet)
    {
        m_header = {.deviceId = packet.deviceId, .packetType = packet.packetType, .flags = packet.flags, .channelId = packet.channelId, .rtc = packet.rtc};
        m_spectrumType = static_cast<SpectrumType>(packet.spectrumType);
        m_paddingLength = packet.paddingLength;

        m_spectrum = std::vector<qint32>();
        m_spectrum.reserve(packet.array.size());
        std::ranges::transform(packet.array, std::back_inserter(m_spectrum), [](auto v) { return static_cast<qint32>(v); });
    }

    SpectrumEventPacket(const DeviceSpectrum32 &packet)
    {
        m_header = {.deviceId = packet.deviceId, .packetType = packet.packetType, .flags = packet.flags, .channelId = packet.channelId, .rtc = packet.rtc};
        m_spectrumType = static_cast<SpectrumType>(packet.spectrumType);
        m_paddingLength = packet.paddingLength;
        m_spectrum = packet.array;
    }

    void serialize(QDataStream &out) const override;
    void serializePadding(QDataStream &out) const override;
    void deserialize(QDataStream &in) override;
    void deserializePadding(QDataStream &in) override;

  protected:
    [[nodiscard]] bool compare(EventPacket *other) const override;

  public:
    SpectrumType m_spectrumType{};
    quint16 m_paddingLength{};
    std::vector<qint32> m_spectrum{};
};

} // namespace network
