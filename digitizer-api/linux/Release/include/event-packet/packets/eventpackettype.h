#pragma once

#include <QObject>

namespace network
{
enum class EventPacketType : quint8 {
    InterleavedWaveform = 0,
    PsdEventInfo = 1,
    PsdWaveform = 2,
    PhaEventInfo = 3,
    PhaWaveform = 4,
    Detectron2DData = 5,
    DetectronStatisticData = 6,
    SplitUpWaveform = 7,
    ConsistentChannelSpectrum16 = 8,
    ConsistentChannelSpectrum32 = 9,
    InvalidEventInfo = 255
};
} // namespace network
