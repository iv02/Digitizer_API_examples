#pragma once

#include "packets/eventpackettype.h"

#include <QObject>

namespace network
{

/*
 * Binary format ConsistentChannelSpectrum32
 *
 * 4 byte - deviceId - Digitizer ID (Spacer)
 * 1 byte - packetType - enum class EventPacketType : quint8
 * 1 byte - flags - service information
 * 2 byte - channelId - For Spectrum bitmask of channels
 * 8 byte - rtc - timestamp

 * 4 byte - length (array size)
 * 2 byte - spectrumType - enum class SpectrumType : quint16
 * 2 byte - padding length
 * 4 byte[length] - spectrum array
 * 2 byte - checksum
 * 2 byte[paddingLength] - padding to align by 8 byte
 *
 * TotalSize - multiple of 8 bytes (uint64_t)
 */

struct ConsistentChannelSpectrum32
{
    QDataStream &deserialize(QDataStream &in)
    {
        in >> deviceId >> packetType >> flags >> channelId >> rtc

            >> arrayLength >> spectrumType >> paddingLength;

        for (quint32 i = 0; i < arrayLength; ++i)
        {
            qint32 value;
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
        return offsetof(ConsistentChannelSpectrum32, arrayLength);
    }

    static quint32 paddingLengthOffset()
    {
        return offsetof(ConsistentChannelSpectrum32, paddingLength);
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
