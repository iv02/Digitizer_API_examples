#pragma once

#include "packetwrappers/eventdata.h"

#include <QByteArray>
#include <QSharedPointer>

#include <vector>

namespace network
{
struct EventGroup;
class EventPacket;

/**
 * Converts EventGroup (raw wire QByteArrays) into legacy EventData pairs
 * expected by NetworkWorker / Digitizer_SW.
 *
 * Fan-out rules:
 * - N infos + waveform  -> N x EventData{info_i, sharedWaveform}
 * - infos only          -> N x EventData{info_i, nullptr}
 * - waveform only       -> 1 x EventData{nullptr, waveform}
 * - empty / unsupported -> skipped
 */
class EventGroupConverter
{
  public:
    [[nodiscard]] static std::vector<EventData> toEventData(const EventGroup &group);
    [[nodiscard]] static std::vector<EventData> toEventData(const std::vector<EventGroup> &groups);

    [[nodiscard]] static QSharedPointer<EventPacket> packetFromBytes(const QByteArray &bytes);
};

} // namespace network
