#pragma once

#include "packets/eventpackettype.h"

#include <QObject>

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
    QDataStream &deserialize(QDataStream &in)
    {
        in >> deviceId >> packetType >> flags >> channelId >> rtc

            >> arrayLength >> decimationFactor >> paddingLength;

        for (quint32 i = 0; i < arrayLength; ++i)
        {
            qint16 value;
            in >> value;
            array.push_back(value);
        }

        in >> checksum;

        for (quint16 i = 0; i < paddingLength; ++i)
        {
            qint16 padding;
            in >> padding;
        }

        return in;
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
