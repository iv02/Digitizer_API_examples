#pragma once

#include "packets/eventpackettype.h"

#include <QDataStream>
#include <QDebug>

#include <expected>

namespace network
{

inline quint16 calculateChecksum(const QByteArray &data)
{
    quint16 checksum = 0, wordsCnt = static_cast<quint16>(data.size()) / 2;
    const auto *buff = reinterpret_cast<const quint16 *>(data.data());

    for (auto it = buff; it < buff + wordsCnt; it++)
        checksum += *it;

    checksum = ~checksum;

    return checksum;
}

enum class EventError
{
    NotEnoughBytes,
    InvalidDeviceId,
    UnsupportedPacketType,
    ChecksumMismatch,
    ParseError
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
    { t.deserialize(std::declval<QDataStream &>()) } -> std::same_as<QDataStream &>;
};

template <typename T>
concept KnownSizeStructure = requires(T t) {
    requires has_field<T, &T::deviceId, quint32>;
    requires has_field<T, &T::packetType, EventPacketType>;
    requires has_field<T, &T::checksum, quint16>;

    requires has_field<T, &T::arrayLength, quint32>;
    requires has_field<T, &T::paddingLength, quint32>;
    requires has_no_signature_field<T>;

    { t.deserialize(std::declval<QDataStream &>()) } -> std::same_as<QDataStream &>;
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

    { t.deserialize(std::declval<QDataStream &>(), std::declval<size_t>()) } -> std::same_as<QDataStream &>;
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

    std::expected<std::pair<T, QByteArray>, EventError> parsePacket(QByteArrayView packetArray)
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

    std::expected<std::pair<T, QByteArray>, EventError> parseFixedSizePacket(QByteArrayView packetArray)
    {
        if (packetArray.size() < static_cast<int>(T::size()))
            return std::unexpected(EventError::NotEnoughBytes);

        const QByteArray copy = packetArray.toByteArray();
        const auto checksum = calculateChecksum(copy.chopped(sizeof(quint16)));

        QDataStream stream(copy);
        stream.setByteOrder(QDataStream::LittleEndian);

        T packet{};
        packet.deserialize(stream);

        if (packet.deviceId != m_deviceId)
        {
            qWarning() << "Invalid device ID in packet. Expected:" << m_deviceId << "Received:" << packet.deviceId;
            qWarning() << "Packet array:" << copy.toHex();
            return std::unexpected(EventError::InvalidDeviceId);
        }

        if (packet.packetType != m_packetType)
        {
            qWarning() << "Unsupported packet type. Expected:" << static_cast<int>(m_packetType) << "Received:" << static_cast<int>(packet.packetType);
            qWarning() << "Packet array:" << copy.toHex();
            return std::unexpected(EventError::UnsupportedPacketType);
        }

        if (packet.checksum != checksum)
        {
            qWarning() << "Checksum mismatch in packet. Expected:" << checksum << "Received:" << packet.checksum;
            qWarning() << "Packet array:" << copy.toHex();
            return std::unexpected(EventError::ChecksumMismatch);
        }

        return std::make_pair(packet, copy);
    }

    std::expected<std::pair<T, QByteArray>, EventError> parseKnownSizePacket(QByteArrayView packetArray)
    {
        if (packetArray.size() < static_cast<int>(T::fixedPartSize()))
            return std::unexpected(EventError::NotEnoughBytes);

        const auto arrayLength = *reinterpret_cast<const quint32 *>(packetArray.constData() + T::arrayLengthOffset());
        const auto paddingLength = *reinterpret_cast<const quint16 *>(packetArray.constData() + T::paddingLengthOffset());
        const auto totalSize = static_cast<int>(T::fixedPartSize() + arrayLength * T::arrayItemSize() + paddingLength * sizeof(qint16) + sizeof(qint16));

        if (packetArray.size() < totalSize)
            return std::unexpected(EventError::NotEnoughBytes);

        const QByteArray copy = packetArray.toByteArray();
        const auto checksum = calculateChecksum(copy.chopped(paddingLength * sizeof(qint16) + sizeof(quint16)));

        QDataStream stream(copy);
        stream.setByteOrder(QDataStream::LittleEndian);

        T packet{};
        packet.deserialize(stream);

        if (packet.deviceId != m_deviceId)
        {
            qWarning() << "Invalid device ID in packet. Expected:" << m_deviceId << "Received:" << packet.deviceId;
            qWarning() << "Packet array:" << copy.toHex();
            return std::unexpected(EventError::InvalidDeviceId);
        }

        if (packet.packetType != m_packetType)
        {
            qWarning() << "Unsupported packet type. Expected:" << static_cast<int>(m_packetType) << "Received:" << static_cast<int>(packet.packetType);
            qWarning() << "Packet array:" << copy.toHex();
            return std::unexpected(EventError::UnsupportedPacketType);
        }

        if (packet.checksum != checksum)
        {
            qWarning() << "Checksum mismatch in packet. Expected:" << checksum << "Received:" << packet.checksum;
            qWarning() << "Packet array:" << copy.toHex();
            return std::unexpected(EventError::ChecksumMismatch);
        }

        return std::make_pair(packet, copy);
    }

    std::expected<std::pair<T, QByteArray>, EventError> parseUnknownSizePacket(QByteArrayView packetArray)
    {
        if (packetArray.size() < static_cast<int>(T::fixedPartSize()))
            return std::unexpected(EventError::NotEnoughBytes);

        for (quint32 xyCounter = 0; xyCounter < T::arrayLimit(); ++xyCounter)
        {
            const auto mayBeSignaturePos = T::fixedPartSize() + xyCounter * T::arrayPartSize();
            const auto mayBePacketEnd = mayBeSignaturePos + static_cast<int>(T::signature().size()) + static_cast<int>(sizeof(quint16));
            if (packetArray.size() < mayBePacketEnd)
                return std::unexpected(EventError::NotEnoughBytes);

            if (packetArray.sliced(mayBeSignaturePos, T::signature().size()) != T::signature())
                continue;

            const QByteArray copy = packetArray.toByteArray();
            const auto checksum = calculateChecksum(copy.chopped(sizeof(quint16)));

            QDataStream stream(copy);
            stream.setByteOrder(QDataStream::LittleEndian);

            T packet{};
            packet.deserialize(stream, xyCounter);

            if (packet.deviceId != m_deviceId)
            {
                qWarning() << "Invalid device ID in packet. Expected:" << m_deviceId << "Received:" << packet.deviceId;
                qWarning() << "Packet array:" << copy.toHex();
                return std::unexpected(EventError::InvalidDeviceId);
            }

            if (packet.packetType != m_packetType)
            {
                qWarning() << "Unsupported packet type. Expected:" << static_cast<int>(m_packetType) << "Received:" << static_cast<int>(packet.packetType);
                qWarning() << "Packet array:" << copy.toHex();
                return std::unexpected(EventError::UnsupportedPacketType);
            }

            if (packet.checksum != checksum)
            {
                qWarning() << "Checksum mismatch in packet. Expected:" << checksum << "Received:" << packet.checksum;
                qWarning() << "Packet array:" << copy.toHex();
                return std::unexpected(EventError::ChecksumMismatch);
            }

            return std::make_pair(packet, copy);
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
