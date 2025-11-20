#pragma once

#include "packetwrappers/eventpacket.h"

#include <QSharedPointer>

namespace network
{

struct EventData {
    QSharedPointer<EventPacket> infoPacket;
    QSharedPointer<EventPacket> waveformPacket;
};

} // namespace network

