#pragma once

#include "eventpacketbase.h"

#include "packets/psdnetworkpacket.h"
#include "packets/psdnetworkpacketv2.h"

#include <memory>

namespace network
{

class PsdEventPacket final : public EventPacket
{
    Q_OBJECT
  public:
    PsdEventPacket() = default;

    explicit PsdEventPacket(const std::shared_ptr<PsdNetworkPacket> &packet);

    explicit PsdEventPacket(const std::shared_ptr<PsdNetworkPacketV2> &packetV2);

    void serialize(QDataStream &out) const override;
    void deserialize(QDataStream &in) override;

  protected:
    [[nodiscard]] bool compare(EventPacket *other) const override;

  public:
    qint32 m_qShort{};
    qint32 m_qLong{};
    qint16 m_cfdY1{};
    qint16 m_cfdY2{};
    qint16 m_baseline{};
    qint16 m_height{};
    quint32 m_eventCounter{};
    quint32 m_eventCounterPsd{};
    qint16 m_psdValue{};
    quint16 m_reserved[2]{};

  private:
    static constexpr size_t kPsdCommonFieldsSize = sizeof(qint32) + sizeof(qint32) + sizeof(qint16) + sizeof(qint16) + sizeof(qint16) +
        sizeof(qint16) + sizeof(quint32) + sizeof(quint32);

    void assignFrom(const PsdNetworkPacket &packet);
    void assignFrom(const PsdNetworkPacketV2 &packet);
};

} // namespace network
