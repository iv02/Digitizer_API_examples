#pragma once

#include <QFlags>
#include <QtGlobal>

namespace client
{

enum class PhaShortField : quint16
{
    Rtc16 = static_cast<quint16>(1u << 0),
    Rtc32 = static_cast<quint16>(1u << 1),
    Rtc48 = static_cast<quint16>(1u << 2),
    Rtc64 = static_cast<quint16>(1u << 3),

    TrapBaseline = static_cast<quint16>(1u << 4),
    TrapHeightMean = static_cast<quint16>(1u << 5),
    TrapHeightMax = static_cast<quint16>(1u << 6),

    EventCounter = static_cast<quint16>(1u << 7),

    RcCr2Y1 = static_cast<quint16>(1u << 8),
    RcCr2Y2 = static_cast<quint16>(1u << 9),

    SpectrumBin = static_cast<quint16>(1u << 10),
};

Q_DECLARE_FLAGS(PhaShortFields, PhaShortField)
Q_DECLARE_OPERATORS_FOR_FLAGS(PhaShortFields)

} // namespace client

