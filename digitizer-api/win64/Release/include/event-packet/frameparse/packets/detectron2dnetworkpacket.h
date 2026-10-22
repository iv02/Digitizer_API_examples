#pragma once

#include "packets/eventpackettype.h"

#include <QByteArrayView>
#include <QDebug>
#include <QObject>
#include <vector>

namespace network
{

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
    void deserialize(const QByteArrayView &in, size_t count)
    {
        const char *raw = in.constData();
        int offset = static_cast<int>(fixedPartSize());

        std::memcpy(&deviceId, raw, offset);

        data.resize(count);
        if (const auto bytesToCopy = count * static_cast<size_t>(arrayPartSize()); bytesToCopy > 0)
        {
            std::memcpy(data.data(), raw + static_cast<int>(fixedPartSize()), bytesToCopy);
            offset += static_cast<int>(bytesToCopy);
        }

        std::memcpy(&receivedSignature[0], raw + offset, sizeof(receivedSignature) + sizeof(checksum));
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
    quint32 deviceId{};
    EventPacketType packetType{};
    quint8 flags{};
    quint16 channelId{};
    quint64 rtcChopper{};
    std::vector<event_info_detectron_xy_t> data;
    quint16 receivedSignature[3];
    quint16 checksum{};
};

} // namespace network
