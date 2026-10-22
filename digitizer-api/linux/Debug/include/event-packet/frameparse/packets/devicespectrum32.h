#pragma once

#include "packets/eventpackettype.h"

#include <QObject>
#include <vector>
#include <QByteArrayView>

#include <cstring>

namespace network
{

struct DeviceSpectrum32
{
    void deserialize(const QByteArrayView &in)
    {
        const char *data = in.constData();
        std::memcpy(&deviceId, data, fixedPartSize());

        int offset = static_cast<int>(fixedPartSize());
        if (arrayLength > 0)
        {
            array.resize(arrayLength);
            const auto bytesToCopy = static_cast<size_t>(arrayLength) * sizeof(qint32);
            std::memcpy(array.data(), data + offset, bytesToCopy);
            offset += static_cast<int>(bytesToCopy);
        }
        else
        {
            array.clear();
        }

        std::memcpy(&checksum, data + offset, sizeof(checksum));
    }

    static quint32 arrayLengthOffset()
    {
        return offsetof(DeviceSpectrum32, arrayLength);
    }

    static quint32 paddingLengthOffset()
    {
        return offsetof(DeviceSpectrum32, paddingLength);
    }

    static quint32 fixedPartSize()
    {
        return 24;
    }

    static quint32 arrayItemSize()
    {
        return sizeof(uint32_t);
    }
    quint32 deviceId{};
    EventPacketType packetType{};
    quint8 flags{};
    quint16 channelId{};
    quint64 rtc{};
    quint32 arrayLength{};
    quint16 spectrumType{};
    quint16 paddingLength{};
    std::vector<qint32> array{};
    quint16 checksum{};
};

} // namespace network
