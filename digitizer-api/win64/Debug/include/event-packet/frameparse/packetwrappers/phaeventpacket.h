#pragma once

#include "eventpacketbase.h"

#include "packets/phanetworkpacket.h"

#include <memory>

namespace network
{

class PhaEventPacket : public EventPacket
{
    Q_OBJECT
  public:
    PhaEventPacket() = default;

    explicit PhaEventPacket(const std::shared_ptr<PhaNetworkPacket> &packet);
    void serialize(QDataStream &out) const override;
    void deserialize(QDataStream &in) override;

  protected:
    [[nodiscard]] bool compare(EventPacket *other) const override;

  public:
    qint64 m_trapBaseline{};
    qint64 m_trapHeightMean{};
    qint64 m_trapHeightMax{};
    quint32 m_eventCounter{};
    qint16 m_rcCr2Y1{};
    qint16 m_rcCr2Y2{};
    quint16 m_reserved[3]{};

  private:
    static constexpr size_t kPhaCommonFieldsSize = sizeof(qint64) + sizeof(qint64) + sizeof(qint64) + sizeof(quint32) + sizeof(qint16) + sizeof(qint16);

    void assignFrom(const PhaNetworkPacket &packet);
};

} // namespace network
