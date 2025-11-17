#pragma once

#include "packets/eventpackettype.h"

#include <QObject>

namespace network
{

/*
 * Binary format WaveformEventPacket
 *
 * 4 byte - deviceId - Digitizer ID (Spacer)
 * 1 byte - packetType - Type packet depends of FPGA firmware type
 * 1 byte - flags - service information (for PSD - event overlaps)
 * 2 byte - channelId - For PSD/PHA is ID of channel, for Waveform bitmask of channels
 * 8 byte - rtcChopper - timestamp
 *
 * 4 byte - channelNum
 * 2 byte - amp1
 * 2 byte - amp2
 * 8 byte - rtc
 * ... many times ...
 *
 * 6 byte receivedSignature
 * 2 byte - checksum
 *
 * TotalSize - multiple of 8 bytes (uint64_t)
 */

struct event_info_detectron_xy_t
{
    quint32 channelNum{};
    qint16 amp1{};
    qint16 amp2{};
    quint64 rtc{};

    friend QDataStream &operator>>(QDataStream &in, event_info_detectron_xy_t &xy)
    {
        in >> xy.channelNum >> xy.amp1 >> xy.amp2 >> xy.rtc;
        return in;
    }
};

struct Detectron2dNetworkPacket
{
    QDataStream &deserialize(QDataStream &in, size_t count)
    {
        in >> deviceId >> packetType >> flags >> channelId >> rtcChopper;

        for (auto i = 0; i < count; ++i)
        {
            event_info_detectron_xy_t xy;
            in >> xy;
            data.push_back(xy);
        }

        in >> receivedSignature[0] >> receivedSignature[1] >> receivedSignature[2];
        in >> checksum;

        return in;
    }

    static quint32 fixedPartSize()
    {
        return 16;
    }

    static quint32 arrayPartSize()
    {
        return 16;
    }

    static quint32 arrayLimit()
    {
        return 64;
    }

    static QByteArray signature()
    {
        return QByteArray::fromHex(QString("11D0E1FEADDE").toUtf8());
    }

    //[HEADER]
    quint32 deviceId{};
    EventPacketType packetType{};
    quint8 flags{};
    quint16 channelId{};
    quint64 rtcChopper{};

    //[BODY]
    std::vector<event_info_detectron_xy_t> data;

    //[END]
    quint16 receivedSignature[3];
    quint16 checksum{};
};

} // namespace network
