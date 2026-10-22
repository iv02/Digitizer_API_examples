#pragma once

#include "packets/eventpackettype.h"

#include <QObject>
#include <QByteArrayView>

#include <cstring>

namespace network
{

struct PsdNetworkPacketV2
{
    static size_t size()
    {
        return 48;
    }

    void deserialize(const QByteArrayView &in)
    {
        std::memcpy(this, in.constData(), size());
    }
    quint32 deviceId{};
    EventPacketType packetType{};
    quint8 flags{};
    quint16 channelId{};
    quint64 rtc{};
    qint32 qShort{};
    qint32 qLong{};
    qint16 cfdY1{};
    qint16 cfdY2{};
    qint16 baseline{};
    qint16 height{};
    quint32 eventCounter{};
    quint32 eventCounterPsd{};
    quint16 channelIdDouble{};
    quint16 spectrumBin{};
    qint16 psdValue{};
    quint16 checksum{};
};

} // namespace network
