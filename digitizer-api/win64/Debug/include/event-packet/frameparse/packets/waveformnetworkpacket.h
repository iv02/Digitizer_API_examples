#pragma once

#include "packets/eventpackettype.h"

#include <QByteArrayView>
#include <QObject>
#include <vector>

#include <cstring>

namespace network
{

/*
 * Binary format WaveformEventPacket
 *
 * 4 byte - deviceId - Digitizer ID (Spacer)
 * 1 byte - packetType - Type packet depends on FPGA firmware type
 * 1 byte - flags - service information (for PSD - event overlaps)
 * 2 byte - channelId - For PSD/PHA is ID of channel, for Waveform bitmask of channels
 * 8 byte - rtc - timestamp

 * 4 byte - length (array size)
 * 2 byte - decimation factor
 * 2 byte - padding length
 * 2 byte[length] - waveform array
 * 2 byte - checksum
 * 2 byte[paddingLength] - padding to align by 8 byte
 *
 * TotalSize - multiple of 8 bytes (uint64_t)
 */

struct WaveformNetworkPacket
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
        return offsetof(WaveformNetworkPacket, arrayLength);
    }

    static quint32 paddingLengthOffset()
    {
        return offsetof(WaveformNetworkPacket, paddingLength);
    }

    static quint32 fixedPartSize()
    {
        return 24;
    }

    static quint32 arrayItemSize()
    {
        return sizeof(uint16_t);
    }

    //[HEADER]
    quint32 deviceId{};
    EventPacketType packetType{};
    quint8 flags{};
    quint16 channelId{};
    quint64 rtc{};
    //[BODY]
    quint32 arrayLength{};
    quint16 decimationFactor{};
    quint16 paddingLength{};
    std::vector<qint16> array{};
    quint16 checksum{};
};

} // namespace network
