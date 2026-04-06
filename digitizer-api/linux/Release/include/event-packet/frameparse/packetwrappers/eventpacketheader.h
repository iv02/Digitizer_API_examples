#pragma once

#include "packets/eventpackettype.h"

#include <QDataStream>
#include <cstring>

namespace network
{
struct EventPacketHeader {

    quint32 deviceId;
    EventPacketType packetType;
    quint8 flags;
    quint16 channelId;
    quint64 rtc;

    bool operator==(const EventPacketHeader &other) const;
    bool operator!=(const EventPacketHeader &other) const;
};

QDataStream &operator<<(QDataStream &out, const EventPacketHeader &header);
QDataStream &operator>>(QDataStream &in, EventPacketHeader &header);

template <typename T> EventPacketHeader makePacketHeader(const T &packet)
{
    EventPacketHeader header{};
    std::memcpy(&header, &packet, sizeof(EventPacketHeader));
    return header;
}

template <typename T> EventPacketHeader makePacketHeader(const T &packet, EventPacketType packetType)
{
    EventPacketHeader header = makePacketHeader(packet);
    header.packetType = packetType;
    return header;
}

} // namespace network
