#pragma once

#include "packets/eventpackettype.h"

#include <QObject>

namespace network
{

/*
 * Binary format PhaEventPacket
 *
 * 4 byte - deviceId - Digitizer ID (Spacer)
 * 1 byte - packetType - Type packet depends, of FPGA firmware type
 * 1 byte - flags - service information (for PSD - event overlaps)
 * 2 byte - channelId - For PSD/PHA is ID of channel, for Waveform bitmask of channels
 * 8 byte - rtc - timestamp
 *
 * 8 byte - trapBaseline
 * 8 byte - trapHeightMean
 * 8 byte - trapHeightMax
 * 4 byte - eventCounter
 * 2 byte - rcCr2Y1
 * 2 byte - rcCr2Y2
 * 6 byte - padding
 * 2 byte - checksum
 *
 * TotalSize - 56 bytes
 */

struct PhaNetworkPacket
{
    static size_t size()
    {
        return 56;
    }

    QDataStream &deserialize(QDataStream &in)
    {
        in >> deviceId >> packetType >> flags >> channelId >> rtc >> trapBaseline >> trapHeightMean >> trapHeightMax >> eventCounter >> rcCr2Y1 >> rcCr2Y2;

        for (auto &res : reserved)
            in >> res;

        in >> checksum;

        return in;
    }

    //[HEADER]
    quint32 deviceId{};
    EventPacketType packetType{};
    quint8 flags{};
    quint16 channelId{};
    quint64 rtc{};
    //[BODY]
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
