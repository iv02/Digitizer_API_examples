#pragma once

#include "packets/eventpackettype.h"

#include <QByteArrayView>
#include <QObject>

#include <cstring>

namespace network
{

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
    quint32 deviceId{};
    EventPacketType packetType{};
    quint8 flags{};
    quint16 channelId{};
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
