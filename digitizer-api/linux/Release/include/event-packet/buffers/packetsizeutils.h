#pragma once

#include "packetparser.h"
#include "packets/eventpackettype.h"

#include <QByteArray>
#include <QDebug>

#include <expected>
#include <limits>

namespace network
{

template <typename T> std::expected<int, EventError> computePacketSizeFor(const QByteArray &buffer, int offset, EventPacketType type)
{
    Q_UNUSED(type)

    if constexpr (FixedSizeStructure<T>)
    {
        if (buffer.size() - offset < static_cast<int>(T::size()))
            return std::unexpected(EventError::NotEnoughBytes);

        return static_cast<int>(T::size());
    }
    else if constexpr (KnownSizeStructure<T>)
    {
        if (buffer.size() - offset < static_cast<int>(T::fixedPartSize()))
            return std::unexpected(EventError::NotEnoughBytes);

        const auto arrayLength = *reinterpret_cast<const quint32 *>(buffer.constData() + offset + T::arrayLengthOffset());
        const auto paddingLength = *reinterpret_cast<const quint16 *>(buffer.constData() + offset + T::paddingLengthOffset());
        const quint64 totalSize64 = static_cast<quint64>(T::fixedPartSize()) + static_cast<quint64>(arrayLength) * static_cast<quint64>(T::arrayItemSize()) +
                                    static_cast<quint64>(paddingLength) * static_cast<quint64>(sizeof(qint16)) + static_cast<quint64>(sizeof(qint16));

        if (totalSize64 == 0 || totalSize64 > static_cast<quint64>(std::numeric_limits<int>::max()))
            return std::unexpected(EventError::ParseError);

        const int totalSize = static_cast<int>(totalSize64);

        if (buffer.size() - offset < totalSize)
            return std::unexpected(EventError::NotEnoughBytes);

        return totalSize;
    }
    else if constexpr (UnknownSizeStructure<T>)
    {
        if (buffer.size() - offset < static_cast<int>(T::fixedPartSize()))
            return std::unexpected(EventError::NotEnoughBytes);

        for (quint32 xyCounter = 0; xyCounter < T::arrayLimit(); ++xyCounter)
        {
            const quint64 mayBeSignaturePos64 = static_cast<quint64>(offset) + static_cast<quint64>(T::fixedPartSize()) +
                                                static_cast<quint64>(xyCounter) * static_cast<quint64>(T::arrayPartSize());
            const quint64 mayBePacketEnd64 = mayBeSignaturePos64 + static_cast<quint64>(T::signature().size()) + static_cast<quint64>(sizeof(quint16));

            if (std::cmp_greater(mayBePacketEnd64, buffer.size()))
                return std::unexpected(EventError::NotEnoughBytes);

            if (mayBeSignaturePos64 > static_cast<quint64>(std::numeric_limits<int>::max()) ||
                static_cast<size_t>(T::signature().size()) > static_cast<size_t>(std::numeric_limits<int>::max()))
                return std::unexpected(EventError::ParseError);

            const int mayBeSignaturePos = static_cast<int>(mayBeSignaturePos64);
            const int signatureSize = static_cast<int>(T::signature().size());

            if (buffer.sliced(mayBeSignaturePos, signatureSize) != T::signature())
                continue;

            const quint64 packetSize64 = mayBePacketEnd64 - static_cast<quint64>(offset);
            if (packetSize64 == 0 || packetSize64 > static_cast<quint64>(std::numeric_limits<int>::max()))
                return std::unexpected(EventError::ParseError);

            return static_cast<int>(packetSize64);
        }

        qWarning() << "No valid packet found in the buffer for type" << static_cast<int>(type);
        return std::unexpected(EventError::ParseError);
    }
    else
    {
        return std::unexpected(EventError::ParseError);
    }
}

} // namespace network
