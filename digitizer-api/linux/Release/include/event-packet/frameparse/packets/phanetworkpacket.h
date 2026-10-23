#pragma once

#include "packets/eventpackettype.h"

#include <QObject>
#include <QByteArrayView>

#include <cstring>

namespace network
{

struct PhaNetworkPacket
{
    static size_t size()
    {
        return 56;
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
    qint64 trapBaseline{};
    qint64 trapHeightMean{};
    qint64 trapHeightMax{};
    quint32 eventCounter{};
    qint16 rcCr2Y1{};
    qint16 rcCr2Y2{};
    quint16 reserved[3]{};
    quint16 checksum{};
};

} // namespace network
