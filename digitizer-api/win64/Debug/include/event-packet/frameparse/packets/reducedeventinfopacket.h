#pragma once

#include "packets/eventpackettype.h"

#include <QByteArrayView>
#include <QObject>
#include <vector>

#include <cstring>

namespace network
{

struct ReducedEventInfoPacket
{
    void deserialize(const QByteArrayView &in)
    {
        const char *data = in.constData();
        int offset = static_cast<int>(fixedPartSize());

        std::memcpy(&deviceId, data, offset);

        if (arrayLength > 0)
        {
            array.resize(arrayLength);
            const auto bytesToCopy = static_cast<size_t>(arrayLength) * sizeof(qint16);
            std::memcpy(array.data(), data + offset, bytesToCopy);
            offset += static_cast<int>(bytesToCopy);
        }

        std::memcpy(&checksum, data + offset, sizeof(checksum));
    }

    static quint32 arrayLengthOffset()
    {
        return offsetof(ReducedEventInfoPacket, arrayLength);
    }

    static quint32 paddingLengthOffset()
    {
        return offsetof(ReducedEventInfoPacket, paddingLength);
    }

    static quint32 fixedPartSize()
    {
        return 24;
    }

    static quint32 arrayItemSize()
    {
        return sizeof(uint16_t);
    }
    quint32 deviceId{};
    EventPacketType packetType{};
    quint8 flags{};
    quint16 channelId{};
    quint64 rtc{};
    quint32 arrayLength{};
    quint16 partSize{};
    quint16 paddingLength{};
    std::vector<qint16> array{};
    quint16 checksum{};
};

} // namespace network
