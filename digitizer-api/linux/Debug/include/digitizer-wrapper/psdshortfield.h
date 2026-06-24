#pragma once

#include <QFlags>
#include <QtGlobal>

namespace client
{

enum class PsdShortField : quint16
{
    isChannelEnabled = static_cast<quint16>(1u << 0),

    Rtc16 = static_cast<quint16>(1u << 1),
    Rtc32 = static_cast<quint16>(1u << 2),
    Rtc48 = static_cast<quint16>(1u << 3),
    Rtc64 = static_cast<quint16>(1u << 4),

    CfdY1 = static_cast<quint16>(1u << 5),
    CfdY2 = static_cast<quint16>(1u << 6),
    Height = static_cast<quint16>(1u << 7),
    Baseline = static_cast<quint16>(1u << 8),

    QLong = static_cast<quint16>(1u << 9),
    QShort = static_cast<quint16>(1u << 10),
    PsdValue = static_cast<quint16>(1u << 11),

    EventCounter = static_cast<quint16>(1u << 12),
    EventCounterPsd = static_cast<quint16>(1u << 13),
    SpectrumBin = static_cast<quint16>(1u << 14)
};

Q_DECLARE_FLAGS(PsdShortFields, PsdShortField)
Q_DECLARE_OPERATORS_FOR_FLAGS(PsdShortFields)

} // namespace client
