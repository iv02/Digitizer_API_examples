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
    DeviceSpectrum16 = 8,
    DeviceSpectrum32 = 9,
    PsdEventInfoV2 = 10,
    InvalidEventInfo = 255
};
} // namespace network
