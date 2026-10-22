#pragma once

#include "model/payload_arena.h"
#include "frameparse/packets/eventpackettype.h"

#include <QMetaType>

#include <vector>

namespace network
{

enum class EventKind : quint8
{
    Info,
    Waveform,
    Spectrum,
    Other
};

struct ParsedEvent
{
    quint64 rtc{};
    quint16 channelId{};
    EventKind kind{EventKind::Other};
    EventPacketType packetType{EventPacketType::InvalidEventInfo};
    PayloadRef payload{};
    quint64 seq{0};
};

using ParsedEventList = std::vector<ParsedEvent>;

} // namespace network

Q_DECLARE_METATYPE(network::ParsedEventList)
