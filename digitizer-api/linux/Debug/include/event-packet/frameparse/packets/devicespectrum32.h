#pragma once

#include "packets/eventpackettype.h"

#include <QObject>
#include <vector>
#include <QByteArrayView>

#include <cstring>

namespace network
{

/*
 * Binary format DeviceSpectrum32
 *
 * 4 byte - deviceId - Digitizer ID (Spacer)
 * 1 byte - packetType - enum class EventPacketType : quint8
 * 1 byte - flags - service information
 * 2 byte - channelId - Channel ID
 * 8 byte - rtc - timestamp
 *
 * 4 byte - length (array size)
 * 2 byte - spectrumType - enum class SpectrumType : quint16
 * 2 byte - padding length
 * 4 byte[length] - spectrum array
 * 2 byte - checksum
 * 2 byte[paddingLength] - padding to align by 8 byte
 *
 * TotalSize - multiple of 8 bytes (uint64_t)
 */

struct DeviceSpectrum32
{
    void deserialize(const QByteArrayView &in)
    {
        const char *data = in.constData();
        int offset = static_cast<int>(fixedPartSize());

        std::memcpy(&deviceId, data, offset);

        if (arrayLength > 0)
        {
            array.resize(arrayLength);
            const auto bytesToCopy = static_cast<size_t>(arrayLength) * sizeof(qint32);
            std::memcpy(array.data(), data + offset, bytesToCopy);
            offset += static_cast<int>(bytesToCopy);
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

    //[HEADER]
    quint32 deviceId{};
    EventPacketType packetType{};
    quint8 flags{};
    quint16 channelId{};
    quint64 rtc{};
    //[BODY]
    quint32 arrayLength{};
    quint16 spectrumType{};
    quint16 paddingLength{};
    std::vector<qint32> array{};
    quint16 checksum{};
};

} // namespace network
