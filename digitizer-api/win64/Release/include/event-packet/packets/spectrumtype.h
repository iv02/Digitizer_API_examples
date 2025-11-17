#pragma once

#include <QObject>

namespace network
{
enum class SpectrumType : quint16
{
    PSDHeight = 0,
    PSDQLong = 1,
    PHAMean = 2,
    PHAMax = 3,
    InvalidSpectrum = 255
};
} // namespace network