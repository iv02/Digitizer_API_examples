#pragma once

#include "packets/eventpackettype.h"

#include <QByteArrayView>
#include <QDebug>

#include <cstring>
#include <expected>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace network
{

template <typename T> inline T readUnaligned(const char *data)
{
    static_assert(std::is_trivially_copyable_v<T>);
    T value{};
    std::memcpy(&value, data, sizeof(T));
    return value;
}

inline quint16 calculateChecksum(const char *data, int size)
{
    quint16 checksum = 0;
    const int wordsCnt = size / 2;
    const auto *buff = reinterpret_cast<const quint16 *>(data);

    for (auto it = buff; it < buff + wordsCnt; it++)
        checksum += *it;

    return ~checksum;
}

enum class EventError
{
    NotEnoughBytes,
    InvalidDeviceId,
    UnsupportedPacketType,
    ChecksumMismatch,
    ParseError,
    RtcMismatch
};

template <typename T, auto fieldPtr, typename U>
concept has_field = requires(T t) {
    { t.*fieldPtr } -> std::convertible_to<U>;
};

template <typename T>
concept has_no_array_length_field = !requires(T t) { t.arrayLength; };

template <typename T>
concept has_no_padding_length_field = !requires(T t) { t.paddingLength; };

template <typename T>
concept has_no_signature_field = !requires(T t) { t.signature; };

template <typename T>
concept FixedSizeStructure = requires(T t) {
    requires has_field<T, &T::deviceId, quint32>;
    requires has_field<T, &T::packetType, EventPacketType>;
    requires has_field<T, &T::checksum, quint16>;

    requires has_no_array_length_field<T>;
    requires has_no_padding_length_field<T>;
    requires has_no_signature_field<T>;

    { T::size() } -> std::same_as<size_t>;
    { t.deserialize(std::declval<const QByteArrayView &>()) } -> std::same_as<void>;
};

template <typename T>
concept KnownSizeStructure = requires(T t) {
    requires has_field<T, &T::deviceId, quint32>;
    requires has_field<T, &T::packetType, EventPacketType>;
    requires has_field<T, &T::checksum, quint16>;

    requires has_field<T, &T::arrayLength, quint32>;
    requires has_field<T, &T::paddingLength, quint32>;
    requires has_no_signature_field<T>;

    { t.deserialize(std::declval<const QByteArrayView &>()) } -> std::same_as<void>;
    { t.arrayLengthOffset() } -> std::same_as<quint32>;
    { t.paddingLengthOffset() } -> std::same_as<quint32>;
    { t.fixedPartSize() } -> std::same_as<quint32>;
    { t.arrayItemSize() } -> std::same_as<quint32>;
};

template <typename T>
concept UnknownSizeStructure = requires(T t) {
    requires has_field<T, &T::deviceId, quint32>;
    requires has_field<T, &T::packetType, EventPacketType>;
    requires has_field<T, &T::checksum, quint16>;

    requires has_no_array_length_field<T>;
    requires has_no_padding_length_field<T>;
    requires has_field<T, &T::receivedSignature, quint16 *>;

    { t.deserialize(std::declval<const QByteArrayView &>(), std::declval<size_t>()) } -> std::same_as<void>;
    { t.fixedPartSize() } -> std::same_as<quint32>;
    { t.arrayPartSize() } -> std::same_as<quint32>;
    { t.arrayLimit() } -> std::same_as<quint32>;
    { t.signature() } -> std::same_as<QByteArray>;
};

template <typename T> class PacketParser final
{
  public:
    explicit PacketParser(EventPacketType packetType) : m_packetType(packetType)
    {
    }

    PacketParser(const PacketParser &) = delete;
    PacketParser(PacketParser &&) = delete;
    PacketParser &operator=(const PacketParser &) = delete;
    PacketParser &operator=(PacketParser &&) = delete;
    ~PacketParser() = default;

    void setDeviceId(quint32 deviceId)
    {
        m_deviceId = deviceId;
    }

    using ParseResult = std::expected<std::shared_ptr<T>, EventError>;
    using ParseFromBufferResult = std::expected<std::pair<std::shared_ptr<T>, int>, EventError>;

    ParseResult parsePacket(QByteArrayView packetArray)
    {
        if constexpr (FixedSizeStructure<T>)
            return parseFixedSizePacket(packetArray);
        else if constexpr (KnownSizeStructure<T>)
            return parseKnownSizePacket(packetArray);
        else if constexpr (UnknownSizeStructure<T>)
            return parseUnknownSizePacket(packetArray);
        else
        {
            qWarning() << "Unsupported packet type for parsing. Expected FixedSizeStructure, KnownSizeStructure or UnknownSizeStructure.";
            return std::unexpected(EventError::ParseError);
        }
    }

    ParseFromBufferResult parseFromBuffer(QByteArrayView bufferFromOffset)
    {
        if constexpr (FixedSizeStructure<T>)
            return parseFixedSizeFromBuffer(bufferFromOffset);
        else if constexpr (KnownSizeStructure<T>)
            return parseKnownSizeFromBuffer(bufferFromOffset);
        else if constexpr (UnknownSizeStructure<T>)
            return parseUnknownSizeFromBuffer(bufferFromOffset);
        else
        {
            qWarning() << "Unsupported packet type for parsing. Expected FixedSizeStructure, KnownSizeStructure or UnknownSizeStructure.";
            return std::unexpected(EventError::ParseError);
        }
    }

    ParseResult parseFixedSizePacket(QByteArrayView packetArray)
    {
        if (packetArray.size() < static_cast<int>(T::size()))
            return std::unexpected(EventError::NotEnoughBytes);

        constexpr auto checksumSize = static_cast<int>(sizeof(quint16));
        const auto checksum = calculateChecksum(packetArray.constData(), static_cast<int>(packetArray.size()) - checksumSize);

        auto packet = std::make_shared<T>();
        packet->deserialize(packetArray.sliced(0, static_cast<int>(T::size())));

        if (packet->deviceId != m_deviceId)
        {
            qWarning() << "Invalid device ID in packet. Expected:" << m_deviceId << "Received:" << packet->deviceId;
            qWarning() << "Packet array:" << packetArray.toByteArray().toHex();
            return std::unexpected(EventError::InvalidDeviceId);
        }

        if (packet->packetType != m_packetType)
        {
            qWarning() << "Unsupported packet type. Expected:" << static_cast<int>(m_packetType) << "Received:" << static_cast<int>(packet->packetType);
            qWarning() << "Packet array:" << packetArray.toByteArray().toHex();
            return std::unexpected(EventError::UnsupportedPacketType);
        }

        if (packet->checksum != checksum)
        {
            qWarning() << "Checksum mismatch in packet. Expected:" << checksum << "Received:" << packet->checksum;
            qWarning() << "Packet array:" << packetArray.toByteArray().toHex();
            return std::unexpected(EventError::ChecksumMismatch);
        }

        return packet;
    }

    ParseResult parseKnownSizePacket(QByteArrayView packetArray)
    {
        if (packetArray.size() < static_cast<int>(T::fixedPartSize()))
            return std::unexpected(EventError::NotEnoughBytes);

        const auto arrayLength = readUnaligned<quint32>(packetArray.constData() + T::arrayLengthOffset());
        const auto paddingLength = static_cast<quint32>(readUnaligned<quint16>(packetArray.constData() + T::paddingLengthOffset()));
        const auto checksumBytes64 = static_cast<quint64>(T::fixedPartSize()) + static_cast<quint64>(arrayLength) * static_cast<quint64>(T::arrayItemSize());
        const auto totalSize64 = checksumBytes64 + static_cast<quint64>(paddingLength) * sizeof(qint16) + sizeof(quint16);
        if (totalSize64 > static_cast<quint64>(std::numeric_limits<int>::max()))
            return std::unexpected(EventError::ParseError);

        const auto totalSize = static_cast<int>(totalSize64);

        if (packetArray.size() < totalSize)
            return std::unexpected(EventError::NotEnoughBytes);

        const auto checksum = calculateChecksum(packetArray.constData(), static_cast<int>(checksumBytes64));

        auto packet = std::make_shared<T>();
        packet->deserialize(packetArray.sliced(0, totalSize));

        if (packet->deviceId != m_deviceId)
        {
            qWarning() << "Invalid device ID in packet. Expected:" << m_deviceId << "Received:" << packet->deviceId;
            qWarning() << "Packet array:" << packetArray.toByteArray().toHex();
            return std::unexpected(EventError::InvalidDeviceId);
        }

        if (packet->packetType != m_packetType)
        {
            qWarning() << "Unsupported packet type. Expected:" << static_cast<int>(m_packetType) << "Received:" << static_cast<int>(packet->packetType);
            qWarning() << "Packet array:" << packetArray.toByteArray().toHex();
            return std::unexpected(EventError::UnsupportedPacketType);
        }

        if (packet->checksum != checksum)
        {
            qWarning() << "Checksum mismatch in packet. Expected:" << checksum << "Received:" << packet->checksum;
            qWarning() << "Packet array:" << packetArray.toByteArray().toHex();
            return std::unexpected(EventError::ChecksumMismatch);
        }

        return packet;
    }

    ParseResult parseUnknownSizePacket(QByteArrayView packetArray)
    {
        if (packetArray.size() < static_cast<int>(T::fixedPartSize()))
            return std::unexpected(EventError::NotEnoughBytes);

        const auto sig = T::signature();

        for (quint32 xyCounter = 0; xyCounter < T::arrayLimit(); ++xyCounter)
        {
            const auto mayBeSignaturePos = T::fixedPartSize() + xyCounter * T::arrayPartSize();
            const auto mayBePacketEnd = mayBeSignaturePos + static_cast<int>(sig.size()) + static_cast<int>(sizeof(quint16));
            if (packetArray.size() < mayBePacketEnd)
                return std::unexpected(EventError::NotEnoughBytes);

            if (packetArray.sliced(mayBeSignaturePos, sig.size()) != sig)
                continue;

            constexpr auto checksumSize = static_cast<int>(sizeof(quint16));
            const auto checksum = calculateChecksum(packetArray.constData(), mayBePacketEnd - checksumSize);

            auto packet = std::make_shared<T>();
            packet->deserialize(packetArray.sliced(0, mayBePacketEnd), xyCounter);

            if (packet->deviceId != m_deviceId)
            {
                qWarning() << "Invalid device ID in packet. Expected:" << m_deviceId << "Received:" << packet->deviceId;
                qWarning() << "Packet array:" << packetArray.toByteArray().toHex();
                return std::unexpected(EventError::InvalidDeviceId);
            }

            if (packet->packetType != m_packetType)
            {
                qWarning() << "Unsupported packet type. Expected:" << static_cast<int>(m_packetType) << "Received:" << static_cast<int>(packet->packetType);
                qWarning() << "Packet array:" << packetArray.toByteArray().toHex();
                return std::unexpected(EventError::UnsupportedPacketType);
            }

            if (packet->checksum != checksum)
            {
                qWarning() << "Checksum mismatch in packet. Expected:" << checksum << "Received:" << packet->checksum;
                qWarning() << "Packet array:" << packetArray.toByteArray().toHex();
                return std::unexpected(EventError::ChecksumMismatch);
            }

            return packet;
        }

        qWarning() << "No valid packet found in buffer. Device ID:" << m_deviceId << "Packet type:" << static_cast<int>(m_packetType);
        return std::unexpected(EventError::ParseError);
    }

    ParseFromBufferResult parseFixedSizeFromBuffer(QByteArrayView bufferFromOffset)
    {
        const int totalSize = static_cast<int>(T::size());
        if (bufferFromOffset.size() < totalSize)
            return std::unexpected(EventError::NotEnoughBytes);

        auto parsed = parseFixedSizePacket(bufferFromOffset.sliced(0, totalSize));
        if (!parsed.has_value())
            return std::unexpected(parsed.error());
        return std::make_pair(std::move(*parsed), totalSize);
    }

    ParseFromBufferResult parseKnownSizeFromBuffer(QByteArrayView bufferFromOffset)
    {
        if (bufferFromOffset.size() < static_cast<int>(T::fixedPartSize()))
            return std::unexpected(EventError::NotEnoughBytes);

        const auto arrayLength = readUnaligned<quint32>(bufferFromOffset.constData() + T::arrayLengthOffset());
        const auto paddingLength = static_cast<quint32>(readUnaligned<quint16>(bufferFromOffset.constData() + T::paddingLengthOffset()));
        const auto totalSize64 = static_cast<quint64>(T::fixedPartSize()) + static_cast<quint64>(arrayLength) * static_cast<quint64>(T::arrayItemSize()) +
                                 static_cast<quint64>(paddingLength) * sizeof(qint16) + sizeof(quint16);
        if (totalSize64 > static_cast<quint64>(std::numeric_limits<int>::max()))
            return std::unexpected(EventError::ParseError);

        const int totalSize = static_cast<int>(totalSize64);

        if (bufferFromOffset.size() < totalSize)
            return std::unexpected(EventError::NotEnoughBytes);

        auto parsed = parseKnownSizePacket(bufferFromOffset.sliced(0, totalSize));
        if (!parsed.has_value())
            return std::unexpected(parsed.error());
        return std::make_pair(std::move(*parsed), totalSize);
    }

    ParseFromBufferResult parseUnknownSizeFromBuffer(QByteArrayView bufferFromOffset)
    {
        if (bufferFromOffset.size() < static_cast<int>(T::fixedPartSize()))
            return std::unexpected(EventError::NotEnoughBytes);

        const auto sig = T::signature();

        for (quint32 xyCounter = 0; xyCounter < T::arrayLimit(); ++xyCounter)
        {
            const auto mayBeSignaturePos = T::fixedPartSize() + xyCounter * T::arrayPartSize();
            const auto mayBePacketEnd = mayBeSignaturePos + static_cast<int>(sig.size()) + static_cast<int>(sizeof(quint16));
            if (bufferFromOffset.size() < mayBePacketEnd)
                return std::unexpected(EventError::NotEnoughBytes);

            if (bufferFromOffset.sliced(mayBeSignaturePos, sig.size()) != sig)
                continue;

            auto parsed = parseUnknownSizePacket(bufferFromOffset.sliced(0, mayBePacketEnd));
            if (!parsed.has_value())
                return std::unexpected(parsed.error());
            return std::make_pair(std::move(*parsed), mayBePacketEnd);
        }

        qWarning() << "No valid packet found in buffer. Device ID:" << m_deviceId << "Packet type:" << static_cast<int>(m_packetType);
        return std::unexpected(EventError::ParseError);
    }

    EventPacketType packetType() const
    {
        return m_packetType;
    }

  private:
    quint32 m_deviceId{};
    EventPacketType m_packetType{};
};

} // namespace network
