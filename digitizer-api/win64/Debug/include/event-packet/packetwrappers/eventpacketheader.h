#pragma once

#include "packets/eventpackettype.h"

#include <QObject>

namespace network
{
struct EventPacketHeader {

    quint32 deviceId;
    EventPacketType packetType;
    quint8 flags;
    quint16 channelId;
    quint64 rtc;

    friend QDataStream &operator<<(QDataStream &out, const EventPacketHeader &header)
    {
        out << header.deviceId
            << header.packetType
            << header.flags
            << header.channelId
            << header.rtc;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, EventPacketHeader &header)
    {
        in >> header.deviceId
           >> header.packetType
           >> header.flags
           >> header.channelId
           >> header.rtc;
        return in;
    }

    bool operator==(const EventPacketHeader &other) const
    {
        return deviceId == other.deviceId &&
               packetType == other.packetType &&
               flags == other.flags &&
               channelId == other.channelId &&
               rtc == other.rtc;
    }

    bool operator!=(const EventPacketHeader &other) const
    {
        return !(*this == other);
    }
};

} // namespace network
