#pragma once

#include "packets/eventpackettype.h"

#include <QObject>

namespace network
{

/*
 * Binary format PsdEventPacket
 *
 * 4 byte - deviceId - Digitizer ID (Spacer)
 * 1 byte - packetType - Type packet depends, of FPGA firmware type
 * 1 byte - flags - service information (for PSD - event overlaps)
 * 2 byte - channelId - For PSD/PHA is ID of channel, for Waveform bitmask of channels
 * 8 byte - rtc - timestamp
 *
 * 4 byte - qShort
 * 4 byte - qLong
 * 2 byte - cfdY1
 * 2 byte - cfdY2
 * 2 byte - baseline
 * 2 byte - height
 * 4 byte - eventCounter
 * 4 byte - eventCounterPsd
 * 2 byte - psdValue
 * 4 byte - reserved[2]
 * 2 byte - checksum
 *
 * TotalSize - 48 bytes
 */

struct PsdNetworkPacket
{
    static size_t size()
    {
        return 48;
    }

    QDataStream &deserialize(QDataStream &in)
    {
        in >> deviceId >> packetType >> flags >> channelId >> rtc >> qShort >> qLong >> cfdY1 >> cfdY2 >> baseline >> height >> eventCounter >>
            eventCounterPsd >> psdValue;

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
    qint32 qShort{};
    qint32 qLong{};
    qint16 cfdY1{};
    qint16 cfdY2{};
    qint16 baseline{};
    qint16 height{};
    quint32 eventCounter{};
    quint32 eventCounterPsd{};
    qint16 psdValue{};
    quint16 reserved[2]{};
    quint16 checksum{};
};

} // namespace network
