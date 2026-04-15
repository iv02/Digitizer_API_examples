#pragma once

#include <QFlags>
#include <QtGlobal>

namespace client
{

enum class PsdShortField : quint16
{
    Rtc16 = static_cast<quint16>(1u << 0),
    Rtc32 = static_cast<quint16>(1u << 1),
    Rtc48 = static_cast<quint16>(1u << 2),
    Rtc64 = static_cast<quint16>(1u << 3),

    CfdY1 = static_cast<quint16>(1u << 4),
    CfdY2 = static_cast<quint16>(1u << 5),
    Height = static_cast<quint16>(1u << 6),
    Baseline = static_cast<quint16>(1u << 7),

    QLong = static_cast<quint16>(1u << 8),
    QShort = static_cast<quint16>(1u << 9),
    PsdValue = static_cast<quint16>(1u << 10),

    EventCounter = static_cast<quint16>(1u << 11),
    EventCounterPsd = static_cast<quint16>(1u << 12),
    SpectrumBin = static_cast<quint16>(1u << 13),
};

Q_DECLARE_FLAGS(PsdShortFields, PsdShortField)
Q_DECLARE_OPERATORS_FOR_FLAGS(PsdShortFields)

} // namespace client
