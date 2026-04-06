#pragma once

#include "eventpacket.h"

#include <QMetaType>
#include <QSharedPointer>

#include <vector>

namespace network
{

struct EventData {
    QSharedPointer<EventPacket> infoPacket;
    QSharedPointer<EventPacket> waveformPacket;
};

} // namespace network

Q_DECLARE_METATYPE(std::vector<network::EventData>)
