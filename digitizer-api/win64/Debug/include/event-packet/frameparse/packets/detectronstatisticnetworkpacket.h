#pragma once

#include "packets/eventpackettype.h"

#include <QByteArrayView>
#include <QObject>

#include <cstring>

namespace network
{

/*
 * Binary format DetectronStatisticNetworkPacket
 *
 * 4 byte - deviceId - Digitizer ID (Spacer)
 * 1 byte - packetType - Type packet
 * 1 byte - flags unused
 * 2 byte - channelId unused
 *
 * 4 byte anodeTriggers;
 * 4 byte anodeProcessed;
 *
 * 4 byte x1Triggers;
 * 4 byte x1Processed;
 *
 * 4 byte x2Triggers;
 * 4 byte x2Processed;
 *
 * 4 byte y1Triggers;
 * 4 byte y1Processed;
 *
 * 4 byte y2Triggers;
 * 4 byte y2Processed;
 *
 * 4 byte - cntMonitor
 * 2 byte - padding
 * 2 byte - checksum
 *
 * TotalSize - 56 byte
 */

struct DetectronStatisticNetworkPacket
{
    static size_t size()
    {
        return 56;
    }

    void deserialize(const QByteArrayView &in)
    {
        std::memcpy(this, in.constData(), size());
    }

    //[HEADER]
    quint32 deviceId{};
    EventPacketType packetType{};
    quint8 flags{};
    quint16 channelId{};
    //[BODY]

    quint32 anodeTriggers{};
    quint32 anodeProcessed{};

    quint32 x1Triggers{};
    quint32 x1Processed{};

    quint32 x2Triggers{};
    quint32 x2Processed{};

    quint32 y1Triggers{};
    quint32 y1Processed{};

    quint32 y2Triggers{};
    quint32 y2Processed{};

    quint32 cntMonitor{};
    quint16 padding{};
    quint16 checksum{};
};

} // namespace network
