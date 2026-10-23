#pragma once

#include "packets/eventpackettype.h"
#include "parsing_mode.h"

#include <QByteArrayView>

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

    for (auto it = buff; it < buff + wordsCnt; ++it)
        checksum = static_cast<quint16>(checksum + *it);

    return static_cast<quint16>(~checksum);
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
    using ParseFromBufferResult = std::expected<std::pair<std::shared_ptr<T>, int>, EventError>;

    explicit PacketParser(EventPacketType packetType, ParsingMode parsingMode = ParsingMode::Normal)
        : m_packetType(packetType), m_parsingMode(parsingMode)
    {
    }

    PacketParser(const PacketParser &) = delete;
    PacketParser(PacketParser &&) = delete;
    PacketParser &operator=(const PacketParser &) = delete;
    PacketParser &operator=(PacketParser &&) = delete;
    ~PacketParser() = default;

    void setDeviceId(quint32 deviceId) { m_deviceId = deviceId; }

    ParseFromBufferResult parseFromBuffer(QByteArrayView bufferFromOffset)
    {
        if constexpr (FixedSizeStructure<T>)
            return parseFixedSizeFromBuffer(bufferFromOffset);
        else if constexpr (KnownSizeStructure<T>)
            return parseKnownSizeFromBuffer(bufferFromOffset);
        else if constexpr (UnknownSizeStructure<T>)
            return parseUnknownSizeFromBuffer(bufferFromOffset);
        else
            return std::unexpected(EventError::ParseError);
    }

  private:
    [[nodiscard]] bool verifyChecksum() const noexcept { return m_parsingMode == ParsingMode::Paranoid; }

    ParseFromBufferResult parseFixedSizeFromBuffer(QByteArrayView bufferFromOffset)
    {
        const int totalSize = static_cast<int>(T::size());
        if (bufferFromOffset.size() < totalSize)
            return std::unexpected(EventError::NotEnoughBytes);

        const char *data = bufferFromOffset.constData();
        if (verifyChecksum())
        {
            const auto expected = calculateChecksum(data, totalSize - static_cast<int>(sizeof(quint16)));
            const auto actual = readUnaligned<quint16>(data + totalSize - static_cast<int>(sizeof(quint16)));
            if (actual != expected)
                return std::unexpected(EventError::ChecksumMismatch);
        }

        auto packet = std::make_shared<T>();
        packet->deserialize(bufferFromOffset.sliced(0, totalSize));

        if (packet->deviceId != m_deviceId)
            return std::unexpected(EventError::InvalidDeviceId);
        if (packet->packetType != m_packetType)
            return std::unexpected(EventError::UnsupportedPacketType);

        return std::make_pair(std::move(packet), totalSize);
    }

    ParseFromBufferResult parseKnownSizeFromBuffer(QByteArrayView bufferFromOffset)
    {
        if (bufferFromOffset.size() < static_cast<int>(T::fixedPartSize()))
            return std::unexpected(EventError::NotEnoughBytes);

        const char *data = bufferFromOffset.constData();
        const auto arrayLength = readUnaligned<quint32>(data + T::arrayLengthOffset());
        const auto paddingLength = static_cast<quint32>(readUnaligned<quint16>(data + T::paddingLengthOffset()));
        const auto checksumBytes64 = static_cast<quint64>(T::fixedPartSize()) + static_cast<quint64>(arrayLength) * static_cast<quint64>(T::arrayItemSize());
        const auto totalSize64 = checksumBytes64 + static_cast<quint64>(paddingLength) * sizeof(qint16) + sizeof(quint16);
        if (totalSize64 > static_cast<quint64>(std::numeric_limits<int>::max()))
            return std::unexpected(EventError::ParseError);

        const int totalSize = static_cast<int>(totalSize64);
        if (bufferFromOffset.size() < totalSize)
            return std::unexpected(EventError::NotEnoughBytes);

        if (verifyChecksum())
        {
            const auto expected = calculateChecksum(data, static_cast<int>(checksumBytes64));
            const auto actual = readUnaligned<quint16>(data + static_cast<int>(checksumBytes64));
            if (actual != expected)
                return std::unexpected(EventError::ChecksumMismatch);
        }

        auto packet = std::make_shared<T>();
        packet->deserialize(bufferFromOffset.sliced(0, totalSize));

        if (packet->deviceId != m_deviceId)
            return std::unexpected(EventError::InvalidDeviceId);
        if (packet->packetType != m_packetType)
            return std::unexpected(EventError::UnsupportedPacketType);

        return std::make_pair(std::move(packet), totalSize);
    }

    ParseFromBufferResult parseUnknownSizeFromBuffer(QByteArrayView bufferFromOffset)
    {
        if (bufferFromOffset.size() < static_cast<int>(T::fixedPartSize()))
            return std::unexpected(EventError::NotEnoughBytes);

        const auto sig = T::signature();
        const char *data = bufferFromOffset.constData();

        for (quint32 xyCounter = 0; xyCounter < T::arrayLimit(); ++xyCounter)
        {
            const auto mayBeSignaturePos = T::fixedPartSize() + xyCounter * T::arrayPartSize();
            const auto mayBePacketEnd = mayBeSignaturePos + static_cast<int>(sig.size()) + static_cast<int>(sizeof(quint16));
            if (bufferFromOffset.size() < mayBePacketEnd)
                return std::unexpected(EventError::NotEnoughBytes);

            if (std::memcmp(data + mayBeSignaturePos, sig.constData(), static_cast<size_t>(sig.size())) != 0)
                continue;

            if (verifyChecksum())
            {
                const auto expected = calculateChecksum(data, mayBePacketEnd - static_cast<int>(sizeof(quint16)));
                const auto actual = readUnaligned<quint16>(data + mayBePacketEnd - static_cast<int>(sizeof(quint16)));
                if (actual != expected)
                    return std::unexpected(EventError::ChecksumMismatch);
            }

            auto packet = std::make_shared<T>();
            packet->deserialize(bufferFromOffset.sliced(0, mayBePacketEnd), xyCounter);

            if (packet->deviceId != m_deviceId)
                return std::unexpected(EventError::InvalidDeviceId);
            if (packet->packetType != m_packetType)
                return std::unexpected(EventError::UnsupportedPacketType);

            return std::make_pair(std::move(packet), mayBePacketEnd);
        }

        return std::unexpected(EventError::ParseError);
    }

    quint32 m_deviceId{};
    EventPacketType m_packetType{};
    ParsingMode m_parsingMode{ParsingMode::Normal};
};

} // namespace network
