#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QtGlobal>

#include <vector>

namespace network
{

struct EventGroup
{
    quint64 rtc{};
    quint16 channelId{};
    QByteArray waveform;
    std::vector<QByteArray> infos;
};

} // namespace network

Q_DECLARE_METATYPE(std::vector<network::EventGroup>)
